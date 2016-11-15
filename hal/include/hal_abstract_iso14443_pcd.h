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
    PCD_GOT_TIMEOUT   = 6,      /**< Timeout while waiting for response      */
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
    PCD_LOCKED        = 5       /**< In use by another thread                */
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
    uint8_t  supported_speedsA;     /**< Bit mask of supported tx/rx speeds  */
    uint8_t  supported_speedsB;     /**< Bit mask of supported tx/rx speeds  */
    bool     supported_asym_speeds; /**< Support of asymetric speed setting  */
    uint8_t  supported_modes;       /**< Bit mask of supported modes (A or B)*/
    uint16_t max_tx_size;           /**< Maximum Transmit buffer size        */
    uint16_t max_rx_size;           /**< Maximum Receive buffer size         */
} PcdSParams;

/**
 * @brief  @p Abstract ISO14443 PCD Virtual Methods Table
 */
struct BasePcdVMT {
    pcdstate_t  (*getStateAB)(void *inst);
    pcdresult_t (*activateRFAB)(void *inst);
    pcdresult_t (*deactivateRFAB)(void *inst);
    PcdSParams* (*getSupportedParamsAB)(void *inst);
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
                                 uint8_t *buffer, uint16_t *size_copied,
                                 uint8_t *n_last_bits);
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

/**
 * @brief      Returns the device state
 *
 * @param[in]  ip    pointer to a Pcd structure
 *
 * @return     State of the device
 */
#define pcdGetStateAB(ip) ((ip)->vmt->getStateAB(ip))

/**
 * @brief      Activates the RF Field of the PCD
 *
 * @param[in]  ip    pointer to a Pcd structure
 *
 * @return     Operation result
 * @retval     PCD_OK
 * @retval     PCD_BAD_STATE
 * @retval     PCD_LOCKED
 */
#define pcdActivateRFFieldAB(ip) ((ip)->vmt->activateRFAB(ip))

/**
 * @brief      Deactivates the RF Field of the PCD
 *
 * @param[in]  ip    pointer to a Pcd structure
 *
 * @return     Operation result
 * @retval     PCD_OK
 * @retval     PCD_BAD_STATE
 * @retval     PCD_LOCKED
 */
#define pcdDeactivateRFFieldAB(ip) ((ip)->vmt->deactivateRFAB(ip))

/**
 * @brief      Returns supported features (speed, modes) of this PCD
 *
 * @param[in]  ip    pointer to a Pcd structure
 *
 * @return     pointer to a @p PcdSParams structure. This structure should not
 *             be modified. If @var PcdSParams::max_tx_size or
 *             @var PcdSParams::max_rx_size is zero then no limits are placed
 *             on the maximum buffer size
 */
#define pcdGetSupportedParamsAB(ip) ((ip)->vmt->getSupportedParamsAB(ip))

/**
 * @brief      Sets communication parameters.
 * @details    If the parameter combination is unsupported #PCD_UNSUPPORTED
 *             will be returned. It is advised to first check the value
 *             returned by #pcdGetSupportedParamsAB.
 *
 * @param[in]  ip      pointer to a Pcd strucutre
 * @param[in]  rx_spd  The pcdspeed_rx_t receive speed
 * @param[in]  tx_spd  The pcdspeed_tx_t transmit speed
 * @param[in]  mode    The pcdmode_t PCD mode
 *
 * @return     Operation result
 * @retval     PCD_OK
 * @retval     PCD_BAD_STATE
 * @retval     PCD_UNSUPPORTED
 * @retval     PCD_LOCKED
 */
#define pcdSetParamsAB(ip, rx_spd, tx_spd, mode)                              \
        ((ip)->vmt->setParamsAB(ip, rx_spd, tx_spd, mode))

/**
 * @brief      Transmits a 'Short Frame' and blocks until the response is
 *             ready.
 * @details    Short frame transmits 7 data bits without parity. Therefore only
 *             7 Least Significant Bits of parameter @p data are sent.
 *             For @p resp_len_p meaning see pcdTransceiveStdFrameA.
 *
 * @param      ip          pointer to a Pcd structure
 * @param      data        7 bits of data to be sent (MSB is ignored)
 * @param      resp_len_p  Length of the received response
 *
 * @return     Operation result and received response type (pcdresult_t)
 */
#define pcdTransceiveShortFrameA(ip, data, resp_len_p)                        \
        ((ip)->vmt->transceiveShortFrameA(ip, data, resp_len_p))

/**
 * @brief      Transmits a 'Standard Frame' and blocks until the response is
 *             ready.
 * @details    Standard frame transmits `n` (where `n >= 1`) bytes.
 *             The @p len parameter cannot be greater than maximum buffer size
 *             supported by the reader. After the response is received
 *             @p resp_len_p will be set to the received response length. You
 *             can then use this parameter to allocate a new buffer and obtain
 *             the response using pcdGetRespAB function.
 *             If you don't care about the response you can discard it using
 *             pcdDiscardResponseAB.
 *             Even if no response is received (because the timeout occurs)
 *             you still must call either pcdGetRespAB or pcdDiscardResponseAB
 *             to transition again to a PCD_READY state (think of it as
 *             acknowledging the fact that a timeout has occured).
 *
 * @param      ip          pointer to a Pcd structure
 * @param      buffer      Bytes to send in a standard frame
 * @param      len         Size of the @p buffer
 * @param      resp_len_p  Size of received response
 *
 * @return     Operation result and received response type (pcdresult_t)
 * @see        pcdGetSupportedParamsAB
 * @see        pcdGetRespAB
 * @see        pcdDiscardResponseAB
 */
