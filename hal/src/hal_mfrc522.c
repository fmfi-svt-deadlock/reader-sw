#include "hal.h"
#include "hal_custom.h"

#if (HAL_USE_MFRC522 == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

typedef enum {
    Reserved00          = 0x00,
    CommandReg          = 0x01,
    ComIEnReg           = 0x02,
    DivIEnReg           = 0x03,
    CommIrqReg          = 0x04,
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

const PcdSParams supported_params = {
    // Supported tx / rx speeds in mode A
    (PCD_TX_SPEED_106 | PCD_TX_SPEED_212 | PCD_TX_SPEED_424 | PCD_TX_SPEED_848 |
    PCD_RX_SPEED_106 | PCD_RX_SPEED_212 | PCD_RX_SPEED_424 | PCD_RX_SPEED_848),
    // Supported tx / rx speeds in mode B
    0,
    // Support for assymetric speed setting
    true,
    // Supported operation modes (A or B)
    PCD_ISO14443_A,
    // Max tx frame size
    64,
    // Max rx frame size
    64
};

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

void ext_callback(EXTDriver *extp, expchannel_t);

const EXTChannelConfig interrupt_config = {
    EXT_CH_MODE_RISING_EDGE | EXT_MODE_GPIOA,
    &ext_callback
};

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
#define Mask_TxModeReg_TxSpeed          (0x7 << TxModeReg_TxSpeed)

#define RxModeReg_RxSpeed               4
#define RxModeReg_RxSpeed_106           0b000
#define RxModeReg_RxSpeed_212           0b001
#define RxModeReg_RxSpeed_424           0b010
#define RxModeReg_RxSpeed_848           0b011
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

// Self-test expected results
// Version 0.0 (0x90)
static const uint8_t selftest_result_ver00[] = {
    0x00, 0x87, 0x98, 0x0f, 0x49, 0xFF, 0x07, 0x19,
    0xBF, 0x22, 0x30, 0x49, 0x59, 0x63, 0xAD, 0xCA,
    0x7F, 0xE3, 0x4E, 0x03, 0x5C, 0x4E, 0x49, 0x50,
    0x47, 0x9A, 0x37, 0x61, 0xE7, 0xE2, 0xC6, 0x2E,
    0x75, 0x5A, 0xED, 0x04, 0x3D, 0x02, 0x4B, 0x78,
    0x32, 0xFF, 0x58, 0x3B, 0x7C, 0xE9, 0x00, 0x94,
    0xB4, 0x4A, 0x59, 0x5B, 0xFD, 0xC9, 0x29, 0xDF,
    0x35, 0x96, 0x98, 0x9E, 0x4F, 0x30, 0x32, 0x8D
};

// Version 1.0 (0x91)
static const uint8_t selftest_result_ver10[] = {
    0x00, 0xC6, 0x37, 0xD5, 0x32, 0xB7, 0x57, 0x5C,
    0xC2, 0xD8, 0x7C, 0x4D, 0xD9, 0x70, 0xC7, 0x73,
    0x10, 0xE6, 0xD2, 0xAA, 0x5E, 0xA1, 0x3E, 0x5A,
    0x14, 0xAF, 0x30, 0x61, 0xC9, 0x70, 0xDB, 0x2E,
    0x64, 0x22, 0x72, 0xB5, 0xBD, 0x65, 0xF4, 0xEC,
    0x22, 0xBC, 0xD3, 0x72, 0x35, 0xCD, 0xAA, 0x41,
    0x1F, 0xA7, 0xF3, 0x53, 0x14, 0xDE, 0x7E, 0x02,
    0xD9, 0x0F, 0xB5, 0x5E, 0x25, 0x1D, 0x29, 0x79
};
// Version 2.0 (0x92)
static const uint8_t selftest_result_ver20[] = {
    0x00, 0xEB, 0x66, 0xBA, 0x57, 0xBF, 0x23, 0x95,
    0xD0, 0xE3, 0x0D, 0x3D, 0x27, 0x89, 0x5C, 0xDE,
    0x9D, 0x3B, 0xA7, 0x00, 0x21, 0x5B, 0x89, 0x82,
    0x51, 0x3A, 0xEB, 0x02, 0x0C, 0xA5, 0x00, 0x49,
    0x7C, 0x84, 0x4D, 0xB3, 0xCC, 0xD2, 0x1B, 0x81,
    0x5D, 0x48, 0x76, 0xD5, 0x71, 0x61, 0x21, 0xA9,
    0x86, 0x96, 0x83, 0x38, 0xCF, 0x9D, 0x5B, 0x6D,
    0xDC, 0x15, 0xBA, 0x3E, 0x7D, 0x95, 0x3B, 0x2F
};

// Fudan Semiconductor FM17522 (0x88)
static const uint8_t selftest_result_fudan[] = {
    0x00, 0xD6, 0x78, 0x8C, 0xE2, 0xAA, 0x0C, 0x18,
    0x2A, 0xB8, 0x7A, 0x7F, 0xD3, 0x6A, 0xCF, 0x0B,
    0xB1, 0x37, 0x63, 0x4B, 0x69, 0xAE, 0x91, 0xC7,
    0xC3, 0x97, 0xAE, 0x77, 0xF4, 0x37, 0xD7, 0x9B,
    0x7C, 0xF5, 0x3C, 0x11, 0x8F, 0x15, 0xC3, 0xD7,
    0xC1, 0x5B, 0x00, 0x2A, 0xD0, 0x75, 0xDE, 0x9E,
    0x51, 0x64, 0xAB, 0x3E, 0xE9, 0x15, 0xB5, 0xAB,
    0x56, 0x9A, 0x98, 0x82, 0x26, 0xEA, 0x2A, 0x62
};

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

static void write_register(Mfrc522Driver *mdp, Mfrc522Register reg,
                           uint8_t value) {
    switch(mdp->connection_type) {
#if MFRC522_USE_SPI
        case MFRC522_CONN_SPI:
            //TODO call conditionally if the SPI has mutual exclusion enabled
            spiAcquireBus(mdp->iface.spip);
            spiSelect(mdp->iface.spip);

            const uint8_t txbuf[2] = {
                // Bit 7 is zero: write. 6 bit address starting on bit 1.
                ((reg & 0x3F) << 1),
                value,
            };
            uint8_t rxbuf[2];

            spiExchange(mdp->iface.spip, 2, txbuf, rxbuf);

            spiUnselect(mdp->iface.spip);
            spiReleaseBus(mdp->iface.spip);
            break;
#endif
#if MFRC522_USE_I2C
        case MFRC522_CONN_I2C:
            osalSysHalt("MFRC522: I2C not implemented!");
            break;
#endif
#if MFRC522_USE_UART
        case MFRC522_CONN_SERIAL:
            osalSysHalt("MFRC522: Serial not implemented!");
            break;
#endif
    }
}

static uint8_t read_register(Mfrc522Driver *mdp, Mfrc522Register reg) {
    switch(mdp->connection_type) {
#if MFRC522_USE_SPI
        case MFRC522_CONN_SPI:
            //TODO call conditionally if the SPI has mutual exclusion enabled
            spiAcquireBus(mdp->iface.spip);
            spiSelect(mdp->iface.spip);

            const uint8_t txbuf[2] = {
                // Bit 7 is one: read. 6 bit address starting on bit 1.
                0x80 | ((reg & 0x3F) << 1),
                0x00,
            };
            uint8_t rxbuf[2];

            spiExchange(mdp->iface.spip, 2, txbuf, rxbuf);

            spiUnselect(mdp->iface.spip);
            spiReleaseBus(mdp->iface.spip);
            return rxbuf[1];
            break;
#endif
#if MFRC522_USE_I2C
        case MFRC522_CONN_I2C:
            osalSysHalt("MFRC522: I2C not implemented!");
            return 0;
            break;
#endif
#if MFRC522_USE_UART
        case MFRC522_CONN_SERIAL:
            osalSysHalt("MFRC522: Serial not implemented!");
            return 0;
            break;
#endif
    }
}

static void write_register_burst(Mfrc522Driver *mdp, Mfrc522Register reg,
                                 const uint8_t *values, uint8_t length) {
    switch(mdp->connection_type) {
#if MFRC522_USE_SPI
        case MFRC522_CONN_SPI:
            //TODO call conditionally if the SPI has mutual exclusion enabled
            spiAcquireBus(mdp->iface.spip);
            spiSelect(mdp->iface.spip);

            const uint8_t txbuf[1] = {
                // Bit 7 is zero: write. 6 bit address starting on bit 1.
                ((reg & 0x3F) << 1)
            };

            spiSend(mdp->iface.spip, 1, txbuf);
            spiSend(mdp->iface.spip, length, values);

            spiUnselect(mdp->iface.spip);
            spiReleaseBus(mdp->iface.spip);
            break;
#endif
#if MFRC522_USE_I2C
        case MFRC522_CONN_I2C:
            osalSysHalt("MFRC522: I2C not implemented!");
            break;
#endif
#if MFRC522_USE_UART
        case MFRC522_CONN_SERIAL:
            osalSysHalt("MFRC522: Serial not implemented!");
            break;
#endif
    }
}

static void read_register_burst(Mfrc522Driver *mdp, Mfrc522Register reg,
                                uint8_t *values, uint8_t length) {
    switch(mdp->connection_type) {
#if MFRC522_USE_SPI
        case MFRC522_CONN_SPI:
            //TODO call conditionally if the SPI has mutual exclusion enabled
            spiAcquireBus(mdp->iface.spip);
            spiSelect(mdp->iface.spip);

            uint8_t txbuf[length+1];
            txbuf[0] = 0x80 | ((reg & 0x3F) << 1);
            uint8_t i;
            for(i = 0; i < length; i++) {
                txbuf[i+1] = 0x80 | ((reg & 0x3F) << 1);
            }

            spiSend(mdp->iface.spip, 1, txbuf);
            spiExchange(mdp->iface.spip, length, txbuf, values);

            spiUnselect(mdp->iface.spip);
            spiReleaseBus(mdp->iface.spip);
            break;
#endif
#if MFRC522_USE_I2C
        case MFRC522_CONN_I2C:
            osalSysHalt("MFRC522: I2C not implemented!");
            break;
#endif
#if MFRC522_USE_UART
        case MFRC522_CONN_SERIAL:
            osalSysHalt("MFRC522: Serial not implemented!");
            break;
#endif
    }
}

void ext_callback(EXTDriver *extp, expchannel_t channel) {
    // TODO implement this callback.
}

static void write_register_bitmask(Mfrc522Driver *mdp, Mfrc522Register reg,
                        uint8_t bitmask, uint8_t data) {
    uint8_t reg_value = read_register(mdp, reg) & ~bitmask;
    write_register(mdp, reg, reg_value | data);
}

static void set_register_bits(Mfrc522Driver *mdp, Mfrc522Register reg,
                              uint8_t data) {
    write_register(mdp, reg, read_register(mdp, reg) | data);
}

static void clear_register_bits(Mfrc522Driver *mdp, Mfrc522Register reg,
                                uint8_t data) {
    write_register(mdp, reg, read_register(mdp, reg) & ~data);
}

static void command(Mfrc522Driver *mdp, Mfrc522Command command) {
    write_register(mdp, CommandReg, command);
}

// --- Extended functionality ---

// TODO maybe support for this should be a compile-time option?
bool mfrc522PerformSelftest(Mfrc522Driver *mdp) {
    // TODO state check
    // Soft reset
    command(mdp, Command_SoftReset);

    // Clear the internal buffer
    uint8_t i;
    for (i = 0; i < 25; i++) {
        write_register(mdp, FIFODataReg, 0x00);
    }
    command(mdp, Command_Mem);

    // Enable the self test
    write_register(mdp, AutoTestReg,
                   (AutoTestReg_SelfTestEnabled << AutoTestReg_SelfTest));

    // Write 0 to FIFO
    write_register(mdp, FIFODataReg, 0x00);

    // Initiate the self test by issuing a CalcCRC command
    command(mdp, Command_CalcCRC);

    // Be an asshole and busy-wait (TODO!)
    while ((read_register(mdp, CommandReg) & 0xF) == Command_CalcCRC);

    // Read the result
    uint8_t result[64];
    read_register_burst(mdp, FIFODataReg, result, 64);

    // Restore the config lost during reset
    mfrc522Reconfig(mdp, mdp->current_config);

    switch(read_register(mdp, VersionReg)) {
        case 0x90:
            return memcmp(result, selftest_result_ver00, 64) == 0;
        case 0x91:
            return memcmp(result, selftest_result_ver10, 64) == 0;
        case 0x92:
            return memcmp(result, selftest_result_ver20, 64) == 0;
        case 0x88:
            return memcmp(result, selftest_result_fudan, 64) == 0;
        default:
            return false;
    }
}

// --- Implementation of 'member functions' ---

#define MEMBER_FUNCTION_CHECKS(inst)                                          \
    osalDbgCheck(inst != NULL);                                               \
    osalDbgCheck(((Pcd*)inst)->data != NULL);

#define DEFINE_AND_SET_mdp(inst)                                              \
    Mfrc522Driver *mdp = (Mfrc522Driver*)((Pcd*)inst)->data;

#define CHECK_STATE(condition)                                                \
    if (condition) { return PCD_BAD_STATE; }

pcdstate_t mfrc522GetStateAB(void *inst) {
    MEMBER_FUNCTION_CHECKS(inst);
    DEFINE_AND_SET_mdp(inst);

    return mdp->state;
}

pcdresult_t mfrc522ActivateRFAB(void *inst) {
    MEMBER_FUNCTION_CHECKS(inst);
    DEFINE_AND_SET_mdp(inst);
    CHECK_STATE(mdp->state != PCD_RF_OFF);

    set_register_bits(mdp, TxControlReg,
                      (1 << TxControlReg_Tx1RFEn) | (1 << TxControlReg_Tx2RFEn));
    return PCD_OK;
}

pcdresult_t mfrc522DeactivateRFAB(void *inst) {
    MEMBER_FUNCTION_CHECKS(inst);
    DEFINE_AND_SET_mdp(inst);
    CHECK_STATE(mdp->state != PCD_READY);

    clear_register_bits(mdp, TxControlReg,
                        (1 << TxControlReg_Tx1RFEn) | (1 << TxControlReg_Tx2RFEn));
    return PCD_OK;
}

PcdSParams* mfrc522GetSupportedParamsAB(void *inst) {
    return &supported_params;
}

pcdresult_t mfrc522SetParamsAB(void *inst, pcdspeed_rx_t rx_spd,
                               pcdspeed_tx_t tx_spd, pcdmode_t mode) {
    MEMBER_FUNCTION_CHECKS(inst);
    DEFINE_AND_SET_mdp(inst);
    CHECK_STATE(mdp->state != PCD_READY && mdp->state != PCD_RF_OFF);

    uint8_t rx_speed;
    uint8_t tx_speed;

    if (mode != PCD_ISO14443_A) {
        return PCD_UNSUPPORTED;
    }

    switch(rx_spd) {
        case PCD_RX_SPEED_106:
            rx_speed = RxModeReg_RxSpeed_106;
            break;
        case PCD_RX_SPEED_212:
            rx_speed = RxModeReg_RxSpeed_212;
            break;
        case PCD_RX_SPEED_424:
            rx_speed = RxModeReg_RxSpeed_424;
            break;
        case PCD_RX_SPEED_848:
            rx_speed = RxModeReg_RxSpeed_848;
            break;
        default:
            return PCD_UNSUPPORTED;
    }

    switch(tx_spd) {
        case PCD_TX_SPEED_106:
            tx_speed = TxModeReg_TxSpeed_106;
            break;
        case PCD_TX_SPEED_212:
            tx_speed = TxModeReg_TxSpeed_212;
            break;
        case PCD_TX_SPEED_424:
            tx_speed = TxModeReg_TxSpeed_424;
            break;
        case PCD_TX_SPEED_848:
            tx_speed = TxModeReg_TxSpeed_848;
            break;
        default:
            return PCD_UNSUPPORTED;
    }

    write_register_bitmask(mdp, TxModeReg, Mask_TxModeReg_TxSpeed,
                           (tx_speed << TxModeReg_TxSpeed));
    write_register_bitmask(mdp, RxModeReg, Mask_RxModeReg_RxSpeed,
                           (rx_speed << RxModeReg_RxSpeed));

    if (tx_spd == PCD_RX_SPEED_106 && mode == PCD_ISO14443_A) {
        // Standard mandates 100% ASK for 106k speed in mode A
        set_register_bits(mdp, TxASKReg, (1 << TxAskReg_Force100ASK));
    } else {
        clear_register_bits(mdp, TxASKReg, (1 << TxAskReg_Force100ASK));
    }

    return PCD_OK;
}

pcdresult_t mfrc522TransceiveShortFrameA(void *inst, uint8_t data,
                                         uint16_t *resp_length,
                                         uint16_t timeout_us) {

}

pcdresult_t mfrc522TransceiveStandardFrameA(void *inst, uint8_t *buffer,
                                            uint16_t length,
                                            uint16_t *resp_length,
                                            uint16_t timeout_us) {

}

pcdresult_t mfrc522TransceiveAnticollFrameA(void *inst, uint8_t *buffer,
                                            uint16_t length,
                                            uint8_t n_last_bits,
                                            uint16_t *resp_length,
                                            uint16_t timeout_us) {

}

uint16_t mfrc522GetResponseLengthA(void *inst) {

}

pcdresult_t mfrc522GetResponseAB(void *inst, uint16_t buffer_size,
                                 uint8_t *buffer, uint16_t *size_copied,
                                 uint8_t *n_last_bits) {

}

void mfrc522AcquireBus(void *inst) {

}

void mfrc522ReleaseBus(void *inst) {

}

bool mfrc522SupportsExtFeature(void *inst, pcdfeature_t feature) {

}

pcdresult_t mfrc522CallExtFeature(void *inst, pcdfeature_t feature,
                                  void *params, void *result) {

}

const struct BasePcdVMT mfrc522VMT = {
    &mfrc522GetStateAB,
    &mfrc522ActivateRFAB,
    &mfrc522DeactivateRFAB,
    &mfrc522GetSupportedParamsAB,
    &mfrc522SetParamsAB,
    &mfrc522TransceiveShortFrameA,
    &mfrc522TransceiveStandardFrameA,
    &mfrc522TransceiveAnticollFrameA,
    &mfrc522GetResponseLengthA,
    &mfrc522GetResponseAB,
    &mfrc522AcquireBus,
    &mfrc522ReleaseBus,
    &mfrc522SupportsExtFeature,
    &mfrc522CallExtFeature
};

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

void mfrc522Init(void) {
    // Nothing to do.
}

#if MFRC522_USE_SPI || defined(__DOXYGEN__)
void mfrc522ObjectInitSPI(Mfrc522Driver *mdp, SPIDriver *spip) {
    mdp->connection_type = MFRC522_CONN_SPI;
    mdp->iface.spip = spip;
    mdp->state = PCD_STOP;
    mdp->pcd.vmt = &mfrc522VMT;
    mdp->pcd.data = mdp;
}
#endif

#if MFRC522_USE_I2C || defined(__DOXYGEN__)
void mfrc522ObjectInitI2C(Mfrc522Driver *mdp, I2CDriver *i2cp) {
    osalSysHalt("Not implemented!");
}
#endif

#if MFRC522_USE_UART || defined(__DOXYGEN__)
void mfrc522ObjectInitSerial(Mfrc522Driver *mdp, SerialDriver *sdp) {
    osalSysHalt("Not implemented!");
}
#endif

void mfrc522Start(Mfrc522Driver *mdp, const Mfrc522Config *config) {
    osalDbgCheck(mdp != NULL);
    osalDbgAssert(mdp->state == PCD_STOP, "Incorrect state!");
    osalDbgCheck(config != NULL);

    // Enable the MFRC522
    mdp->reset_line = config->reset_line;
    palSetLine(config->reset_line);
    osalThreadSleepMicroseconds(40);  // Oscillator start-up time

    // Interrupt handler setup
    osalDbgCheck(config->extp != NULL);
    mdp->extp = config->extp;
    mdp->interrupt_channel = config->interrupt_channel;
    extSetChannelMode(config->extp, config->interrupt_channel,
                      &interrupt_config);
    extChannelEnable(config->extp, config->interrupt_channel);

    mdp->state = PCD_RF_OFF;

    mfrc522Reconfig(mdp, config);
}

void mfrc522Reconfig(Mfrc522Driver *mdp, const Mfrc522Config *config) {
    osalDbgCheck(mdp != NULL);
    osalDbgAssert(mdp->state == PCD_RF_OFF | mdp->state == PCD_READY,
                  "Incorrect state!");
    osalDbgCheck(config != NULL);

    // TODO maybe nicer code formatting?

    write_register_bitmask(mdp, ModeReg, (ModeReg_PolMFin | Mask_ModeReg_CRCPreset),
                           (config->MFIN_polarity ? 1 : 0 << ModeReg_PolMFin) |
                           (ModeReg_CRCPreset_6363 << ModeReg_CRCPreset));

    write_register_bitmask(mdp, TxModeReg, TxModeReg_InvMod,
                           (config->inverse_modulation ? 1 : 0 << TxModeReg_InvMod));

    write_register(mdp, TxControlReg, config->tx_control_reg);

    write_register_bitmask(mdp, TxSelReg, (Mask_TxSelReg_DriverSel | Mask_TxSelReg_MFOutSel),
                           (config->driver_input_select << TxSelReg_DriverSel) |
                           (config->mfout_select << TxSelReg_MFOutSel));

    write_register_bitmask(mdp, RxSelReg, Mask_RxSelReg_UARTSel,
                           (config->cl_uart_in_sel << RxSelReg_UARTSel));

    write_register_bitmask(mdp, RxThresholdReg, (Mask_RxThresholdReg_CollLevel | Mask_RxThresholdReg_CollLevel),
                           ((config->min_rx_signal_strength & 0xF) << RxThresholdReg_MinLevel) |
                           ((config->min_rx_collision_level & 0x7) << RxThresholdReg_CollLevel));

    write_register(mdp, DemodReg, config->demod_reg);

    write_register_bitmask(mdp, RFCfgReg, Mask_RFCfgReg_RxGain,
                           (config->receiver_gain) << RFCfgReg_RxGain);

    write_register(mdp, GsNReg,
                   ((config->transmit_power_n & 0xF) << GsNReg_CWGsN) |
                   ((config->modulation_index_n & 0xF) << GsNReg_ModGsN));

    write_register_bitmask(mdp, CWGsPReg, Mask_CWGsPReg_CWGsP,
                           (config->transmit_power_p & 0x3F) << CWGsPReg_CWGsP);

    write_register_bitmask(mdp, ModGsPReg, Mask_ModGsPReg_ModGsP,
                           (config->modulation_index_p & 0x3F) << ModGsPReg_ModGsP);

    mdp->current_config = config;
}

void mfrc522Stop(Mfrc522Driver *mdp) {
    osalDbgCheck(mdp != NULL);
    osalDbgAssert(mdp->state == PCD_READY | mdp->state == PCD_RF_OFF,
                  "Incorrect state!");
    extChannelDisable(mdp->extp, mdp->interrupt_channel);
    palClearLine(mdp->reset_line);
    mdp->state = PCD_STOP;
}

#endif /* HAL_USE_MFRC522 == TRUE */
