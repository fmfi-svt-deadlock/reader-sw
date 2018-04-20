/**
 * @file    src/tasks/master/master-task.c
 * @brief   Master task implementation
 */

#include <string.h>
#include "ch.h"
#include "hal.h"
#include "board_devices.h"

#include "tasks/ui-task/ui-task.h"
#include "tasks/cardid-task/cardid-task.h"
#include "tasks/comm-task/comm-task.h"
#include "master-task.h"


/*===========================================================================*/
/* Internal defines                                                          */
/*===========================================================================*/

#define TASK_ID_UI              0
#define TASK_ID_CARDID          1
#define TASK_ID_COMM_CONTROL    2
#define TASK_ID_COMM_RECV       3
#define WATCHDOGED_TASKS        0xF // All 4 tasks

#define INBOX_SIZE              10

#define THREAD_WORKING_AREA_SIZE  1024


/*===========================================================================*/
/* Internal data structures and enums                                        */
/*===========================================================================*/

typedef enum {
    STATE_DISCONNECTED,
    STATE_INACTIVE,
    STATE_ACTIVE
} master_task_state;

typedef enum {
    MT_MSG_CARD_DETECTED,
    MT_MSG_READER_ERROR,
    MT_MSG_LINK_CHANGE,
    MT_MSG_COMM_SYS_QUERY_REQ,
    MT_MSG_COMM_ACTIVATE_AM0,
    MT_MSG_COMM_DEACTIVATE_AM0,
    MT_MSG_COMM_UIUPDATE
} master_task_msg_type;

typedef struct {
    master_task_msg_type t;
    union master_msg_union {
        struct picc_uids {
            dl_picc_uid  card[DL_MAX_CARDS_PER_POLL_CYCLE];
            size_t       n;
        } detected_cards;
        dl_task_comm_linkstate      new_link_state;
        DeadcomCRPMUIClass0States   new_ui_state;
    } payload;
} master_task_msg;


/*===========================================================================*/
/* Internal variable and function prototypes                                 */
/*===========================================================================*/

// Task callbacks
void cb_task_heartbeat(uint8_t task_id);

void cb_task_cardid_cardDetected(dl_picc_uid *cards, uint8_t len);
void cb_task_cardid_readerError(void);

void cb_task_comm_linkChange(dl_task_comm_linkstate new_link_state);
void cb_task_comm_sysQueryRequest(void);
void cb_task_comm_activateAuthMethods(DeadcomCRPMAuthMethod *methods, size_t methods_len);
void cb_task_comm_uiUpdate(DeadcomCRPMUIClass0States uistate);

// Dest should be at least 8 bytes long. No '\0' char will be added.
void numToHex(uint32_t num, char *dest);

/*===========================================================================*/
/* Internal variables and constants                                          */
/*===========================================================================*/

static mailbox_t inbox;
static msg_t     msg_buffer[INBOX_SIZE];

static master_task_msg        msg_objs[INBOX_SIZE];
static guarded_memory_pool_t  msg_pool;

dl_task_ui_callbacks uiCallbacks = {
    .heartbeat = cb_task_heartbeat
};

dl_task_cardid_callbacks cardIDCallbacks = {
    .heartbeat = cb_task_heartbeat,
    .card_detected = cb_task_cardid_cardDetected,
    .reader_error = cb_task_cardid_readerError
};

dl_task_comm_callbacks commCallbacks = {
    .heartbeat = cb_task_heartbeat,
    .linkChange = cb_task_comm_linkChange,
    .rcvdSystemQueryRequest = cb_task_comm_sysQueryRequest,
    .rcvdActivateAuthMethods = cb_task_comm_activateAuthMethods,
    .rcvdUiUpdate = cb_task_comm_uiUpdate
};

static THD_WORKING_AREA(masterTaskWA, THREAD_WORKING_AREA_SIZE);
static thread_t *master_task_thread;

static master_task_state state = STATE_DISCONNECTED;

volatile uint8_t heartbeat_vector;

/*===========================================================================*/
/* Master Task thread and internal functions                                 */
/*===========================================================================*/

