/**
 * @file    hal_abstract_iso14443_pcd.h
 * @brief   Abstract ISO14443 Proximity Coupling Device (card reader) driver
 *          interface.
 * @details This header defines an abstract interface useful to access generic
 *          ISO14443-compliant PCDs (contactless card readers) in a
 *          standardized way.
 *
 * @addtogroup ISO14443_PCD
 * @details This module defines an abstract interface for using
 *          ISO14443-compliant PCDs.<br>
 *          Note that no code is present, just abstract interfaces-like
 *          structures, you should look at the system as to a set of
 *          abstract C++ classes (even if written in C). This system
 *          has then advantage to make the access to block devices
 *          independent from the implementation logic.
 * @{
 */

#ifndef HAL_ABSTRACT_ISO14443_PCD
#define HAL_ABSTRACT_ISO14443_PCD

#include "hal.h"
#include "hal_custom.h"


/**
 * @brief  States of the driver state machine
 */
typedef enum {
    PCD_UNINT         = 0,      /**< Not initialized                         */
    PCD_RF_OFF        = 1,      /**< RF Field is off                         */
    PCD_READY         = 2,      /**< Ready to transmit                       */
    PCD_ACTIVE        = 3,      /**< Transceiving                            */
    PCD_GOT_RESP      = 4,      /**< Response was received                   */
    PCD_GOT_RESP_COLL = 5,      /**< Response was received with collision    */
    PCD_GOT_TIMEOUT   = 6       /**< Timeout while waiting for response      */
} pcdstate_t;

/**
 * @brief  Operation result codes
 */
typedef enum {
    PCD_OK            = 0,      /**< Command completed successfully          */
    PCD_BAD_STATE     = 1,      /**< Command not possible in this state      */
    PCD_UNSUPPORTED   = 2,      /**< This PCD does not support this command  */
    PCD_OK_COLLISION  = 3,      /**< Command OK, but received collision      */
    PCD_OK_TIMEOUT    = 4,      /**< Command OK, but card did not respond    */
    PCD_LOCKED        = 5       /**< Concurrent requests from mult. threads  */
} pcdresult_t;

/**
 * @brief  Receive speed keys for speed bitmask
 */
typedef enum {
    PCD_RX_SPEED_106  = 1,
    PCD_RX_SPEED_212  = 2,
    PCD_RX_SPEED_424  = 4,
    PCD_RX_SPEED_848  = 8,
} pcdspeed_rx_t;

/**
 * @brief  Transmit speed keys for speed bitmask
 */
typedef enum {
    PCD_TX_SPEED_106  = 16,
    PCD_TX_SPEED_212  = 32,
    PCD_TX_SPEED_424  = 64,
    PCD_TX_SPEED_848  = 128
} pcdspeed_tx_t;

/**
 * @brief  Standard communication modes
 */
typedef enum {
    PCD_ISO14443_A    = 0,
    PCD_ISO14443_B    = 1
} pcdmode_t;

/**
 * @brief  Structure of communuication parameters supported by the PCD
 */
typedef struct {
    uint8_t supported_speedsA;      /**< Bit mask of supported tx/rx speeds  */
    uint8_t supported_speedsB;      /**< Bit mask of supported tx/rx speeds  */
    bool    supported_asym_speeds;  /**< Support of asymetric speed setting  */
    uint8_t supported_modes;        /**< Bit mask of supported modes (A or B)*/
} PcdSParams;

/**
 * @brief  @p Abstract ISO14443 PCD Virtual Methods Table
 */
