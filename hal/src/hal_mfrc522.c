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
    mdp->state = MFRC522_STOP;
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
    osalDbgAssert(mdp->state == MFRC522_STOP, "Incorrect state!");
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

    mdp->state = MFRC522_READY;

    mfrc522Reconfig(mdp, config);
}

void mfrc522Reconfig(Mfrc522Driver *mdp, const Mfrc522Config *config) {
    osalDbgCheck(mdp != NULL);
    osalDbgAssert(mdp->state == MFRC522_READY, "Incorrect state!");
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
}

void mfrc522Stop(Mfrc522Driver *mdp) {
    osalDbgCheck(mdp != NULL);
    osalDbgAssert(mdp->state == MFRC522_READY, "Incorrect state!");
    extChannelDisable(mdp->extp, mdp->interrupt_channel);
    palClearLine(mdp->reset_line);
    mdp->state = MFRC522_STOP;
}

#endif /* HAL_USE_MFRC522 == TRUE */
