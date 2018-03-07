/**
 * @file        src/tasks/ui-task.c
 * @brief       User Interface Task implementation
 */

#include <ch.h>
#include <hal.h>
#include "ui-task.h"

/*===========================================================================*/
/* Internal data structures and constants                                    */
/*===========================================================================*/

#define LED_ACTION_NOCHANGE 0
#define LED_ACTION_SET      1
#define LED_ACTION_CLEAR    2

#define LED_SHIFT_STATUS_R  0
#define LED_SHIFT_STATUS_G  2
#define LED_SHIFT_LOCK_R    4
#define LED_SHIFT_LOCK_G    6

typedef struct {
    /* Number of 2MHz timer ticks for speaker phase change, 0 for no sound */
    uint32_t sound;
    /* LED status bitmask, 2 bits per LED */
    uint32_t leds;
} internal_ui_state;


#define END_OF_SEQUENCE     {{0, 0}, 0}

typedef struct {
    internal_ui_state   state;
    /* Number of 100ms ticks this state should be flashed, 0 for end-of-sequence */
    uint8_t             duration;
} internal_ui_seq_element;


/*== UI States and Flashes ===================================================*/

/* 880Hz beep for 1s, no change in LEDs */
internal_ui_seq_element ui_flash_read_ok[] = {
    {{1136, LED_ACTION_NOCHANGE}, 10},
    END_OF_SEQUENCE
};

/* 3 220Hz beeps for 100ms with blinking red LED */
internal_ui_seq_element ui_flash_read_fail[] = {
    {{0,    (LED_ACTION_CLEAR << LED_SHIFT_LOCK_R)}, 1},
    {{4545, (LED_ACTION_SET   << LED_SHIFT_LOCK_R)}, 1},
    {{0,    (LED_ACTION_CLEAR << LED_SHIFT_LOCK_R)}, 1},
    {{4545, (LED_ACTION_SET   << LED_SHIFT_LOCK_R)}, 1},
    {{0,    (LED_ACTION_CLEAR << LED_SHIFT_LOCK_R)}, 1},
    {{4545, (LED_ACTION_SET   << LED_SHIFT_LOCK_R)}, 1},
    {{0,    (LED_ACTION_CLEAR << LED_SHIFT_LOCK_R)}, 1},
    END_OF_SEQUENCE
};

static void buzzer_callback(GPTDriver*);
static const GPTConfig timer_config = {
    .frequency = 2000000,
    .callback  = &buzzer_callback
};


/*===========================================================================*/
/* Internal variables                                                        */
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
/* Task thread and internal functions                                        */
/*===========================================================================*/

static void buzzer_callback(GPTDriver *driver) {
    (void)driver;
    palToggleLine(LINE_AUDIO_OUT);
}


static void perform_led_action(ioline_t line, uint8_t action) {
    switch (action) {
        case LED_ACTION_SET:
            palSetLine(line);
            break;
        case LED_ACTION_CLEAR:
            palClearLine(line);
            break;
    }
}


static void set_int_ui_state(internal_ui_state state) {
    if (state.sound != 0) {
        if (GPTD14.state == GPT_CONTINUOUS) {
            gptChangeInterval(&GPTD14, state.sound);
        } else {
            gptStartContinuous(&GPTD14, state.sound);
        }
    } else {
        gptStopTimer(&GPTD14);
    }

    perform_led_action(LINE_LED_STATUS_R, ((state.leds >> LED_SHIFT_STATUS_R) & 0xFF));
    perform_led_action(LINE_LED_STATUS_G, ((state.leds >> LED_SHIFT_STATUS_G) & 0xFF));
    perform_led_action(LINE_LED_LOCK_R, ((state.leds >> LED_SHIFT_LOCK_R) & 0xFF));
    perform_led_action(LINE_LED_LOCK_G, ((state.leds >> LED_SHIFT_LOCK_G) & 0xFF));
}


static void clear_int_ui_state(void) {
    if (GPTD14.state == GPT_CONTINUOUS) {
        gptStopTimer(&GPTD14);
    }
    perform_led_action(LINE_LED_STATUS_R, LED_ACTION_CLEAR);
    perform_led_action(LINE_LED_STATUS_G, LED_ACTION_CLEAR);
    perform_led_action(LINE_LED_LOCK_R,   LED_ACTION_CLEAR);
    perform_led_action(LINE_LED_LOCK_G,   LED_ACTION_CLEAR);
}


static THD_FUNCTION(uiTask, arg) {
    (void) arg;
    
    dl_task_ui_state         state = DL_TASK_UI_STATE_ERROR;
    msg_t                    msg;
    msg_t                    fetch_status;
    internal_ui_seq_element  *current_seq = NULL;
    uint8_t                  current_seq_position = 0;
    uint8_t                  current_element_duration = 0;

    gptStart(&GPTD14, &timer_config);

    while (!chThdShouldTerminateX()) {
        fetch_status = chMBFetch(&inbox, &msg, OSAL_MS2ST(100));
        if (fetch_status == MSG_OK) {
            current_seq = NULL;
            current_seq_position = 0;
            current_element_duration = 0;

            if (UNPACK_TYPE(msg) == MSG_SET_STATE) {
                state = UNPACK_PAYLOAD(msg);
                // Clear previous LED states
                palClearLine(LINE_LED_STATUS_R);
                palClearLine(LINE_LED_STATUS_G);
                palClearLine(LINE_LED_LOCK_R);
                palClearLine(LINE_LED_LOCK_G);
                // Clear previous flash sequence
            } else if (UNPACK_TYPE(msg) == MSG_FLASH) {
                switch(UNPACK_PAYLOAD(msg)) {
                    case DL_TASK_UI_FLASH_READ_OK:
                        current_seq = ui_flash_read_ok;
                        break;
                    case DL_TASK_UI_FLASH_READ_BAD:
                        current_seq = ui_flash_read_fail;
                        break;
                }
            }
        }

        if (current_seq != NULL) {
            if ((current_seq[current_seq_position].duration != 0) &&
                (current_seq[current_seq_position].duration == current_element_duration)) {
                    current_element_duration = 0;
                    current_seq_position++;
                }
            if (current_seq[current_seq_position].duration == 0) {
                current_seq = NULL;
            } else {
                set_int_ui_state(current_seq[current_seq_position].state);
                current_element_duration++;
            }
        }

        if (current_seq == NULL) {
            clear_int_ui_state();
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
