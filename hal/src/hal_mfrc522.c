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

}

void mfrc522Stop(Mfrc522Driver *mdp) {
    osalDbgCheck(mdp != NULL);
    osalDbgAssert(mdp->state == MFRC522_READY, "Incorrect state!");
    extChannelDisable(mdp->extp, mdp->interrupt_channel);
    palClearLine(mdp->reset_line);
    mdp->state = MFRC522_STOP;
}

#endif /* HAL_USE_MFRC522 == TRUE */