#define pcdTransceiveStdFrameA(ip, buffer, len, resp_len_p)                   \
        ((ip)->vmt->transceiveStandardFrameA(ip, buffer, len, resp_len_p))

/**
 * @brief      Transmits the first part of an 'Anticollision Frame' and blocks
 *             until the response is ready.
 * @details    Anticollision frame is a standard 7-byte frame split anywhere
 *             after 16th bit and before 55th bit. First part is transmitted
 *             by the PCD and the second part is transmitted by the PICC as
 *             a part of an anticollision sequence.
 *             For @p resp_len_p meaning see pcdTransceiveStdFrameA.
 *
 * @param      ip           pointer to a Pcd structure
 * @param      buffer       Bytes to send in an anticoll frame
 * @param      len          Size of the buffer
 * @param      n_last_bits  Number of valid bits in the last byte to be
 *                          transmitted. 0 means the whole byte is valid.
 * @param      resp_len_p   Size of the received response
 *
 * @return     Operation result and received response type (pcdresult_t)
 * @see        pcdTransceiveStdFrameA
 */
#define pcdTransceiveAnticollFrameA(ip, buffer, len, n_last_bits, resp_len_p) \
        ((ip)->vmt->transceiveAnticollFrameA(ip, buffer, len, n_last_bits     \
                                             resp_len_p))

/**
 * @brief      Non-blocking version of pcdTransceiveShortFrameA.
 * @detail     This function does the same thing as pcdTransceiveShortFrameA
 *             except for waiting for the response. This function returns
 *             immediatelly, leaving the Pcd in PCD_ACTIVE state which
 *             eventually transitions to either PCD_GOT_RESP, PCD_GOT_RESP_COLL
 *             or PCD_GOT_TIMEOUT state. To wait for this transition you can
 *             use blocking function pcdWaitForResponseA.
 *
 * @see        pcdTransceiveShortFrameA
 * @see        pcdWaitForResponseA
 */
#define pcdSendShortFrameA(ip, data) ((ip)->vmt->sendShortFrameA(ip, data))

/**
 * @brief      Non-blocking version of pcdTransferStdFrameA.
 * @defail     This function does the same thing as pcdTransceiveStdFrameA
 *             except for waiting for the response. This function returns
 *             immediatelly, leaving the Pcd in PCD_ACTIVE state which
 *             eventually transitions to either PCD_GOT_RESP, PCD_GOT_RESP_COLL
 *             or PCD_GOT_TIMEOUT state. To wait for this transition you can
 *             use blocking function pcdWaitForResponseA.
 *
 * @see        pcdTransceiveStdFrameA
 * @see        pcdWaitForResponseA
 */
#define pcdSendStdFrameA(ip, buffer, len)                                     \
        ((ip)->vmt->sendStandardFrameA(ip, buffer, len))

/**
 * @brief      Non-blocking version of pcdTransferAnticollFrameA
 * @defail     This function does the same thing as pcdTransceiveAnticollFrameA
 *             except for waiting for the response. This function returns
 *             immediatelly, leaving the Pcd in PCD_ACTIVE state which
 *             eventually transitions to either PCD_GOT_RESP, PCD_GOT_RESP_COLL
 *             or PCD_GOT_TIMEOUT state. To wait for this transition you can
 *             use blocking function pcdWaitForResponseA.
 *
 * @see        pcdTransceiveAnticollFrameA
 * @see        pcdWaitForResponseA
 */
#define pcdSendAnticollFrameA(ip, buffer, len, n_last_bits)                   \
        ((ip)->vmt->sendAnticollFrameA(ip, buffer, len, n_last_bits))

/**
 * @brief      Blocks until either a response is received or a timeout occurs.
 *
 * @param      ip          pointer to a Pcd structure
 * @param      resp_len_p  Size of the received response
 *
 * @return     Operation result and received response type (pcdresult_t)
 */
#define pcdWaitForResponseA(ip, resp_len_p)                                   \
        ((ip)->vmt->waitForResponseA(ip, resp_len_p))

/**
 * @brief      Gets (remaining) size of response stored in the buffer, if any
 *
 * @param      ip          pointer to a Pcd structure
 * @param      resp_len_p  Size of the received response
 *
 * @return     Operation result
 */
#define pcdGetRespLengthA(ip, resp_len_p)                                     \
        ((ip)->vmt->getResponseLengthA(ip, resp_len_p))

/**
 * @brief      Read response from the internal response buffer.
 * @detail     Read response from the internal buffer. If size of the buffer
 *             passed to this function is smaller than the response size
 *             only part of the response will be copied and this function must
 *             be called several times. Bytes received first are copied first.
 *             @p n_last_bits_p is valid only when copying last part of the
 *             response.
 *             If the whole buffer is copied this function transitions the Pcd
 *             to a PCD_READY state.
 *
 * @param[in]  ip             pointer to a Pcd structure
 * @param[in]  buf_size       Max size to read
 * @param[out] buffer         The buffer
 * @param[out] size_copied    Number of bytes written to the buffer
 * @param[out] n_last_bits_p  Number of valid bits in last byte copied to the
 *                            buffer.
 *
 * @return     Operation result
 */
#define pcdGetRespAB(ip, buf_size, buffer, size_copied, n_last_bits_p)        \
        ((ip)->vmt->getResponseAB(ip, buf_size, buffer, size_copied,          \
                                  n_last_bits_p))

/**
 * @brief      Deletes content of the internal response buffer and transitions
 *             to PCD_READY state
 *
 * @param      ip    pointer to a Pcd structure
 *
 * @return     Operation result
 */
#define pcdDiscardResponseAB(ip) ((ip)->vmt->discardResponseAB(ip))

/** @} */

#endif /* HAL_ABSTRACT_ISO14443_PCD */

/** @} */
