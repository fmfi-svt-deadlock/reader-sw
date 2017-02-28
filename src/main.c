#include "ch.h"
#include "hal.h"
#include "hal_custom.h"
#include "board_devices.h"
#include "main.h"

<<<<<<< HEAD
static THD_WORKING_AREA(init_thread_wa, 512);
=======
static THD_WORKING_AREA(init_thread_wa, 1024);
>>>>>>> 6b9606b... [docs] Describe the Reader <-> Controller protocol
static THD_FUNCTION(init_thread, p) {
    (void)p;

    // This is the "init" thread. It initializes high-level devices which
    // require the OS to run and spawns other threads

    devicesInit();

    pcdresult_t result_r;
    result_r = pcdActivateRFAB(PCD);

<<<<<<< HEAD
    iso14443result_t result_p;
    Picc cards[10];
    bool is_that_all;
    result_p = iso14443FindCards(PCD, cards, 10, &is_that_all);

=======
    while(true) {
        Picc cards[10];
        bool is_that_all;
        unsigned int n_found_cards = iso14443FindCards(PCD, cards, 10, &is_that_all);

        if (n_found_cards > 0) {
            palSetPad(GPIOB, GPIOB_LED_G1);
        } else {
            palClearPad(GPIOB, GPIOB_LED_G1);
        }
    }

    uint8_t x = 0;
>>>>>>> 6b9606b... [docs] Describe the Reader <-> Controller protocol
    // result = pcdTransceiveShortFrameA(PCD, 0x26, &resp_len, 100000);
    // uint8_t anticoll[] = {0x93, 0x20};
    // result = pcdTransceiveAnticollFrameA(PCD, anticoll, 2, 0, 0, &resp_len, 100000);
    // uint8_t resp[20];
    // uint8_t last_bits;
    // result = pcdGetRespAB(PCD, 20, resp, &resp_len, &last_bits);
    // // Hardcoded anticoll
    // uint8_t anticoll2[] = {0x93, 0x23, 0x06};
    // result = pcdTransceiveAnticollFrameA(PCD, anticoll2, 3, 3, 3, &resp_len, 100000);
    // result = pcdGetRespAB(PCD, 20, resp, &resp_len, &last_bits);
}

int main(void) {

    /*
     * System initializations.
     * - HAL initialization, this also initializes the configured device drivers
     *   and performs the board-specific initializations.
     * - Kernel initialization, the main() function becomes a thread and the
     *   RTOS is active.
     */

    halInit();
    halCustomInit();
    chSysInit();

    // Start the "init" thread
    chThdCreateStatic(init_thread_wa, sizeof(init_thread_wa),
                      NORMALPRIO + 1, init_thread, NULL);

    // This function is now the Idle thread. It must never exit and it must implement
    // an infinite loop.
    while(true) {
        // To prevent compiler from optimizing-out the empty loop
        // TODO implement switching MCU to a low-power mode.
        __asm__ __volatile__("");
    }
}
