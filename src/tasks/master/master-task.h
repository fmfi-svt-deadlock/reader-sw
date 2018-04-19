/**
 * @file    tasks/master/master-task.h
 * @brief   Master task
 *
 * This is the Master task. This tasks starts and monitors other tasks, takes care of hardware
 * watchdog, and most importantly, it implements business logic of the Reader.
 *
 *
 * High-level functional description
 * ---------------------------------
 *
 * Bootup to Disconnected mode:
 * After the Master task is started, it should initialize and start a watchdog timer. After that,
 * it should start a UI Task, and set UI state to DL_TASK_UI_STATE_ERROR. It should then start
 * the CardID Task and CommTask. Watchdog is now running and being refreshed, and the Reader is
 * considered to be in the Disconnected mode.
 *
 * Operations in Disconnected mode: The master task is waiting until CommTask signals that the
 * link was established. After that, the Reader transitions to Inactive mode.
 *
 * Operations in Inactive mode: The master task is waiting for instructions from the CommTask.
 * If the link drops, UI state transitions to DL_TASK_UI_STATE_ERROR and the Reader will be
 * considered to be in the Disconnected mode again. If System Query request arrives, an appropriate
 * response will be sent. If UI Update request arrives the UI will be updated accordingly.
 * If Activate Auth Method 0 arrives, the Reader signals CardID task to start polling for cards and
 * transitions to Active mode.
 *
 * Operations in Active mode: The Reader is polling for cards. Handling of System Query and
 * UI Update requests from CommTask is the same as in Inactive mode. If Activate Auth Method
 * request arrives with no auth methods, the Reader signals CardID task to stop polling for cards
 * and transitions to Inactive mode. If the link drops, the reader signals CardID task to stop
 * polling for cards, UI state transitions to DL_TASK_UI_STATE_ERROR and the Reader will be
 * considered in Disconnected mode.
 * If a card (or cards) are detected by the CardID task, the Reader will send
 * Auth Method 0: Got Uids frame.
 */

#ifndef TASKS__MASTER_H
#define TASKS__MASTER_H

/*===========================================================================*/
/* Common task functions                                                     */
/*===========================================================================*/

/**
 * @brief       Master task initializer
 *
 * This function initializes the Master Task
 */
void dlTaskMasterInit(void);

/**
 * @brief       Master task starter
 *
 * This function starts the Master task
 */
void dlTaskMasterStart(void);

// The master task has no stop function.

#endif
