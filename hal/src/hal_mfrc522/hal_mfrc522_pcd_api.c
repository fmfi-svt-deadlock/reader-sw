/**
 * @file    hal_mfrc522_pcd_api.c
 * @brief   Implementation of the PCD API
 * @detial	This file implements functions defined in the
 *          hal_abstract_iso14443_pcd.h (definition of the abstract PCD API)
 *          for the MFRC522.
 */

#include "hal_mfrc522_internal.h"

// ----------- COMMON MACROS -------------

#define MEMBER_FUNCTION_CHECKS(inst)                                          \
    osalDbgCheck(inst != NULL);                                               \
    osalDbgCheck(((Pcd*)inst)->data != NULL);

#define DEFINE_AND_SET_mdp(inst)                                              \
    Mfrc522Driver *mdp = (Mfrc522Driver*)((Pcd*)inst)->data;

// TODO counterintuitive!
#define CHECK_STATE(condition)                                                \
    if (condition) { return PCD_BAD_STATE; }

// ----------- COMMON TRANSMISSION ROUTINES -------------

static void prepare_transceive(Mfrc522Driver *mdp) {
    // Flush FIFO
    mfrc522_write_register(mdp, FIFOLevelReg, (1 << FIFOLevelReg_FlushBuffer));

    // Clear interrupt bits
    // Select all interrupts, bit Set1 is zero. This will unset all selected
    // interrupts.
    mfrc522_write_register(mdp, ComIrqReg, 0xFF & ~(1 << ComIrqReg_Set1));
    mdp->interrupt_pending = false;
    // Enable interrupts: Rx complete, Timeout and Error
    mfrc522_set_register_bits(mdp, ComIEnReg,
                      (1 << ComIEnReg_RxIEn) |
                      (1 << ComIEnReg_ErrIEn));


    mfrc522_command(mdp, Command_Transceive);
}

static msg_t wait_for_response(Mfrc522Driver *mdp, uint32_t timeout_us) {
    osalSysLock();
    msg_t message;
    if (!mdp->interrupt_pending) {
        message = osalThreadSuspendTimeoutS(&mdp->tr,
                                            OSAL_US2ST(timeout_us));
    }
    mdp->interrupt_pending = false;
    osalSysUnlock();
}

static void cleanup_transceive(Mfrc522Driver *mdp) {
    // Reset bit-oriented adjustments and clear 'Start Send' flag
    mfrc522_write_register(mdp, BitFramingReg, 0);

    // Clear interrupt bits and disable previously enabled interrupts
    mfrc522_clear_register_bits(mdp, ComIEnReg,
                        (1 << ComIEnReg_RxIEn) |
                        (1 << ComIEnReg_ErrIEn));
    mfrc522_write_register(mdp, ComIrqReg, 0xFF & ~(1 << ComIrqReg_Set1));
}

static pcdresult_t handle_response(Mfrc522Driver *mdp,
                                   msg_t message,
                                   uint16_t *resp_length,
                                   bool collisions_possible) {
    if (message == MSG_TIMEOUT) {
        return PCD_OK_TIMEOUT;
    }

    if (message != MFRC522_MSG_INTERRUPT) {
        // OK, this should not have happened. Fail-fast.
        osalSysHalt("Unexpected wakeup message");
    }

    // Handle possible error
    if (!(mfrc522_read_register(mdp, ComIrqReg) & (1 << ComIrqReg_RxIRq))) {
        // This was not a 'receive complete' interrupt
        // TODO more thorough error handling
        return PCD_ERROR;
    }

    // TODO if collisions are possible handle them!

    mdp->resp_read_bytes = 0;
    mdp->resp_length = mfrc522_read_register(mdp, FIFOLevelReg);
    *resp_length = mdp->resp_length;
    mfrc522_read_register_burst(mdp, FIFODataReg, mdp->response, mdp->resp_length);

    return PCD_OK;
}

// ----------- API FUNCTIONS -------------

