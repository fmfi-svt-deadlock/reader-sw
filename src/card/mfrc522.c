#include "stdint.h"
#include "ch.h"
#include "hal.h"

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

void dlMfrc522Reset(void) {
    dlMfrc522PowerDown();
    dlMfrc522PowerUp();
}

void dlMfrc522DriverInit(void) {
    spiStart(&SPI_MFRC522, &mfrc522_spi_config);
    dlMfrc522PowerUp();

    uint8_t response = dlMfrc522ReadRegister(CommandReg);

    return;
}
