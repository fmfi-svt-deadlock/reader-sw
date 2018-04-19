/**
 * @file        src/tasks/comm-task/comm-task.c
 * @brief       Card ID Reader Task implementation
 */

#include <string.h>
#include "ch.h"
#include "hal.h"
#include "comm-task.h"
#include "dcl2.h"
#include "cn-cbor/cn-cbor.h"

/*===========================================================================*/
/* Internal data structures                                                  */
/*===========================================================================*/

typedef struct {
    mutex_t m;
    condition_variable_t c;
} dl_task_comm_tvars_t;


/*===========================================================================*/
/* Internal variable and  function prototypes                                */
/*===========================================================================*/

static cn_cbor* allocate_cncbor(void *context);
static void free_cncbor(cn_cbor *p, void *context);

static bool dclTransmitBytes(const uint8_t *buf, size_t buf_size, void* context);
static bool dclMtxObjectInit(dl_task_comm_tvars_t *tvars);
static bool dclMtxLock(dl_task_comm_tvars_t *tvars);
static bool dclMtxUnlock(dl_task_comm_tvars_t *tvars);
static bool dclCondObjectInit(dl_task_comm_tvars_t *tvars);
static bool dclCondWait(dl_task_comm_tvars_t *tvars, uint32_t milliseconds, bool *timed_out);
static bool dclCondSignal(dl_task_comm_tvars_t *tvars);


/*===========================================================================*/
/* Internal variables and constants                                          */
/*===========================================================================*/

#define THREAD_WORKING_AREA_SIZE_MASTER 1024
#define THREAD_WORKING_AREA_SIZE_RECEIVE_HANDLER 2048

#define OUT_QUEUE_LENGTH 5

static const SerialConfig sd2_config = SD2_CONFIG;
static const DeadcomL2ThreadingMethods dcl2_t_methods = {
    .mutexInit     = (bool (*)(void*))&dclMtxObjectInit,
    .mutexLock     = (bool (*)(void*))&dclMtxLock,
    .mutexUnlock   = (bool (*)(void*))&dclMtxUnlock,
    .condvarInit   = (bool (*)(void*))&dclCondObjectInit,
    .condvarWait   = (bool (*)(void*, uint32_t, bool*))&dclCondWait,
    .condvarSignal = (bool (*)(void*))&dclCondSignal
};

static dl_task_comm_tvars_t dl_task_comm_tvars;

static dl_task_comm_callbacks  *callbacks;

static THD_WORKING_AREA(commTaskControlThreadWA, THREAD_WORKING_AREA_SIZE_MASTER);
static THD_WORKING_AREA(commTaskReceiveThreadWA, THREAD_WORKING_AREA_SIZE_RECEIVE_HANDLER);
static thread_t  *task_control_thread;
static thread_t  *task_receive_thread;

uint8_t  ctrl_task_id, rcv_task_id;

static DeadcomL2  dc_link;

static DeadcomCRPM           out_objs[OUT_QUEUE_LENGTH];
static guarded_memory_pool_t out_pool;
static mailbox_t             outbox;
static msg_t                 outbox_buffer[OUT_QUEUE_LENGTH];

static cn_cbor        cbors_out[DCRCP_REQUIRED_CNCBOR_BUFFERS];
static memory_pool_t  cbors_out_pool;
static cn_cbor        cbors_in[DCRCP_REQUIRED_CNCBOR_BUFFERS];
static memory_pool_t  cbors_in_pool;

static cn_cbor_context cbor_out_context = {
    .calloc_func = allocate_cncbor,
    .free_func   = free_cncbor,
    .context     = &cbors_out_pool
};

static cn_cbor_context cbor_in_context = {
    .calloc_func = allocate_cncbor,
    .free_func   = free_cncbor,
    .context     = &cbors_in_pool
};

volatile dl_task_comm_linkstate last_link_state = DL_TASK_COMM_LINKDOWN;


/*===========================================================================*/
/* Task-specific API                                                         */
/*===========================================================================*/

void dlTaskCommSendSysQueryResp(uint16_t rdrClass, uint16_t hwModel, uint16_t hwRev,
                                char serial[25], uint8_t swVerMajor, uint8_t swVerMinor) {
    DeadcomCRPM *m = chGuardedPoolAllocTimeout(&out_pool, TIME_INFINITE);
    if (m == NULL) {
        chSysHalt("null from inifinitely waiting guarded pool");
        return;
    }
    memset(m, 0, sizeof(DeadcomCRPM));
    m->type = DCRCP_CRPM_SYS_QUERY_RESPONSE;
    m->data.sysQueryResponse.rdrClass   = rdrClass;
    m->data.sysQueryResponse.hwModel    = hwModel;
    m->data.sysQueryResponse.hwRev      = hwRev;
    m->data.sysQueryResponse.swVerMajor = swVerMajor;
    m->data.sysQueryResponse.swVerMinor = swVerMinor;
    memcpy(&(m->data.sysQueryResponse.serial), serial, 25); // TODO define 25 as constant somewhere
    (void)chMBPost(&outbox, (msg_t)m, TIME_INFINITE);
}

