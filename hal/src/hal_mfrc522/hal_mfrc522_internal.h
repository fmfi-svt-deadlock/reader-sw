/**
 * @file    hal_mfrc522_internal.h
 *
 * Although most of the drivers in the HAL are contained in one file, the
 * MFRC522 driver was becoming unreadable, so I've split it between several
 * files.
 *
 *   - `src/hal_mfrc522/hal_mfrc522_internal.h` header contains internal
 *     constants and internal documentation. It is not supposed to be used
 *     by application using this driver.
 *   - `src/hal_mfrc522/hal_mfrc522.c` is the main driver file. It contains
 *     MFRC522-specific initialization and configuration functions. It also
 *     contains the interrupt handler only purpose of which is to wake up a
 *     sleeping thread.
 *   - `src/hal_mfrc522/hal_mfrc522_ext_api.c` contains implementation of
 *     extended features. See hal_abstract_iso14443_pcd_ext.h.
 *   - `src/hal_mfrc522/hal_mfrc522_llcom.c` contains low-level communication
 *     routines for reading and writing registers of the MFRC522 over various
 *     connection interfaces.
 *   - `src/hal_mfrc552/hal_mfrc522_pcd_api.c` contains implementation of
 *     Pcd API functions for MFRC522.
 *
 * Primary goals
 * -------------
 *
 * This driver should provide easy-to-use synchronous API to drivers of
 * higher protocol layers while being efficient and friendly to other threads
 * (it suspends the calling thread while it's waiting for data).
 *
 * Initialization and configuration functions handling global objects of this
 * driver must be thread-safe.
 *
 * This driver does not guarantee thread safety if single Mfrc522Driver file
 * is used simultaneously by multiple threads. Those threads should call
 * #pcdAcquireBus and #pcdReleaseBus to achiveve mutual exclusion.
 * However, it is safe to use different Mfrc522Driver objects from different
 * threads simultaneously.
 *
 * This driver should be as universal as possible. This is the reason for
 * (over?)complicated configuration structure. It allows you to, when you
 * know what are you doing, configure the MFRC522 modulee for your specific
 * use-case (like using an external modulator with the chip). It is also easy
 * to use, since default values work out-of-the box with the typical use-case
 * (like the RFID-RC522 chinese module).
 *
 * Thread suspend and interrupt handling
 * -------------------------------------
 *
 * Each time this driver waits for the action of the reader it suspends the
 * calling thread, to allow other threads to run.
 *
 * MFRC522 has a mechanism of waking up the host using its IRQ pin. Host can
 * conigure which interrupts propagate to the interrupt pin, and later figure
 * out which interrupt occured by reading a specific register.
 *
 * This driver uses the Ext driver provided by ChibiOS to handle these
 * interrupts. It registers its own interrupt handler, the same for each
 * interrupt channel. However, in the current version the Ext driver doesn't
 * allow passing custom parameters to the handler function, therefore when an
 * interrupt occurs we only know which channel it came from. Then we may wake
 * up all sleeping driver threads and let every thread check whether the
 * interrupt is intended for it. This is problemating due to race conditions,
 * necessity of events buffering, etc.
 * Instead, we have imposed limitation that each interrupt channel can have
 * only one reader (and nothing else than the reader) attached to it. Therefore
 * when an interrupt occures by knowing the interrupt channel we can wake
 * up the proper thread.
 *
 * In addition this wake-up is buffered if the thread couldn't be suspended
 * in time.
 *
 */

/**
 * @cond
 */

#include "hal_custom.h"

//-----------------------------------------------------------------------------
// Internal constants
//-----------------------------------------------------------------------------

