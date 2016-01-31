#ifndef CARD_CARD_H
#define CARD_CARD_H

#include "ch.h"

extern THD_WORKING_AREA(waCardReader, 128);
THD_FUNCTION(CardReader, arg);

#endif