void dlTaskCommSendRdrFailure(char *str) {
    DeadcomCRPM *m = chGuardedPoolAllocTimeout(&out_pool, TIME_INFINITE);
    if (m == NULL) {
        chSysHalt("null from inifinitely waiting guarded pool");
        return;
    }
    memset(m, 0, sizeof(DeadcomCRPM));
    m->type = DCRCP_CRPM_RDR_FAILURE;
    // TODO 200 should not be hardcoded here
    strncpy(m->data.rdrFailure, str, 200);
    (void)chMBPost(&outbox, (msg_t)m, TIME_INFINITE);
}

void dlTaskCommSendAM0GotUids(dl_picc_uid *uids, size_t uids_len) {
    DeadcomCRPM *m = chGuardedPoolAllocTimeout(&out_pool, TIME_INFINITE);
    if (m == NULL) {
        chSysHalt("null from inifinitely waiting guarded pool");
        return;
    }
    memset(m, 0, sizeof(DeadcomCRPM));
    m->type = DCRCP_CRPM_AM0_PICC_UID_OBTAINED;
    m->data.authMethod0UuidObtained.len = uids_len;
    size_t i;
    for (i = 0; i < uids_len; i++) {
        m->data.authMethod0UuidObtained.vals[i].uid_len = uids[i].uid_len;
        memcpy(m->data.authMethod0UuidObtained.vals[i].uid, uids[i].uid, uids[i].uid_len);
    }
    (void)chMBPost(&outbox, (msg_t)m, TIME_INFINITE);
}


/*===========================================================================*/
/* Task thread and internal functions                                        */
/*===========================================================================*/

static THD_FUNCTION(commTaskControl, arg) {
    (void) arg;

    while (!chThdShouldTerminateX()) {
        if (dc_link.state == DC_DISCONNECTED) {
            dcConnect(&dc_link);
        } else {
            DeadcomCRPM *m;
            msg_t r = chMBFetch(&outbox, (msg_t*)&m, OSAL_MS2ST(10));
            if (r != MSG_OK) {
                goto heartbeat;
            }

            uint8_t data[DEADCOM_PAYLOAD_MAX_LEN];
            size_t out_size;
            DCRCPStatus s = dcrcpEncode(m, data, DEADCOM_PAYLOAD_MAX_LEN, &out_size,
                                        &cbor_out_context);
            chDbgAssert(s == DCRCP_STATUS_OK, "encoding failed");
            dcSendMessage(&dc_link, data, out_size);

            chGuardedPoolFree(&out_pool, m);
        }

        heartbeat:
        ;
        //TODO generate heartbeat
    }
}

static THD_FUNCTION(commTaskReceiveHandler, arg) {
    (void) arg;
    while (!chThdShouldTerminateX()) {
        msg_t r = sdGetTimeout(&SD2, OSAL_MS2ST(10));
        if (r != MSG_TIMEOUT && r != MSG_RESET) {
            uint8_t b[] = { (uint8_t)r };
            dcProcessData(&dc_link, b, 1);
        }

        size_t received_msg;
        DeadcomL2Result dcr = dcGetReceivedMsg(&dc_link, NULL, &received_msg);
        chDbgAssert(dcr != DC_FAILURE, "dcGetReceivedMsg failed");

        dl_task_comm_linkstate new_state;
        if (dcr == DC_OK) {
            new_state = DL_TASK_COMM_LINKUP;
        } else {
            new_state = DL_TASK_COMM_LINKDOWN;
        }
        if (last_link_state != new_state) {
            callbacks->linkChange(new_state);
            last_link_state = new_state;
        }

        if (received_msg != 0 && dcr == DC_OK) {
            if (received_msg >= DEADCOM_PAYLOAD_MAX_LEN) {
                // Too long, this can't be a valid message, so ignore it.
                goto heartbeat;
            }
            uint8_t buffer[received_msg];
            dcr = dcGetReceivedMsg(&dc_link, buffer, &received_msg);
            if (dcr == DC_NOT_CONNECTED) {
                // Link dropped since we've last checked
                last_link_state = DL_TASK_COMM_LINKDOWN;
                callbacks->linkChange(last_link_state);
                goto heartbeat;
            }
            chDbgAssert(dcr != DC_FAILURE, "dcGetReceivedMsg failed");

            DeadcomCRPM rcvd = {};
            DCRCPStatus s = dcrcpDecode(&rcvd, buffer, received_msg, &cbor_in_context);
            if (s != DCRCP_STATUS_OK) {
                // Decoding failed for whatever reason, we will ignore this message
                goto heartbeat;
            }

            switch(rcvd.type) {
                case DCRCP_CRPM_SYS_QUERY_REQUEST:
                    callbacks->rcvdSystemQueryRequest();
                    break;
                case DCRCP_CRPM_ACTIVATE_AUTH_METHOD:
                    callbacks->rcvdActivateAuthMethods(rcvd.data.authMethods.vals,
                                                       rcvd.data.authMethods.len);
                    break;
                case DCRCP_CRPM_UI_UPDATE:
                    callbacks->rcvdUiUpdate(rcvd.data.ui_class0_state);
                    break;
                default:
                    // It is safe to ignore any other frame
                    break;
            }
        }

        heartbeat:
        ;
        // TODO generate heartbeat
    }
}

