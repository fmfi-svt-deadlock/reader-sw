#include "ch.h"
#include "hal.h"
#include "hal_custom.h"
#include "board_devices.h"
#include "main.h"


static THD_WORKING_AREA(init_thread_wa, 256);
static THD_FUNCTION(init_thread, p) {
    (void)p;

    // This is the "init" thread. It initializes high-level devices which
    // require the OS to run and spawns other threads

    devicesInit();

    uint16_t resp_len;
    pcdActivateRFAB(PCD);
    pcdTransceiveShortFrameA(PCD, 0x26, &resp_len, 100000);
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
