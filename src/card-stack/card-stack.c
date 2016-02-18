#include "ch.h"
#include "hal.h"

#include "card-stack.h"

THD_WORKING_AREA(waCardReader, 512);
THD_FUNCTION(CardReader, arg) {

    (void)arg;

    // dlMfrc522DriverInit();
    // dlMfrc522Init();

    // volatile int8_t result;
    // uint8_t id[10];

    // while (true) {
    //     if (dlCardIsPresent()) {
    //         palSetPad(GPIOA, GPIOA_LED_G2);
    //         result = dlCardGetId(id);
    //         if (result > 0) {
    //             palSetPad(GPIOB, GPIOB_LED_G1);
    //         }
    //     }
    //     chThdSleepMilliseconds(1000);
    //     palClearPad(GPIOA, GPIOA_LED_G2);
    //     palClearPad(GPIOB, GPIOB_LED_G1);
    // }
}
