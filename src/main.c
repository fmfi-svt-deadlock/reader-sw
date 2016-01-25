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

    // This function is now the Idle thread. It must never exit and it must implement
    // an infinite loop.
    while(true) {
        // To prevent compiler from optimizing-out the empty loop
        __asm__ __volatile__("");
    }
}
