#include "stdint.h"
#include "ch.h"
#include "hal.h"

#define dlMfrc522PowerUp()   (palSetPad(GPIOA, GPIOA_RFID_RST))
#define dlMfrc522PowerDown() (palClearPad(GPIOA, GPIOA_RFID_RST))

static const SPIConfig mfrc522_spi_config = SPI_MFRC522_HAL_CONFIG;

void dlMfrc522Reset(void) {
    dlMfrc522PowerDown();
    dlMfrc522PowerUp();
}

void dlMfrc522DriverInit(void) {
    spiStart(&SPI_MFRC522, &mfrc522_spi_config);
    dlMfrc522PowerUp();
}
