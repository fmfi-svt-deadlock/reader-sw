#ifndef CARD_ISO14443COM_H
#define CARD_ISO14443COM_H

#include "stdint.h"

#define CARD_NOT_PRESENT -1
#define CARD_ERROR -2

int8_t dlCardGetId(uint8_t *id);

#endif
