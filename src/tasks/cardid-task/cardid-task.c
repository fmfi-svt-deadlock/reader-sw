/**
 * @file        src/tasks/cardid-task.c
 * @brief       Card ID Reader Task implementation
 */

#include <string.h>
#include "ch.h"
#include "hal.h"
#include "hal_custom.h"
#include "board_devices.h"
#include "cardid-task.h"

/*===========================================================================*/
/* Internal data structures and constants                                    */
/*===========================================================================*/

#define THREAD_WORKING_AREA_SIZE 1024

/*===========================================================================*/
/* Internal variables                                                        */
/*===========================================================================*/

static uint8_t                      task_id;

static volatile bool                poll = false;
static mutex_t                      pollMutex;

static dl_task_cardid_callbacks     *callbacks;

static THD_WORKING_AREA(cardIDTaskThreadWA, THREAD_WORKING_AREA_SIZE);
static thread_t                     *task_thread;


/*===========================================================================*/
/* Task-specific API                                                         */
/*===========================================================================*/

void dlTaskCardIDStartPolling(void) {
    chMtxLock(&pollMutex);
    poll = true;
    chMtxUnlock(&pollMutex);
}

void dlTaskCardIDStopPolling(void) {
    chMtxLock(&pollMutex);
    poll = false;
    chMtxUnlock(&pollMutex);
}


/*===========================================================================*/
/* Task thread and internal functions                                        */
/*===========================================================================*/

static THD_FUNCTION(cardIDTask, arg) {
    (void) arg;
    bool readerActive = false;

    while (!chThdShouldTerminateX()) {
        // Mutex lock here is not necessary. If a race condition occurs the worst case is that the
        // RFID Reader will perform one more poll cycle.
        // If the poll changes to false during polling, it is checked again (with mutex acquired)
        // just before invoking a callback.
        bool pollThisCycle = poll;

        // Activate / deactivate reader
        if (pollThisCycle != readerActive) {
            pcdresult_t result;
            if (pollThisCycle) {
                result = pcdActivateRFAB(PCD);
            } else {
                result = pcdDeactivateRFAB(PCD);
            }

            if (result != PCD_OK) {
                chMtxLock(&pollMutex);
                callbacks->reader_error();
                poll = false;
                readerActive = false;
                // The module is in unknown state, best to try and reset it so that we know that
                // RF field is inactive.
                resetRFIDModule();
                chMtxUnlock(&pollMutex);
            } else {
                readerActive = pollThisCycle;
            }
        }

        if (pollThisCycle) {
            unsigned int result_p = 0;
            Picc cards[10];
            bool is_that_all;
            result_p = iso14443FindCards(PCD, cards, 10, &is_that_all);

            if (result_p > 0) {
                chMtxLock(&pollMutex);
                // Checking intentionally the real `poll` state instead of pollThisCycle
                if (poll) {
                    dl_task_cardid_card sanitized_cards[result_p];
                    for (unsigned int i = 0; i < result_p; i++) {
                        memcpy(sanitized_cards[i].uid, cards[i].uid, 10);
                        sanitized_cards[i].uid_len = cards[i].uid_len;
                    }
                    callbacks->card_detected(sanitized_cards, result_p);
                }
                poll = false;
                chMtxUnlock(&pollMutex);
            }
        } else {
            chThdSleepMilliseconds(100);
        }

        //TODO generate heartbeat
    }

}


/*===========================================================================*/
/* Common task API                                                           */
/*===========================================================================*/

void dlTaskCardIDInit(uint8_t _task_id, dl_task_cardid_callbacks *_callbacks) {
    task_id   = _task_id;
    callbacks = _callbacks;

    chMtxObjectInit(&pollMutex);
}

void dlTaskCardIDStart(void) {
    task_thread = chThdCreateStatic(cardIDTaskThreadWA, sizeof(cardIDTaskThreadWA),
                                    LOWPRIO, cardIDTask, NULL);
}

void dlTaskCardIDStop(void) {
    chThdTerminate(task_thread);
    chMtxUnlockAll();
}
