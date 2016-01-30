#include "stdint.h"
#include "ch.h"
#include "hal.h"

#include "mfrc522.h"

/*
 * Note: This may not make sense to you unless you are familiar with
 * the MFRC522 module. Please consult the MFRC522 module datasheet first.
 */

/*
 * THIS FILE IS A ONE BIG TODO!
 * 
 * This is just a C port of previously-written driver in Python for the MFRC522. It is not optimized,
 * hard to maintain, does not utilize facilities of ChibiOS, and just generally sucks. Will be rewritten ASAP.
 */

#define dlMfrc522PowerUp()   (palSetPad(GPIOA, GPIOA_RFID_RST))
#define dlMfrc522PowerDown() (palClearPad(GPIOA, GPIOA_RFID_RST))

typedef enum {
    PCD_IDLE            = 0x00,
    PCD_AUTHENT         = 0x0E,
    PCD_RECEIVE         = 0x08,
    PCD_TRANSMIT        = 0x04,
    PCD_TRANSCEIVE      = 0x0C,
    PCD_RESETPHASE      = 0x0F,
    PCD_CALCCRC         = 0x03
} Mfrc522Command;

typedef enum {
    Reserved00          = 0x00,
    CommandReg          = 0x01,
    ComIEnReg           = 0x02,
    DivlEnReg           = 0x03,
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
    Reserved34          = 0x3F
} MFRC522Register;

static const SPIConfig mfrc522_spi_config = SPI_MFRC522_HAL_CONFIG;

/*
 * WriteRegister and ReadRegister functions.
 * They send an address byte followed by either data byte or dummy byte.
 * The format of the address byte is:
 *   7 (MSB): 1 - Read. 0 - write
 *   6 - 1  : Address
 *   0      : 0
 */

#define ADDRESS_MASK 0b01111110
#define ADDRESS_READ 0b10000000

void dlMfrc522WriteRegister(MFRC522Register address, uint8_t value) {
    uint8_t const tx[] = {(address << 1) & ADDRESS_MASK, value};
    spiSelect(&SPI_MFRC522);
    spiSend(&SPI_MFRC522, sizeof(tx), tx);
    spiUnselect(&SPI_MFRC522);
}

uint8_t dlMfrc522ReadRegister(MFRC522Register address) {
    uint8_t const tx[] = {((address << 1) & ADDRESS_MASK) | ADDRESS_READ, 0x00};
    uint8_t rx[2];
    spiSelect(&SPI_MFRC522);
    spiExchange(&SPI_MFRC522, sizeof(tx), tx, rx);
    spiUnselect(&SPI_MFRC522);
    return rx[1];
}

#define dlMfrc522SetMaskInRegister(address, mask) (dlMfrc522WriteRegister((address), dlMfrc522ReadRegister((address)) | (mask)))
#define dlMfrc522ClearMaskInRegister(address, mask) (dlMfrc522WriteRegister((address), dlMfrc522ReadRegister((address)) & ~(mask)))

#define dlMfrc522AntennaOn() (dlMfrc522SetMaskInRegister(TxControlReg, 0x03))
#define dlMfrc522AntennaOff() (dlMfrc522ClearMaskInRegister(TxControlReg, 0x03))

void dlMfrc522Reset(void) {
    dlMfrc522PowerDown();
    dlMfrc522PowerUp();
}

void dlMfrc522DriverInit(void) {
    spiStart(&SPI_MFRC522, &mfrc522_spi_config);
}

