/**
 * @file    tasks/comm-task/comm-task.h
 * @brief   Communication handling task
 *
 * This task handles serial port communication with Controller.
 *
 * An intent to send something is delivered as ChibiOS Message. This message is then serialized
 * to CBOR format according to `dcrcp` (DeadCom Reader<->Controller Protocol) schema. The resulting
 * byte buffer is then packed to `dcl2` (DeadCom Layer 2) frame and transmitted over RS232 link.
 *
 * This task handles sending and receiving `dcrcp` messages, and `dcl2` link management.
 *
 */

#ifndef TASKS__COMM_TASK_H
#define TASKS__COMM_TASK_H

#include "common.h"
#include "dcrcp.h"

/*===========================================================================*/
/* Task data structures and constants                                        */
/*===========================================================================*/

typedef enum {
    DL_TASK_COMM_LINKUP,
    DL_TASK_COMM_LINKDOWN
} dl_task_comm_linkstate;

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
     * @brief   A link status has changes.
     *
     * Either the link was established dropped
     */
    void (*linkChange)(dl_task_comm_linkstate new_link_state);

    /**
     * @brief  A System Query Request CRPM has been received
     */
    void (*rcvdSystemQueryRequest)(void);

    /**
     * @brief  An Activate Auth Methods CRPM was received
     */
    void (*rcvdActivateAuthMethods)(DeadcomCRPMAuthMethod *methods, size_t methods_len);

    /**
     * @brief  UI Update CRPM was received
     */
    void (*rcvdUiUpdate)(DeadcomCRPMUIClass0States uistate);
} dl_task_comm_callbacks;

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
void dlTaskCommInit(uint8_t ctrl_task_id, uint8_t rcv_task_id, dl_task_comm_callbacks *callbacks);

/**
 * @brief       Task starter
 *
 * This function starts the task thread.
 */
void dlTaskCommStart(void);

/**
 * @brief       Task stopper
 *
 * This function stops the task thread.
 */
void dlTaskCommStop(void);

/*===========================================================================*/
/* Task-specific API                                                         */
/*===========================================================================*/

/**
 * @brief  Send System Query Response CRPM
 */
void dlTaskCommSendSysQueryResp(uint16_t rdrClass, uint16_t hwModel, uint16_t hwRev,
                                char serial[25], uint8_t swVerMajor, uint8_t swVerMinor);

/**
 * @brief  Send Reader Failure CRPM
 */
void dlTaskCommSendRdrFailure(char *str);

/**
 * @brief  Send Auth method: PICC UID obtained CRPM
 */
void dlTaskCommSendAM0GotUuids(dl_picc_uid *uuids, size_t uuids_len);

#endif