typedef enum {
    Reserved00          = 0x00,
    CommandReg          = 0x01,
    ComIEnReg           = 0x02,
    DivIEnReg           = 0x03,
    ComIrqReg           = 0x04,
    DivIrqReg           = 0x05,
    ErrorReg            = 0x06,
    Status1Reg          = 0x07,
    Status2Reg          = 0x08,
    FIFODataReg         = 0x09,
    FIFOLevelReg        = 0x0A,
    WaterLevelReg       = 0x0B,
    ControlReg          = 0x0C,
    BitFramingReg       = 0x0D,
    CollReg             = 0x0E,
    Reserved01          = 0x0F,
    Reserved10          = 0x10,
    ModeReg             = 0x11,
    TxModeReg           = 0x12,
    RxModeReg           = 0x13,
    TxControlReg        = 0x14,
    TxASKReg            = 0x15,
    TxSelReg            = 0x16,
    RxSelReg            = 0x17,
    RxThresholdReg      = 0x18,
    DemodReg            = 0x19,
    Reserved11          = 0x1A,
    Reserved12          = 0x1B,
    MfTxReg             = 0x1C,
    MfRxReg             = 0x1D,
    Reserved14          = 0x1E,
    SerialSpeedReg      = 0x1F,
    Reserved20          = 0x20,
    CRCResultRegL       = 0x21,
    CRCResultRegH       = 0x22,
    Reserved21          = 0x23,
    ModWidthReg         = 0x24,
    Reserved22          = 0x25,
    RFCfgReg            = 0x26,
    GsNReg              = 0x27,
    CWGsPReg            = 0x28,
    ModGsPReg           = 0x29,
    TModeReg            = 0x2A,
    TPrescalerReg       = 0x2B,
    TReloadRegH         = 0x2C,
    TReloadRegL         = 0x2D,
    TCounterValueRegH   = 0x2E,
    TCounterValueRegL   = 0x2F,
    Reserved30          = 0x30,
    TestSel1Reg         = 0x31,
    TestSel2Reg         = 0x32,
    TestPinEnReg        = 0x33,
    TestPinValueReg     = 0x34,
    TestBusReg          = 0x35,
    AutoTestReg         = 0x36,
    VersionReg          = 0x37,
    AnalogTestReg       = 0x38,
    TestDAC1Reg         = 0x39,
    TestDAC2Reg         = 0x3A,
    TestADCReg          = 0x3B,
    Reserved31          = 0x3C,
    Reserved32          = 0x3D,
    Reserved33          = 0x3E,
    Reserved34          = 0x3F,
} Mfrc522Register;

typedef enum {
    Command_Idle        = 0x0,
    Command_Mem         = 0x1,
    Command_GenRandID   = 0x2,
    Command_CalcCRC     = 0x3,
    Command_Transmit    = 0x4,
    Command_NoChange    = 0x7,
    Command_Receive     = 0x8,
    Command_Transceive  = 0xC,
    Command_Authent     = 0xE,
    Command_SoftReset   = 0xF
} Mfrc522Command;

// ------ REGISTER DEFINES -------
// This list may be incomplete and is expanded on a need-to-use basis.
// Refer to the MFRC522 Datasheet.

// TODO this should be complete and more logically organized one day

#define ModeReg_TxWaitRF                5
#define ModeReg_PolMFin                 3
#define ModeReg_CRCPreset               0
#define ModeReg_CRCPreset_0000          0b00
#define ModeReg_CRCPreset_6363          0b01
#define ModeReg_CRCPreset_A671          0b10
#define ModeReg_CRCPreset_FFFF          0b11
#define Mask_ModeReg_CRCPreset          (0x3 << ModeReg_CRCPreset)

#define TxModeReg_InvMod                3
#define TxModeReg_TxSpeed               4
#define TxModeReg_TxSpeed_106           0b000
#define TxModeReg_TxSpeed_212           0b001
#define TxModeReg_TxSpeed_424           0b010
#define TxModeReg_TxSpeed_848           0b011
#define TxModeReg_TxCRCEn               7
#define Mask_TxModeReg_TxSpeed          (0x7 << TxModeReg_TxSpeed)

#define RxModeReg_RxSpeed               4
#define RxModeReg_RxSpeed_106           0b000
#define RxModeReg_RxSpeed_212           0b001
#define RxModeReg_RxSpeed_424           0b010
#define RxModeReg_RxSpeed_848           0b011
#define RxModeReg_RxCRCEn               7
#define Mask_RxModeReg_RxSpeed          (0x7 << RxModeReg_RxSpeed)

#define TxSelReg_DriverSel              4
#define Mask_TxSelReg_DriverSel         (0x3 << TxSelReg_DriverSel)
#define TxSelReg_MFOutSel               0
#define Mask_TxSelReg_MFOutSel          (0xF << TxSelReg_MFOutSel)

#define RxSelReg_UARTSel                6
#define Mask_RxSelReg_UARTSel           (0x3 << RxSelReg_UARTSel)

#define RxThresholdReg_MinLevel         4
#define Mask_RxThresholdReg_MinLevel    (0xF << RxThresholdReg_MinLevel)
#define RxThresholdReg_CollLevel        0
#define Mask_RxThresholdReg_CollLevel   (0x7 << RxThresholdReg_CollLevel)

