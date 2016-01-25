#include "stdint.h"
#include "ch.h"
#include "hal.h"

static SPIConfig mfrc522_spi_config = SPI_MFRC522_HAL_CONFIG;

void dlMfrc522DriverInit(void) {
    spiStart(&SPI_MFRC522, &mfrc522_spi_config);
}
