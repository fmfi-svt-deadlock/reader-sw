#include "ch.h"
#include "hal.h"
#include "hal_custom.h"
#include "board_devices.h"
#include "main.h"

#include "tasks/master/master-task.h"


int main(void) {
    /*
     * System initialization and startup.
     * - HAL initialization, this also initializes the configured device drivers
     *   and performs board-specific initializations.
     * - Kernel initialization, the main() function becomes a thread and the
     *   RTOS is active.
     *
     * - Master task initialization and startup (which also performs final device-level
     *   initialization)
     */
    halInit();
    halCustomInit();
    chSysInit();

    dlTaskMasterInit();
    dlTaskMasterStart();


    /*
     * Idle thread loop
     *
     * This function is now the Idle thread. It must never exit and it must implement
     * an infinite loop.
     */
    while(true) {
        // To prevent compiler from optimizing-out the empty loop
        // TODO implement switching MCU to a low-power mode.
        __asm__ __volatile__("");
    }
}