struct BasePcdVMT {
    pcdstate_t  (*getStateAB)(void *inst);
    pcdresult_t (*activateRFAB)(void *inst);
    pcdresult_t (*deactivateRFAB)(void *inst);
    PcdSParams  (*getSupportedParamsAB)(void *inst);
    pcdresult_t (*setParamsAB)(void *inst, pcdspeed_rx_t rx_spd,
                               pcdspeed_tx_t tx_spd, pcdmode_t mode);
    pcdresult_t (*transceiveShortFrameA)(void *inst, uint8_t data,
                                         uint16_t *resp_length);
    pcdresult_t (*transceiveStandardFrameA)(void *inst, uint8_t* buffer,
                                            uint16_t length,
                                            uint16_t *resp_length);
    pcdresult_t (*transceiveAnticollFrameA)(void *inst, uint8_t* buffer,
                                            uint16_t length,
                                            uint8_t n_last_bits,
                                            uint16_t *resp_length);
    pcdresult_t (*sendShortFrameA)(void *inst, uint8_t data);
    pcdresult_t (*sendStandardFrameA)(void *inst, uint8_t* buffer,
                                      uint16_t length);
    pcdresult_t (*sendAnticollFrameA)(void *inst, uint8_t buffer,
                                      uint16_t length, uint8_t n_last_bits);
    pcdresult_t (*waitForResponseA)(void *inst, uint16_t *resp_length);
    pcdresult_t (*getResponseLengthA)(void *inst, uint16_t *resp_length);
    pcdresult_t (*getResponseAB)(void *inst, uint16_t buffer_size,
                                 uint8_t *buffer, uint8_t *n_last_bits);
    pcdresult_t (*discardResponseAB)(void *inst);
};

/**
 * @brief   Base ISO/IEC 14443 PCD
 * @details This class represents a generic ISO/IEC 14443 Proximity Coupling
 *          device.
 */
typedef struct {
  const struct BasePcdVMT *vmt;             /**< Virtual Methods Table       */
  void *data;                               /**< Private data of a driver    */
} Pcd;

/**
 * @name  Macro functions (Pcd)
 * @{
 */

#define pcdGetStateAB(ip) ((ip)->vmt->getStateAB(ip))

#define pcdActivateRFAB(ip) ((ip)->vmt->activateRFAB(ip))

#define pcdDeactivateRFAB(ip) ((ip)->vmt->deactivateRFAB(ip))

#define pcdGetSupportedParamsAB(ip) ((ip)->vmt->getSupportedParamsAB(ip))

#define pcdSetParamsAB(ip, rx_spd, tx_spd, mode)                              \
        ((ip)->vmt->setParamsAB(ip, rx_spd, tx_spd, mode))

#define pcdTransceiveShortFrameA(ip, data, resp_len_p)                        \
        ((ip)->vmt->transceiveShortFrameA(ip, data, resp_len_p))

#define pcdTransceiveStdFrameA(ip, buffer, len, resp_len_p)                   \
        ((ip)->vmt->transceiveStandardFrameA(ip, buffer, len, resp_len_p))

#define pcdTransceiveAnticollFrameA(ip, buffer, len, n_last_bits, resp_len_p) \
        ((ip)->vmt->transceiveAnticollFrameA(ip, buffer, len, n_last_bits     \
                                             resp_len_p))

#define pcdSendShortFrameA(ip, data) ((ip)->vmt->sendShortFrameA(ip, data))

#define pcdSendStdFrameA(ip, buffer, len)                                     \
        ((ip)->vmt->sendStandardFrameA(ip, buffer, len))

#define pcdSendAnticollFrameA(ip, buffer, len, n_last_bits)                   \
        ((ip)->vmt->sendAnticollFrameA(ip, buffer, len, n_last_bits))

#define pcdWaitForResponseA(ip, resp_len_p)                                   \
        ((ip)->vmt->waitForResponseA(ip, resp_len_p))

#define pcdGetRespLengthA(ip, resp_len_p)                                     \
        ((ip)->vmt->getResponseLengthA(ip, resp_len_p))

#define pcdGetRespAB(ip, buf_size, buffer, n_last_bits_p)                     \
        ((ip)->vmt->getResponseAB(ip, buf_size, buffer, n_last_bits_p))

#define pcdDiscardResponseAB(ip) ((ip)->vmt->discardResponseAB(ip))

/** @} */

#endif /* HAL_ABSTRACT_ISO14443_PCD */

/** @} */
