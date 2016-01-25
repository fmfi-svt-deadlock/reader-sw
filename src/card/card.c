#include "ch.h"
#include "hal.h"

#include "card.h"
#include "mfrc522.h"

THD_WORKING_AREA(waCardReader, 128);
THD_FUNCTION(CardReader, arg) {

    (void)arg;

    dlMfrc522DriverInit();
}