static THD_FUNCTION(masterTask, arg) {
    (void) arg;

    // Initialize board-level devices.
    devicesInit();
    chThdSleepMilliseconds(1000);

    wdgStart(&WDGD1, &wdgcfg);

    dlTaskUiStart();
    dlTaskCardIDStart();
    dlTaskCommStart();

    while(true) {
        master_task_msg *msg = NULL;
        msg_t fetch_status = chMBFetch(&inbox, (msg_t*)&msg, OSAL_MS2ST(100));
        if (fetch_status != MSG_OK) {goto heartbeat;}

        if (state == STATE_DISCONNECTED) {
            if (msg->t == MT_MSG_LINK_CHANGE &&
                msg->payload.new_link_state == DL_TASK_COMM_LINKUP) {
                state = STATE_INACTIVE;
            }
        } else {
            switch(msg->t) {
                case MT_MSG_CARD_DETECTED:
                    if (state == STATE_INACTIVE) {break;}
                    dlTaskCommSendAM0GotUids(msg->payload.detected_cards.card,
                                             msg->payload.detected_cards.n);
                    dlTaskCardIDStartPolling();
                    break;
                case MT_MSG_READER_ERROR:
                    dlTaskCommSendRdrFailure("Reader module failed!");
                    dlTaskCardIDStopPolling();
                    dlTaskUiSetUIState(DL_TASK_UI_STATE_ERROR);
                    state = STATE_INACTIVE;
                    break;
                case MT_MSG_LINK_CHANGE:
                    if (msg->payload.new_link_state == DL_TASK_COMM_LINKDOWN) {
                        dlTaskCardIDStopPolling();
                        dlTaskUiSetUIState(DL_TASK_UI_STATE_ERROR);
                        state = STATE_DISCONNECTED;
                    }
                    break;
                case MT_MSG_COMM_SYS_QUERY_REQ: {
                    char sn[26] = {'\0'};
                    uint32_t *uid = (uint32_t*)UID_BASE;
                    numToHex(uid[0], sn);
                    numToHex(uid[1], sn+8);
                    numToHex(uid[2], sn+16);
                    sn[24] = 'R'; // Filler constant
                    dlTaskCommSendSysQueryResp(READER_CLASS, BOARD_HW_MODEL, BOARD_HW_REV, sn,
                                               READER_SW_VER_MAJOR, READER_SW_VER_MINOR);
                    break;
                }
                case MT_MSG_COMM_ACTIVATE_AM0:
                    if (state == STATE_ACTIVE) {break;}
                    dlTaskCardIDStartPolling();
                    state = STATE_ACTIVE;
                    break;
                case MT_MSG_COMM_DEACTIVATE_AM0:
                    if (state == STATE_INACTIVE) {break;}
                    dlTaskCardIDStopPolling();
                    state = STATE_INACTIVE;
                    break;
                case MT_MSG_COMM_UIUPDATE:
                    switch(msg->payload.new_ui_state) {
                        case DCRCP_CRPM_UIC0_DOOR_CLOSED:
                            dlTaskUiSetUIState(DL_TASK_UI_STATE_LOCKED);
                            break;
                        case DCRCP_CRPM_UIC0_ID_ACCEPTED_DOOR_UNLOCKED:
                            dlTaskUiSetUIState(DL_TASK_UI_STATE_UNLOCKED);
                            dlTaskUiFlashMessage(DL_TASK_UI_FLASH_READ_OK);
                            break;
                        case DCRCP_CRPM_UIC0_ID_REJECTED:
                            dlTaskUiFlashMessage(DL_TASK_UI_FLASH_READ_BAD);
                            break;
                        case DCRCP_CRPM_UIC0_DOOR_PERMANENTLY_UNLOCKED:
                            dlTaskUiSetUIState(DL_TASK_UI_STATE_UNLOCKED);
                            break;
                        case DCRCP_CRPM_UIC0_DOOR_PERMANENTLY_LOCKED:
                            // TODO maybe another UI state should be added for this?
                            dlTaskUiSetUIState(DL_TASK_UI_STATE_LOCKED);
                            break;
                        case DCRCP_CRPM_UIC0_SYSTEM_FAILURE:
                            dlTaskUiSetUIState(DL_TASK_UI_STATE_ERROR);
                            break;
                        case DCRCP_CRPM_UIC0_DOOR_OPEN_TOO_LONG:
                            // TODO there is no good representation of this state
                            break;
                    }
                    break;
            }
        }

        chGuardedPoolFree(&msg_pool, msg);

        heartbeat:
        if ((heartbeat_vector & WATCHDOGED_TASKS) == WATCHDOGED_TASKS) {
            wdgReset(&WDGD1);
            heartbeat_vector = 0;
        }
    }
}


void numToHex(uint32_t num, char *dest) {
    for (uint8_t i = 0; i < sizeof(num)*2; i++) {
        *dest = "0123456789ABCDEF"[num & 0xF];
        num >>= 4;
        dest++;
    }
}

