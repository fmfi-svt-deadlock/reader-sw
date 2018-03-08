/**
 * @file    tasks/cardid-task/cardid-task.h
 * @brief   Card ID reading task
 *
 * This task uses on-board RFID reader module to try and read IDs of all cards present in the RF
 * field. The underlying library is able to read IDs of all cards at once (if those cards behave
 * properly).
 *
 * The master task must explicitly request that this task starts polling for cards. When this task
 * is polling for cards it may read one or more card IDs in one poll cycle. When that happens,
 * this task will invoke a callback to the Master Task and will stop polling for cards until
 * the Master Task reenables polling.
 *
 * When this task is not polling for cards the RFID reader module is in low-power mode.
 */

#ifndef TASKS__CARDID_TASK_H
#define TASKS__CARDID_TASK_H

/*===========================================================================*/
/* Task data structures and constants                                        */
/*===========================================================================*/

/**
 * @brief       Card ID structure
 *
 * This structure carries the Card ID and information about how long the ID is.
 */
typedef struct {
    uint8_t     uid[10];
    PiccUidLen  uid_len;
} dl_task_cardid_card;

/**
 * @brief       A structure of Master Task callbacks
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

    /**
     * @brief   Card Detected callback
     *
     * This callbacks informs the Master Task that one or more cards were detected in the RF field
     * and sends their IDs. After this callback is invoked the task will stop polling for cards and
     * won't change contents of parameter `cards`. However, when the Master Task requests that this
     * task should start polling for cards again, data pointed to by `cards` may change at any
     * moment!
     *
     * @param[in]   cards   An array of IDs of detected cards
     * @param[in]   len     Length of the array of detected cards
     */
    void (*card_detected)(dl_task_cardid_card *cards, uint8_t len);

    /**
     * @brief   RFID Reader Module error
     *
     * The reader module has experienced an unrecoverable error and can't function. The task will
     * automatically stop polling for cards.
     */
    void (*reader_error)(void);
} dl_task_cardid_callbacks;


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
void dlTaskCardIDInit(uint8_t task_id, dl_task_cardid_callbacks *callbacks);

/**
 * @brief       Task starter
 *
 * This function starts the task thread.
 */
void dlTaskCardIDStart(void);

/**
 * @brief       Task stopper
 *
 * This function stops the task thread.
 */
void dlTaskCardIDStop(void);


/*===========================================================================*/
/* Task-specific API                                                         */
/*===========================================================================*/

/**
 * @brief       Requests that this task starts polling for cards
 *
 * @note        Thread safety: This function can be called from any thread when the ChibiOS is in
 *              Normal state.
 */
void dlTaskCardIDStartPolling(void);

/**
 * @brief       Requests that this task stops polling for cards
 *
 * After this function is invoked, the poll that is already in progress is finished, however result
 * of that poll will be discarded. It is guaranteed that after this function returns, callback
 * `card_detected` won't be invoked until `dlTaskCardIDStartPolling` is called. If this function is
 * called during the `card_detected` callback invocation it will block until the callback returns.
 *
 * @note        Thread safety: This function can be called from any thread when the ChibiOS is in
 *              Normal state.
 */
void dlTaskCardIDStopPolling(void);

#endif
