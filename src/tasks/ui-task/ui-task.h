/**
 * @file    tasks/ui-task/ui-task.h
 * @brief   User Interface control task
 *
 * This task controls the user interface of the Reader. User Interface of the Reader RevA board
 * consists of 2 bi-color (red/green) LEDs, labelled Status LED and Lock LED and of a small speaker.
 * This UI can use a combination of flashes and beeps to inform the user of a certain state or of
 * an event.
 *
 * The UI has 2 ways of informing the  user of something: persistent states and message flashes.
 * The persistent state informs user of some long-lasting condition (like system OK, door locked).
 * The persistent state stays the same and displayed until it is explicitly changed. The message
 * flashes are used to inform the user of one-time events that just happened (card rejected).
 * They will execute a scripted sequence and then automatically return to previous persistent
 * state.
 *
 * Example: The system starts the UI task, which switches to the default state (Error). The system
 * initializes a connection with the Controller and switches the task to "Normal - Locked" state.
 * A user attempts to open the door using an invalid card. The persistent state stays
 * "Normal - Locked" and "Card Rejected" UI flash will play. The user uses a correct card. The
 * persistent state changes to "Normal - Unlocked" and on top of that "Card Accepted" UI flash
 * will play.
 *
 * @note Deciding when to unlock and lock the door again is responsibility of the Controller, so
 *       even though the door is usually unlocked only temporarily, "Normal - Unlocked" is a
 *       persistent state from the Reader's point of view.
 *
 */

#ifndef TASKS__UI_TASK_H
#define TASKS__UI_TASK_H

/*===========================================================================*/
/* Task data structures and constants                                        */
/*===========================================================================*/

/**
 * @brief       A structure of Master Task callbacks
 *
 * User Interface of the Reader has no input elements, so this task does not
 * need to report anything else than heartbeat to the Master Task.
 *
 * @note        These callbacks must be thread safe
 */
typedef struct {
    /**
     * @brief   A heartbeat callback.
     *
     * A Heartbeat callback of the Master Task. See firmware documentation,
     * section "Reader Firmware Architecture", subsection "Watchdog"
     *
     * @param[in]  task_id  ID of this task, assigned to us by the Master Task
     */
    void (*heartbeat)(uint8_t task_id);
} dl_task_ui_callbacks;


/**
 * @brief       User interface states
 *
 * These are persistent states which the user interface may be presenting to the User. User
 * is in a given state until it is explicitly changed.
 */
typedef enum {
    DL_TASK_UI_STATE_ERROR,     /**< Error state. Status LED is blinking red. This is the default */
    DL_TASK_UI_STATE_LOCKED,    /**< Normal locked. Status LED is green, Lock led is red          */
    DL_TASK_UI_STATE_UNLOCKED   /**< Normal unlocked. Status LED is green, lock led is green      */
} dl_task_ui_state;

/**
 * @brief       User interface flashes
 *
 * These are temporary user interface states. They are usually a sequence of actions (a beep,
 * LED blink) displayed on top of the persistent state until the sequence finishes.
 *
 * Example: UI state is DL_TASK_UI_STATE_UNLOCKED, and flash DL_TASK_UI_FLASH_READ_OK is
 * invoked. This flash will beep once with a high tone and then stop. UI stays in the
 * DL_TASK_UI_STATE_UNLOCKED.
 */
typedef enum {
    DL_TASK_UI_FLASH_READ_OK,   /**< Card read and auth OK: One long high-pitched beep            */
    DL_TASK_UI_FLASH_READ_BAD,  /**< Card read and auth failed: Three short low-pitched beeps     */
    DL_TASK_UI_FLASH_VADER      /**< Vader                                                        */
} dl_task_ui_flash;

/*===========================================================================*/
/* Common task functions                                                     */
/*===========================================================================*/

/**
 * @brief       Task initializer
 *
 * This function initalizes internal data structure of the task and sets up
 * callbacks to the Master Task
 *
 * @param[in]  task_id      Our identificator the Master Task has chosen
 * @param[in]  callbacks    Structure of function pointers to callbacks this task should use
 */
void dlTaskUiInit(uint8_t task_id, dl_task_ui_callbacks *callbacks);

/**
 * @brief       Task starter
 *
 * This function starts the task thread.
 */
void dlTaskUiStart(void);

/**
 * @brief       Task stopper
 *
 * This function stops the task thread.
 */
void dlTaskUiStop(void);

/*===========================================================================*/
/* Task-specific API                                                         */
/*===========================================================================*/

/**
 * @brief       Sets the persistent UI state
 *
 * @param[in]  dl_task_ui_state     UI state to set
 *
 * @note        Thread safety: This function can be called from any thread when the ChibiOS is in
 *              Normal state.
 */
void dlTaskUiSetUIState(dl_task_ui_state state);

/**
 * @brief       Flashes a temporary user state
 *
 * @param[in]   dl_task_ui_flash    UI flash
 *
 * @note        Thread safety: This function can be called from any thread when the ChibiOS is in
 *              Normal state.
 */
void dlTaskUiFlashMessage(dl_task_ui_flash flash);



#endif