/*===========================================================================*/
/* Task callbacks implementation                                             */
/*===========================================================================*/

void cb_task_heartbeat(uint8_t task_id) {
    heartbeat_vector |= (1 << task_id);
}

void cb_task_cardid_cardDetected(dl_picc_uid *cards, uint8_t len) {
    master_task_msg *m = chGuardedPoolAllocTimeout(&msg_pool, TIME_INFINITE);
    if (m == NULL) {
        chSysHalt("null from inifinitely waiting guarded pool");
        return;
    }
    m->t = MT_MSG_CARD_DETECTED;
    m->payload.detected_cards.n = len;
    for (uint8_t i = 0; i < len; i++) {
        memcpy(&(m->payload.detected_cards.card[i]), &(cards[i]), sizeof(dl_picc_uid));
    }
    (void)chMBPost(&inbox, (msg_t)m, TIME_INFINITE);
}

void cb_task_cardid_readerError(void) {
    master_task_msg *m = chGuardedPoolAllocTimeout(&msg_pool, TIME_INFINITE);
    if (m == NULL) {
        chSysHalt("null from inifinitely waiting guarded pool");
        return;
    }
    m->t = MT_MSG_READER_ERROR;
    (void)chMBPost(&inbox, (msg_t)m, TIME_INFINITE);
}

void cb_task_comm_linkChange(dl_task_comm_linkstate new_link_state) {
    master_task_msg *m = chGuardedPoolAllocTimeout(&msg_pool, TIME_INFINITE);
    if (m == NULL) {
        chSysHalt("null from inifinitely waiting guarded pool");
        return;
    }
    m->t = MT_MSG_LINK_CHANGE;
    m->payload.new_link_state = new_link_state;
    (void)chMBPost(&inbox, (msg_t)m, TIME_INFINITE);
}

void cb_task_comm_sysQueryRequest(void) {
    master_task_msg *m = chGuardedPoolAllocTimeout(&msg_pool, TIME_INFINITE);
    if (m == NULL) {
        chSysHalt("null from inifinitely waiting guarded pool");
        return;
    }
    m->t = MT_MSG_COMM_SYS_QUERY_REQ;
    (void)chMBPost(&inbox, (msg_t)m, TIME_INFINITE);
}

void cb_task_comm_activateAuthMethods(DeadcomCRPMAuthMethod *methods, size_t methods_len) {
    master_task_msg *m = chGuardedPoolAllocTimeout(&msg_pool, TIME_INFINITE);
    if (m == NULL) {
        chSysHalt("null from inifinitely waiting guarded pool");
        return;
    }
    m->t = MT_MSG_COMM_DEACTIVATE_AM0;
    for (size_t i = 0; i < methods_len; i++) {
        if (methods[i] == DCRCP_CRPM_AM_PICC_UUID) {
            m->t = MT_MSG_COMM_ACTIVATE_AM0;
            break;
        }
    }
    (void)chMBPost(&inbox, (msg_t)m, TIME_INFINITE);
}

void cb_task_comm_uiUpdate(DeadcomCRPMUIClass0States uistate) {
    master_task_msg *m = chGuardedPoolAllocTimeout(&msg_pool, TIME_INFINITE);
    if (m == NULL) {
        chSysHalt("null from inifinitely waiting guarded pool");
        return;
    }
    m->t = MT_MSG_COMM_UIUPDATE;
    m->payload.new_ui_state = uistate;
    (void)chMBPost(&inbox, (msg_t)m, TIME_INFINITE);
}


/*===========================================================================*/
/* Common task API                                                           */
/*===========================================================================*/

void dlTaskMasterInit(void) {
    chMBObjectInit(&inbox, msg_buffer, INBOX_SIZE);
    chGuardedPoolObjectInit(&msg_pool, sizeof(master_task_msg));
    chGuardedPoolLoadArray(&msg_pool, msg_objs, INBOX_SIZE);

    dlTaskUiInit(TASK_ID_UI, &uiCallbacks);
    dlTaskCardIDInit(TASK_ID_CARDID, &cardIDCallbacks);
    dlTaskCommInit(TASK_ID_COMM_CONTROL, TASK_ID_COMM_RECV, &commCallbacks);
}

void dlTaskMasterStart(void) {
    master_task_thread = chThdCreateStatic(masterTaskWA, sizeof(masterTaskWA),
                                           HIGHPRIO, masterTask, NULL);
}
