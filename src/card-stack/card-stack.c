#include "ch.h"
#include "hal.h"

#include "card-stack.h"
#include "mfrc522.h"
#include "iso14443com.h"

THD_WORKING_AREA(waCardReader, 512);
THD_FUNCTION(CardReader, arg) {

    (void)arg;

    dlMfrc522DriverInit();
    dlMfrc522Init();

    volatile int8_t result;
    uint8_t id[10];

    while (true) {
        result = dlCardGetId(id);
        if (result == 15) break;
    }
}
