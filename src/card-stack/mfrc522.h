#ifndef CARD_MFRC522_H
#define CARD_MFRC522_H

#include "stdint.h"

void dlMfrc522DriverInit(void);
void dlMfrc522Reset(void);
void dlMfrc522Init(void);
int16_t dlMfrc522Transceive(uint8_t *data_tx, uint16_t n_tx, uint8_t *data_rx, uint16_t n_rx);

#define MFRC522_TRX_ERROR -1
#define MFRC522_TRX_NOCARD -2

#endif