pcdstate_t mfrc522GetStateAB(void *inst) {
    MEMBER_FUNCTION_CHECKS(inst);
    DEFINE_AND_SET_mdp(inst);

    return mdp->state;
}

pcdresult_t mfrc522ActivateRFAB(void *inst) {
    MEMBER_FUNCTION_CHECKS(inst);
    DEFINE_AND_SET_mdp(inst);
    CHECK_STATE(mdp->state != PCD_RF_OFF);

    mfrc522_set_register_bits(mdp, TxControlReg,
                      (1 << TxControlReg_Tx1RFEn) | (1 << TxControlReg_Tx2RFEn));
    mdp->state = PCD_READY;
    return PCD_OK;
}

pcdresult_t mfrc522DeactivateRFAB(void *inst) {
    MEMBER_FUNCTION_CHECKS(inst);
    DEFINE_AND_SET_mdp(inst);
    CHECK_STATE(mdp->state != PCD_READY);

    mfrc522_clear_register_bits(mdp, TxControlReg,
                        (1 << TxControlReg_Tx1RFEn) | (1 << TxControlReg_Tx2RFEn));
    mdp->state = PCD_RF_OFF;
    return PCD_OK;
}

const PcdSParams* mfrc522GetSupportedParamsAB(void *inst) {
    return &supported_params;
}

pcdresult_t mfrc522SetParamsAB(void *inst, pcdspeed_rx_t rx_spd,
                               pcdspeed_tx_t tx_spd, pcdmode_t mode) {
    MEMBER_FUNCTION_CHECKS(inst);
    DEFINE_AND_SET_mdp(inst);
    CHECK_STATE(mdp->state != PCD_READY && mdp->state != PCD_RF_OFF);

    uint8_t rx_speed;
    uint8_t tx_speed;

    if (mode != PCD_ISO14443_A) {
        return PCD_UNSUPPORTED;
    }

    switch(rx_spd) {
        case PCD_RX_SPEED_106:
            rx_speed = RxModeReg_RxSpeed_106;
            break;
        case PCD_RX_SPEED_212:
            rx_speed = RxModeReg_RxSpeed_212;
            break;
        case PCD_RX_SPEED_424:
            rx_speed = RxModeReg_RxSpeed_424;
            break;
        case PCD_RX_SPEED_848:
            rx_speed = RxModeReg_RxSpeed_848;
            break;
        default:
            return PCD_UNSUPPORTED;
    }

    switch(tx_spd) {
        case PCD_TX_SPEED_106:
            tx_speed = TxModeReg_TxSpeed_106;
            break;
        case PCD_TX_SPEED_212:
            tx_speed = TxModeReg_TxSpeed_212;
            break;
        case PCD_TX_SPEED_424:
            tx_speed = TxModeReg_TxSpeed_424;
            break;
        case PCD_TX_SPEED_848:
            tx_speed = TxModeReg_TxSpeed_848;
            break;
        default:
            return PCD_UNSUPPORTED;
    }

    mfrc522_write_register_bitmask(mdp, TxModeReg, Mask_TxModeReg_TxSpeed,
                           (tx_speed << TxModeReg_TxSpeed));
    mfrc522_write_register_bitmask(mdp, RxModeReg, Mask_RxModeReg_RxSpeed,
                           (rx_speed << RxModeReg_RxSpeed));

    if (tx_spd == PCD_TX_SPEED_106 && rx_spd == PCD_RX_SPEED_106
        && mode == PCD_ISO14443_A) {
        // Standard mandates 100% ASK for 106k speed in mode A
        mfrc522_set_register_bits(mdp, TxASKReg, (1 << TxAskReg_Force100ASK));
    } else {
        mfrc522_clear_register_bits(mdp, TxASKReg, (1 << TxAskReg_Force100ASK));
    }

    return PCD_OK;
}

