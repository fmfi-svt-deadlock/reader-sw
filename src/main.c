#include "ch.h"
#include "hal.h"

int main(void) {
    /*
     * System initializations.
     * - HAL initialization, this also initializes the configured device drivers
     *   and performs the board-specific initializations.
     * - Kernel initialization, the main() function becomes a thread and the
     *   RTOS is active.
     */

    halInit();
    chSysInit();

    while (true) {
        chThdSleepMilliseconds(500);
    }
}