void dlMfrc522Init(void) {
    // Set TAuto - timer starts automatically after the enf of transmission
    //     TPrescaler_Hi - set to 0x0D
    dlMfrc522WriteRegister(TModeReg, 0x8D);

    // Set TPrescaler_Lo - set to 0x3E.
    // Together with TPrescaler_Hi the timer will run at approx 2KHz
    dlMfrc522WriteRegister(TPrescalerReg, 0x3E);

    // Set reload value to 0x1E. The timer will run for approx 15ms.
    dlMfrc522WriteRegister(TReloadRegH, 0x00);
    dlMfrc522WriteRegister(TReloadRegL, 0x00);

    // Set Force100ASK: Forces a 100% ASK modulation independent of the
    //                  ModGSPReg setting
    dlMfrc522WriteRegister(TxASKReg, 0x40);

    // Set TXWaitRF   : Transmitter will only start if the RF field is
    //                  is present
    //     PolMFin    : MFIN pin is active HIGH
    //     CRCPreset  : to 0x6363 as specified in ISO/IEC 14443
    dlMfrc522WriteRegister(ModeReg, 0x3D);
}

#define dlMfrc522Command(command) (dlMfrc522WriteRegister(CommandReg, command))

uint16_t dlMfrc522CalculateCRCA(uint8_t *data, uint16_t n) {
    dlMfrc522Command(PCD_IDLE);

    // Flush the FIFO
    dlMfrc522SetMaskInRegister(FIFOLevelReg, 0x80);

    for (int i = 0; i < n; i++) {
        dlMfrc522WriteRegister(FIFODataReg, *(data++));
    }

    dlMfrc522Command(PCD_CALCCRC);

    while(! (dlMfrc522ReadRegister(DivIrqReg) & 0x04)) {
        ;
    }

    return (dlMfrc522ReadRegister(CRCResultRegH) << 8) | dlMfrc522ReadRegister(CRCResultRegL);
}

int16_t dlMfrc522Transceive(uint8_t *data_tx, uint16_t n_tx, uint8_t *data_rx, uint16_t n_rx) {
    // Invert output interrupt signal; enable Tx interrupt, Rx interrupt,
    // idle interrupt, FIFO Low interrupt, ErrorInterrupt and TimerInterrupt.
    dlMfrc522WriteRegister(ComIEnReg, 0xF7);
    // Clear interrupt request bits
    dlMfrc522ClearMaskInRegister(CommIrqReg, 0x80);
    // Flush the FIFO
    dlMfrc522WriteRegister(FIFOLevelReg, 0x80);

    for (uint16_t i = 0; i < n_tx; i++) {
        dlMfrc522WriteRegister(FIFODataReg, *(data_tx++));
    }

    dlMfrc522Command(PCD_TRANSCEIVE);

    uint8_t bit_framing_reg = dlMfrc522ReadRegister(BitFramingReg);
    // Set 'StartSend' bit
    dlMfrc522WriteRegister(BitFramingReg, bit_framing_reg | 0x80);

    // Busy wait for transmission finish, error or timeout
    uint8_t irq_reg;
    while (true) {
        irq_reg = dlMfrc522ReadRegister(CommIrqReg);
        // Check interrupts: Tx and Rx complete, error or timeout
        if (((irq_reg & 0x20) && (irq_reg & 0x40)) || (irq_reg & 0x03)) {
            break;
        }
    }
    
    // Restore original BitFraming register
    dlMfrc522WriteRegister(BitFramingReg, bit_framing_reg);
    
    // TODO this does not handle all cases
    if (irq_reg & 0x02) {
        uint8_t error = dlMfrc522ReadRegister(ErrorReg);
        if (error & 0x1B) {
            dlMfrc522Command(PCD_IDLE);
            return MFRC522_TRX_ERROR;
        }
    }
    
    // Timeout
    if (irq_reg & 0x01) {
        dlMfrc522Command(PCD_IDLE);
        return MFRC522_TRX_NOCARD;
    }
    
    uint8_t response_length = dlMfrc522ReadRegister(FIFOLevelReg);
    
    if (n_rx > response_length) {
        n_rx = response_length;
    }
    
    for (uint8_t i = 0; i < n_rx; i++) {
        *(data_rx++) = dlMfrc522ReadRegister(FIFODataReg);
    }
    
    dlMfrc522Command(PCD_IDLE);
    
    return n_rx;
}
