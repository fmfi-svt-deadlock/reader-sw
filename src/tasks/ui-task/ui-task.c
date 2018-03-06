/**
 * @file        src/tasks/ui-task.c
 * @brief       User Interface Task implementation
 */

#include <ch.h>
#include <hal.h>
#include "ui-task.h"

/*===========================================================================*/
/* Internal variables and data structures                                    */
/*===========================================================================*/

#define MAX_INBOX_MESSAGES 5
#define THREAD_WORKING_AREA_SIZE 128

static uint8_t                  task_id;

static mailbox_t                inbox;
static msg_t                    msg_buffer[MAX_INBOX_MESSAGES];

static dl_task_ui_callbacks     *callbacks;

static THD_WORKING_AREA(uiTaskThreadWA, THREAD_WORKING_AREA_SIZE);
static thread_t                 *task_thread;


/*===========================================================================*/
/* Task-specific API                                                         */
/*===========================================================================*/

/* Packs a message to msg_t. msg_t is guaranteed to be able to hold pointer on the target
 * architecture, which is 32 bits on MCUs we are using.
 */
#define PACK_MESSAGE(msg_type, msg_payload)     ((msg_t)((msg_type << 16) | msg_payload))
#define UNPACK_TYPE(msg)                        ((msg & 0xFFFF0000) >> 16)
#define UNPACK_PAYLOAD(msg)                     (msg & 0xFFFF)
#define MSG_SET_STATE                           1
#define MSG_FLASH                               2

void dlTaskUiSetUIState(dl_task_ui_state state) {
    // TODO check whether the task is initialized
    (void)chMBPost(&inbox, PACK_MESSAGE(MSG_SET_STATE, state), TIME_INFINITE);
}

void dlTaskUiFlashMessage(dl_task_ui_flash flash) {
    // TODO check whether the task is initialized
    (void)chMBPost(&inbox, PACK_MESSAGE(MSG_FLASH, flash), TIME_INFINITE);
}


/*===========================================================================*/
/* Task thread                                                               */
/*===========================================================================*/

static THD_FUNCTION(uiTask, arg) {
    (void) arg;
    dl_task_ui_state state = DL_TASK_UI_STATE_ERROR;
    msg_t msg;
    msg_t fetch_status;

    while (!chThdShouldTerminateX()) {
        fetch_status = chMBFetch(&inbox, &msg, OSAL_MS2ST(150));
        if (fetch_status == MSG_OK /*&& UNPACK_TYPE(msg) == MSG_SET_STATE*/) {
            state = UNPACK_PAYLOAD(msg);
            // Clear previous LED states
            palClearLine(LINE_LED_STATUS_R);
            palClearLine(LINE_LED_STATUS_G);
            palClearLine(LINE_LED_LOCK_R);
            palClearLine(LINE_LED_LOCK_G);
        }

        switch(state) {
            case DL_TASK_UI_STATE_ERROR:
                palToggleLine(LINE_LED_STATUS_R);
                break;
            case DL_TASK_UI_STATE_LOCKED:
                palSetLine(LINE_LED_STATUS_G);
                palSetLine(LINE_LED_LOCK_R);
                break;
            case DL_TASK_UI_STATE_UNLOCKED:
                palSetLine(LINE_LED_STATUS_G);
                palSetLine(LINE_LED_LOCK_G);
                break;
        }
    }
}


/*===========================================================================*/
/* Common task API                                                           */
/*===========================================================================*/

void dlTaskUiInit(uint8_t _task_id, dl_task_ui_callbacks *_callbacks) {
    task_id   = _task_id;
    callbacks = _callbacks;

    chMBObjectInit(&inbox, msg_buffer, MAX_INBOX_MESSAGES);
}

void dlTaskUiStart(void) {
    task_thread = chThdCreateStatic(uiTaskThreadWA, sizeof(uiTaskThreadWA),
                                    NORMALPRIO, uiTask, NULL);
}

void dlTaskUiStop(void) {
    chThdTerminate(task_thread);
    chMBReset(&inbox);
}