// -- dcl2 functions ---------------------------------------

static bool dclTransmitBytes(const uint8_t *buf, size_t buf_size, void* context) {
    (void)context;
    sdWrite(&SD2, buf, buf_size);
    // TODO wait until the message is fully transmitted
    return true;
}

static bool dclMtxObjectInit(dl_task_comm_tvars_t *tvars) {
    chMtxObjectInit(&(tvars->m));
    return true;
}

static bool dclMtxLock(dl_task_comm_tvars_t *tvars) {
    chMtxLock(&(tvars->m));
    return true;
}

static bool dclMtxUnlock(dl_task_comm_tvars_t *tvars) {
    chMtxUnlock(&(tvars->m));
    return true;
}
static bool dclCondObjectInit(dl_task_comm_tvars_t *tvars) {
    chCondObjectInit(&(tvars->c));
    return true;
}

static bool dclCondWait(dl_task_comm_tvars_t *tvars, uint32_t milliseconds,
                        bool *timed_out) {
    msg_t r = chCondWaitTimeout(&(tvars->c), OSAL_MS2ST(milliseconds));
    if (r == MSG_TIMEOUT) {
        /* dcl2 library expects that if this function returns a success, the mutex is locked.
         * ChibiOS does not relock the mutex when chCondWaitTimeout times out (as pthreads and
         * Python's threading do). Therefore we must relock the mutex manually */
        chMtxLock(&(tvars->m));
    }
    *timed_out = (r == MSG_TIMEOUT);
    return true;
}

static bool dclCondSignal(dl_task_comm_tvars_t *tvars) {
    chCondSignal(&(tvars->c));
    return true;
}


// -- dcrcp functions --------------------------------------

static cn_cbor* allocate_cncbor(void *context) {
    cn_cbor *c = chPoolAlloc((memory_pool_t*)context);
    if (c != NULL) {
        memset(c, 0, sizeof(cn_cbor));
    }
    return c;
}

static void free_cncbor(cn_cbor *p, void *context) {
    chPoolFree((memory_pool_t*)context, p);
}


/*===========================================================================*/
/* Common task API                                                           */
/*===========================================================================*/

void dlTaskCommInit(uint8_t _ctrl_task_id, uint8_t _rcv_task_id,
                    dl_task_comm_callbacks *_callbacks) {
    ctrl_task_id = _ctrl_task_id;
    rcv_task_id  = _rcv_task_id;
    callbacks    = _callbacks;

    chGuardedPoolObjectInit(&out_pool, sizeof(DeadcomCRPM));
    chGuardedPoolLoadArray(&out_pool, out_objs, OUT_QUEUE_LENGTH);
    chMBObjectInit(&outbox, outbox_buffer, OUT_QUEUE_LENGTH);

    chPoolObjectInit(&cbors_out_pool, sizeof(cn_cbor), NULL);
    chPoolLoadArray(&cbors_out_pool, cbors_out, DCRCP_REQUIRED_CNCBOR_BUFFERS);
    chPoolObjectInit(&cbors_in_pool, sizeof(cn_cbor), NULL);
    chPoolLoadArray(&cbors_in_pool, cbors_in, DCRCP_REQUIRED_CNCBOR_BUFFERS);
}

void dlTaskCommStart(void) {
    // TODO this is board specific (see board.h)! So it shouldn't be here. It will be refactored
    // once we start supporting more boards
    palSetLineMode(LINE_RDR_TXD, PAL_MODE_ALTERNATE(1));
    sdStart(&SD2, &sd2_config);
    DeadcomL2Result r = dcInit(&dc_link, &dl_task_comm_tvars, &dl_task_comm_tvars,
                               (DeadcomL2ThreadingMethods*)&dcl2_t_methods, &dclTransmitBytes,
                               NULL);
    chDbgAssert((r == DC_OK), "dcl2 init failed");
    task_control_thread = chThdCreateStatic(commTaskControlThreadWA,
                                            sizeof(commTaskControlThreadWA), NORMALPRIO,
                                            commTaskControl, NULL);
    task_receive_thread = chThdCreateStatic(commTaskReceiveThreadWA,
                                            sizeof(commTaskReceiveThreadWA), NORMALPRIO,
                                            commTaskReceiveHandler, NULL);
}

void dlTaskCommStop(void) {
    chThdTerminate(task_control_thread);
    chThdTerminate(task_receive_thread);
}