pcdresult_t mfrc522TransceiveShortFrameA(void *inst, uint8_t data,
                                         uint16_t *resp_length,
                                         uint32_t timeout_us) {
    MEMBER_FUNCTION_CHECKS(inst);
    DEFINE_AND_SET_mdp(inst);
    CHECK_STATE(mdp->state != PCD_READY);

    mdp->state = PCD_ACTIVE;
    prepare_transceive(mdp);

    // Write data, set Start Send and transmit 7 bits
    mfrc522_write_register(mdp, FIFODataReg, data);
    mfrc522_write_register(mdp, BitFramingReg,
                           (7 << BitFramingReg_TxLastBits) |
                           (1 << BitFramingReg_StartSend));

    msg_t message = wait_for_response(mdp, timeout_us);

    cleanup_transceive(mdp);
    mdp->state = PCD_READY;
    return handle_response(mdp, message, resp_length, false);
}

// TODO untested!
pcdresult_t mfrc522TransceiveStandardFrameA(void *inst, uint8_t *buffer,
                                            uint16_t length,
                                            uint16_t *resp_length,
                                            uint32_t timeout_us) {
    MEMBER_FUNCTION_CHECKS(inst);
    DEFINE_AND_SET_mdp(inst);
    CHECK_STATE(mdp->state != PCD_READY);

    mdp->state = PCD_ACTIVE;
    prepare_transceive(mdp);

    // Write data, set Start Send
    mfrc522_write_register_burst(mdp, FIFODataReg, buffer, length);
    mfrc522_write_register(mdp, BitFramingReg,
                           (7 << BitFramingReg_TxLastBits) |
                           (1 << BitFramingReg_StartSend));

    msg_t message = wait_for_response(mdp, timeout_us);

    cleanup_transceive(mdp);
    mdp->state = PCD_READY;
    return handle_response(mdp, message, resp_length, false);
}

// TODO untested!
pcdresult_t mfrc522TransceiveAnticollFrameA(void *inst, uint8_t *buffer,
                                            uint16_t length,
                                            uint8_t n_last_bits,
                                            uint16_t *resp_length,
                                            uint32_t timeout_us) {
    MEMBER_FUNCTION_CHECKS(inst);
    DEFINE_AND_SET_mdp(inst);
    CHECK_STATE(mdp->state != PCD_READY);

    mdp->state = PCD_ACTIVE;
    prepare_transceive(mdp);

    // Write data, set Start Send and set number of valid bytes.
    // TODO completly forgot about RX align!
    mfrc522_write_register_burst(mdp, FIFODataReg, buffer, length);
    mfrc522_write_register(mdp, BitFramingReg,
                           ((n_last_bits & 0x7) << BitFramingReg_TxLastBits) |
                           (1 << BitFramingReg_StartSend));

    msg_t message = wait_for_response(mdp, timeout_us);

    cleanup_transceive(mdp);
    mdp->state = PCD_READY;
    return handle_response(mdp, message, resp_length, true);
}

uint16_t mfrc522GetResponseLengthA(void *inst) {

}

pcdresult_t mfrc522GetResponseAB(void *inst, uint16_t buffer_size,
                                 uint8_t *buffer, uint16_t *size_copied,
                                 uint8_t *n_last_bits) {

}

void mfrc522AcquireBus(void *inst) {

}

void mfrc522ReleaseBus(void *inst) {

}

bool mfrc522SupportsExtFeature(void *inst, pcdfeature_t feature) {

}

pcdresult_t mfrc522CallExtFeature(void *inst, pcdfeature_t feature,
                                  void *params, void *result) {

}

const struct BasePcdVMT mfrc522VMT = {
    &mfrc522GetStateAB,
    &mfrc522ActivateRFAB,
    &mfrc522DeactivateRFAB,
    &mfrc522GetSupportedParamsAB,
    &mfrc522SetParamsAB,
    &mfrc522TransceiveShortFrameA,
    &mfrc522TransceiveStandardFrameA,
    &mfrc522TransceiveAnticollFrameA,
    &mfrc522GetResponseLengthA,
    &mfrc522GetResponseAB,
    &mfrc522AcquireBus,
    &mfrc522ReleaseBus,
    &mfrc522SupportsExtFeature,
    &mfrc522CallExtFeature
};