#define RFCfgReg_RxGain                 4
#define Mask_RFCfgReg_RxGain            (0x7 << RFCfgReg_RxGain)

#define GsNReg_CWGsN                    4
#define GsNReg_ModGsN                   0

#define CWGsPReg_CWGsP                  0
#define Mask_CWGsPReg_CWGsP             (0x3F << CWGsPReg_CWGsP)

#define ModGsPReg_ModGsP                0
#define Mask_ModGsPReg_ModGsP           (0x3F << ModGsPReg_ModGsP)

#define AutoTestReg_SelfTest            0
#define AutoTestReg_SelfTestEnabled     0b1001

#define TxControlReg_Tx1RFEn            0
#define TxControlReg_Tx2RFEn            1

#define TxAskReg_Force100ASK            6

#define BitFramingReg_TxLastBits        0
#define BitFramingReg_RxAlign           4
#define BitFramingReg_StartSend         7

#define ComIEnReg_IRqInv                7
#define ComIEnReg_TxIEn                 6
#define ComIEnReg_RxIEn                 5
#define ComIEnReg_IdleIEn               4
#define ComIEnReg_HiAlertEn             3
#define ComIEnReg_LoAlertEn             2
#define ComIEnReg_ErrIEn                1
#define ComIEnReg_TimerIEn              0

#define ComIrqReg_Set1                  7
#define ComIrqReg_TxIRq                 6
#define ComIrqReg_RxIRq                 5
#define ComIrqReg_IdleIRq               4
#define ComIrqReg_HiAlertRq             3
#define ComIrqReg_LoAlertRq             2
#define ComIrqReg_ErrIRq                1
#define ComIrqReg_TimerIRq              0

#define FIFOLevelReg_FlushBuffer        7

#define DivIEnReg_IRQPushPull           7

#define ErrorReg_WrErr                  7
#define ErrorReg_TempErr                6
#define ErrorReg_BufferOvfl             4
#define ErrorReg_CollErr                3
#define ErrorReg_CRCErr                 2
#define ErrorReg_ParityErr              1
#define ErrorReg_ProtocolErr            0

#define ControlReg_RxLastBits           0
#define Mask_ControlReg_RxLastBits      0x7

#define CollReg_CollPos                 0
#define Mask_CollReg_CollPos            0x1F
#define CollReg_CollPosNotValid         5
#define CollReg_ValuesAfterColl         7

// --- Thread wakeup messages ---
#define MFRC522_MSG_INTERRUPT           1
#define MFRC522_MSG_PEND_INTERRUPT      2

extern const PcdSParams supported_params;

//-----------------------------------------------------------------------------
// Internal functions and macros
//-----------------------------------------------------------------------------

// Low-level communication functions
// hal_mfrc522_llcom.c

void mfrc522_write_register(Mfrc522Driver *mdp, Mfrc522Register reg,
                            uint8_t value);
uint8_t mfrc522_read_register(Mfrc522Driver *mdp, Mfrc522Register reg);
void mfrc522_write_register_burst(Mfrc522Driver *mdp, Mfrc522Register reg,
                                  const uint8_t *values, uint8_t length);
void mfrc522_read_register_burst(Mfrc522Driver *mdp, Mfrc522Register reg,
                                 uint8_t *values, uint8_t length);

#define mfrc522_write_register_bitmask(mdp, reg, bitmask, data)               \
    mfrc522_write_register(mdp, reg,                                          \
                           (mfrc522_read_register(mdp,                        \
                                                  reg) & ~(bitmask)) | (data));

#define mfrc522_set_register_bits(mdp, reg, data)                             \
    mfrc522_write_register(mdp, reg,                                          \
                           (mfrc522_read_register(mdp, reg) | (data)));

#define mfrc522_clear_register_bits(mdp, reg, data)                           \
    mfrc522_write_register(mdp, reg,                                          \
                           (mfrc522_read_register(mdp, reg) & ~(data)));

#define mfrc522_command(mdp, command)                                         \
    mfrc522_write_register(mdp, CommandReg, command);

// PCD API
// hal_mfrc522_pcd_api.c

// Virtual Method Table containing pointers internal API functions
extern const struct BasePcdVMT mfrc522VMT;

// PCD Extended API (Extended features)
// hal_mfrc522_ext_api.c

bool mfrc522PerformSelftest(Mfrc522Driver *mdp);

/**
 * @endcond
 */
