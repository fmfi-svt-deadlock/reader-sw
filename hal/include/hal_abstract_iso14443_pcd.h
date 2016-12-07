/**
 * @file    hal_abstract_iso14443_pcd.h
 * @brief   Abstract ISO14443 Proximity Coupling Device (card reader) driver
 *          interface.
 *
 * This header defines an abstract interface useful to access generic
 * ISO14443-compliant PCDs (contactless card readers) in a
 * standardized way.
 *
 * ### Design
 *
 * This header provides communication interface with the PCD (Proximity
 * Coupling Device) as described in `ISO/IEC 14443-3`. This standard defines
 * communication with 2 different card types: `A` and `B`. The naming
 * convention is  ‘A’-related functions end with A, B-related functions end
 * with B, common functions end with AB.
 *
 * Currently, API for part A is fully designed, API for part B will be added
 * later.
 *
 * Communication between the PCD and the PICC consists of sending and receiving
 * frames. The frames are transmitted in pairs, PCD to PICC followed by PICC
 * to PCD.
 *
 * Part A of the standard defines transmission of 3 different types of frames:
 *
 *   - Short frame: transmits 7 bits.
 *   - Standard frame: Used for data exchange and can transmit several bytes
 *     with parity.
 *   - Bit oriented anticollision frame: 7 byte long frame spit anywhere into
 *     two parts. First part is transmitted by the PCD, second part is added by
 *     the PICC. It is used during bit-oriented anticollision loop.
 *
 * ISO/IEC 14443-3 specifies different communication methods (different
 * modulation type / index, different encoding) for part A and B. This driver
 * should support setting these modes, various other communication parameters
 * within these modes as defined by the standard, and should also be able to
 * advertise its capabilities. Communication speeds can’t be arbitrary and are
 * defined by ISO/IEC 14443-4 as `$ 1 etu = 128 / (D x fc) $`, where etu is the
 * elementary time unit (duration of one bit), fc is the carrier frequency
 * (defined in ISO/IEC 14443-2 to be `$ 13.56MHz \pm 7kHz $`) and `D` is an
 * integer divisor, which may be 1, 2, 4 or 8. This paradoxically means that
 * increasing the divisor also increases the communication speed.
 *
 * The readers also usually support a number of extended features, not covered
 * by the ISO/IEC 14443 standard. For example, the MFRC522 is able to perform
 * a Mifare authentication using its crypto unit, or a self-test. Upper layers
 * which know how to use these extended features should have access to them,
 * but they should not clutter the main API. That is why each extended feature
 * will have a globally assigned number (in the global abstract header). Then
 * other layers could use these numbers to invoke the extended feature, passing
 * in a “parameter structure”. These structures are also defined globally in
 * the abstract header (although in a different file) to allow their re-use.
 *
 * Often PCDs have a maximum data size they can handle at once. ISO/IEC 14443
 * standard takes this into account and defines “protocol chaining”, a method
 * to send large data units in multiple smaller frames. The upper library
 * handles this, but for it to know whether to use chaining the PCD must be
 * able to report maximum frame size it can handle.
 *
 * ### Driver state diagram
 *
 * This abstract class presumes a driver with state. The following diagram
 * illustrates available states (see #pcdstate_t) and transitions between them.
 *
 * Some functions may be called only in specific states, this is indicated in
 * documentation of each function. Calling a function in an invalid state will
 * be caught by assertion (if assertions are enabled).
 *
 * @verbatim embed:rst

   .. graphviz::

     digraph card_state_transitions {
        PCD_UNINIT -> PCD_STOP [label="driver-specific\ninit\nmethod"]
        PCD_STOP -> PCD_RF_OFF [label="driver-specific\nactivation\nmethod"]
        PCD_RF_OFF -> PCD_READY [label="pcdActivateRFFieldAB()"]
        PCD_READY -> PCD_RF_OFF [label="pcdDeactivateRFFieldAB()"]
        PCD_READY -> PCD_ACTIVE [label="pcdTransceive*()"]
        PCD_ACTIVE -> PCD_READY [label="operation complete \n operation timeout \n operation error"]
     }

  @endverbatim
 *
 * ### Thread safety
 *
 * Implementation of this API does not have to guarantee that it is thread-safe.
 * If you need API access from multiple threads use #pcdAcquireBus and
 * #pcdReleaseBus APIs in order to get exclusive access.
 *
 * @addtogroup HAL_ABSTRACT_ISO14443_PCD
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
    PCD_UNINT,                  /**< Not initialized                         */
    PCD_STOP,                   /**< Initialized, not active.                */
    PCD_RF_OFF,                 /**< RF Field is off                         */
    PCD_READY,                  /**< Ready to transmit                       */
    PCD_ACTIVE                  /**< Transceiving                            */
} pcdstate_t;

/**
 * @brief  Operation result codes
 */
typedef enum {
    PCD_OK,                     /**< Command completed successfully          */
    PCD_BAD_STATE,              /**< Command not possible in this state      */
    PCD_UNSUPPORTED,            /**< This PCD does not support this command  */
    PCD_OK_COLLISION,           /**< Command OK, but received collision      */
    PCD_OK_TIMEOUT,             /**< Command OK, but card did not respond    */
    PCD_ERROR,                  /**< An unspecified error has occured        */
    PCD_TX_ERROR,               /**< Transmission error                      */
    PCD_RX_ERROR,               /**< Receiver error (such as bad parity)     */
    PCD_RX_OVERFLOW,            /**< A receive buffer has overflown          */
    PCD_TX_OVERFLOW             /**< This message won't fit to tx buffer     */
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
    PCD_ISO14443_A    = 1,
    PCD_ISO14443_B    = 2
} pcdmode_t;

/**
 * @brief  List of possible extended features.
 *
 * For each extended feature 2 structures are defined in file
 * hal_abstract_iso14443_pcd_ext.h. The first is a param strucure, the second
 * is a response structure.
 */
typedef enum {
    PCD_EXT_SELFTEST,          /**< Perform a self-test                      */
    PCD_EXT_CALCULATE_CRC_A,   /**< Calculate type-A CRC                     */
    PCD_EXT_CALCULATE_CRC_B,   /**< Calculate type-B CRC                     */
    PCD_EXT_MIFARE_AUTH        /**< Perform a Mifare auth and turn on crypto */
} pcdfeature_t;

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
 * @brief  Abstract ISO14443 PCD Virtual Method Table
 *
 * @note       use macros defined in this file to call these functions for
 *             convenience. Name of the macro is the same as a name of this
 *             function with `pcd` prefix.
 */
struct BasePcdVMT {
    /**
     * @brief     Returns the device state
     *
     * @note      This function can be called in all states.
     *
     * @param[in] inst Pointer to a Pcd structure
     *
     * @return    State of the device
     */
    pcdstate_t  (*getStateAB)(void *inst);

    /**
     * @brief     Activates RF Field of the PCD
     *
     * @note      This function can be called in the following states:
     *              - ::PCD_RF_OFF
     *
     * @param[in] inst Pointer to a Pcd structure
     *
     * @retval    PCD_OK RF Field successfully activated. Pcd transitions
     *                   to ::PCD_READY state.
     * @retval    PCD_BAD_STATE RF Field cannot be activated now or is already
     *                          active. State won't change.
     * @retval    PCD_ERROR An error has occured. State won't change.
     */
    pcdresult_t (*activateRFAB)(void *inst);

    /**
     * @brief     Deactivates the RF Field of the PCD
     *
     * @note      This function can be called in the following states:
     *              - ::PCD_READY
     *
     * @param[in] inst Pointer to a Pcd structure
     *
     * @retval    PCD_OK Field successfully deactivated. Pcd transitions to
     *                   ::PCD_RF_OFF state.
     * @retval    PCD_BAD_STATE RF Field cannot be deactivated now or is
     *                          already inactive. State won't change.
     * @retval    PCD_ERROR An error has occured. State won't change.
     */
    pcdresult_t (*deactivateRFAB)(void *inst);

    /**
     * @brief      Returns structure with supported features of this PCD
     *
     * @note       This function can be called in all states
     *
     * @param[in]  inst Pointer to a Pcd structure
     *
     * @return     Pointer to a PcdSParams structure. Content of this structure
     *             should not be modified.
     */
    PcdSParams *(*getSupportedParamsAB)(void *inst);

    /**
     * @brief      Sets communication parameters.
     *
     * If the parameter combination is unsupported #PCD_UNSUPPORTED
     * will be returned. It is advised to first check the value
     * returned by #getSupportedParamsAB.
     *
     * @note       This function can be called in the following states:
     *               - ::PCD_READY
     *               - ::PCD_RF_OFF
     *
     * @param[in]  inst    pointer to a Pcd strucutre
     * @param[in]  rx_spd  desired reception speed
     * @param[in]  tx_spd  desired transmission speed
     * @param[in]  mode    desired communication mode
     *
     * @retval     PCD_OK Parameters applied successfully. State won't change.
     * @retval     PCD_BAD_STATE Parameters can't be changed now. State
     *                           won't change.
     * @retval     PCD_UNSUPPORTED Some requested parameters are not supported
     *             byt this PCD. State won't change.
     * @retval     PCD_ERROR An error has occured. State won't change.
     */
    pcdresult_t (*setParamsAB)(void *inst, pcdspeed_rx_t rx_spd,
                               pcdspeed_tx_t tx_spd, pcdmode_t mode);

    /**
     * @brief      Transmits a 'Short Frame' and blocks until the response is
     *             received or a timeout occurs.
     *
     * Short frame transmits 7 data bits without parity. Therefore only
     * 7 Least Significant Bits of parameter @p data are sent.
     * This function discards remaining data in the response buffer, if any.
     *
     * @note       This function can be called in the following states:
     *               - ::PCD_READY
     *
     * @note       This function call either returns immediatelly with error
     *             or will change the state ::PCD_ACTIVE when the operation
     *             is in progress. Unless noted otherwise, the state returns
     *             to ::PCD_READY after this function returns.
     *
     * @param      inst        pointer to a Pcd structure
     * @param      data        7 bits of data to be sent (MSB is ignored)
     * @param      resp_len_p  Length of the received response
     *                         (see #transceiveStandardFrameA).
     *
     * @retval     PCD_OK Transmission successful and response received.
     * @retval     PCD_OK_COLLISION Transmission successful and multiple
     *                              responses received.
     * @retval     PCD_OK_TIMEOUT Transmission successful but no response was
     *                            received.
     * @retval     PCD_BAD_STATE Can't transmit right now. The state won't
     *                           change.
     * @retval     PCD_ERROR A driver od hardware error has occured.
     * @retval     PCD_TX_ERROR A transmission error has occured.
     * @retval     PCD_RX_ERROR A reception error has occured.
     * @retval     PCD_RX_OVERFLOW Too much data received.
     */
    pcdresult_t (*transceiveShortFrameA)(void *inst, uint8_t data,
                                         uint16_t *resp_length);


    /**
     * @brief      Transmits a 'Standard Frame' and blocks until the response is
     *             ready.
     *
     * Standard frame transmits `n` (where `n >= 1`) bytes.
     * The @p len parameter cannot be greater than maximum buffer size
     * supported by the reader. After the response is received
     * @p resp_len_p will be set to the received response length. You
     * can then use this parameter to allocate a new buffer and obtain
     * the response using the #getResponseAB function.
     * This function discards remaining data in the response buffer, if any.
     *
     * @note       This function can be called in the following states:
     *               - ::PCD_READY
     *
     * @note       This function call either returns immediatelly with error
     *             or will change the state ::PCD_ACTIVE when the operation
     *             is in progress. Unless noted otherwise, the state returns
     *             to ::PCD_READY after this function returns.
     *
     * @param      inst        pointer to a Pcd structure
     * @param      buffer      Bytes to send in a standard frame
     * @param      len         Size of the @p buffer
     * @param      resp_len_p  Size of received response
     *
     * @retval     PCD_OK Transmission successful and response received.
     * @retval     PCD_OK_COLLISION Transmission successful and multiple
     *                              responses received.
     * @retval     PCD_OK_TIMEOUT Transmission successful but no response was
     *                            received.
     * @retval     PCD_BAD_STATE Can't transmit right now. The state won't
     *                           change.
     * @retval     PCD_ERROR A driver od hardware error has occured.
     * @retval     PCD_TX_ERROR A transmission error has occured.
     * @retval     PCD_RX_ERROR A reception error has occured.
     * @retval     PCD_RX_OVERFLOW Too much data received.
     * @retval     PCD_TX_OVERFLOW Can't transmit this much data. The state
     *                             won't change.
     */
    pcdresult_t (*transceiveStandardFrameA)(void *inst, uint8_t* buffer,
                                            uint16_t length,
                                            uint16_t *resp_length);

    /**
     * @brief      Transmits the first part of an 'Anticollision Frame' and blocks
     *             until the response is ready.
     *
     * Anticollision frame is a standard 7-byte frame split anywhere
     * after 16th bit and before 55th bit. First part is transmitted
     * by the PCD and the second part is transmitted by the PICC as
     * a part of an anticollision sequence.
     * For @p resp_len_p meaning see pcdTransceiveStdFrameA.
     * This function discards remaining data in the response buffer, if any.
     *
     * @note       This function can be called in the following states:
     *               - ::PCD_READY
     *
     * @note       This function call either returns immediatelly with error
     *             or will change the state ::PCD_ACTIVE when the operation
     *             is in progress. Unless noted otherwise, the state returns
     *             to ::PCD_READY after this function returns.
     *
     * @param      inst         pointer to a Pcd structure
     * @param      buffer       Bytes to send in an anticoll frame
     * @param      len          Size of the buffer
     * @param      n_last_bits  Number of valid bits in the last byte to be
     *                          transmitted. 0 means the whole byte is valid.
     * @param      resp_len_p   Size of the received response
     *
     * @retval     PCD_OK Transmission successful and response received.
     * @retval     PCD_OK_COLLISION Transmission successful and multiple
     *                              responses received.
     * @retval     PCD_OK_TIMEOUT Transmission successful but no response was
     *                            received.
     * @retval     PCD_BAD_STATE Can't transmit right now. The state won't
     *                           change.
     * @retval     PCD_ERROR A driver od hardware error has occured.
     * @retval     PCD_TX_ERROR A transmission error has occured.
     * @retval     PCD_RX_ERROR A reception error has occured.
     * @retval     PCD_RX_OVERFLOW Too much data received.
     * @retval     PCD_TX_OVERFLOW Can't transmit this much data. The state
     *                             won't change.
     */
    pcdresult_t (*transceiveAnticollFrameA)(void *inst, uint8_t* buffer,
                                            uint16_t length,
                                            uint8_t n_last_bits,
                                            uint16_t *resp_length);

    /**
     * @brief      Gets (remaining) size of response stored in the buffer, if
     *             any.
     *
     * @note       This function can be called in the following states:
     *               - ::PCD_READY
     *               - ::PCD_RF_OFF
     *
     * @param      inst        pointer to a Pcd structure
     *
     * @return     Number of bytes in the response buffer. `0` if the response
     *             buffer is empty.
     */
    uint16_t (*getResponseLengthA)(void *inst);

    /**
     * @brief      Read response from the internal response buffer.
     *
     * Read response from the internal buffer. If size of the buffer
     * passed to this function is smaller than the response size
     * only part of the response will be copied and this function must
     * be called several times. Bytes received first are copied first.
     * @p n_last_bits_p is valid only when copying last part of the
     * response.
     *
     * @note       This function can be called in the following states:
     *               - ::PCD_READY
     *               - ::PCD_RF_OFF
     *
     * @param[in]  inst           pointer to a Pcd structure
     * @param[in]  buf_size       Max size to read
     * @param[out] buffer         The buffer
     * @param[out] size_copied    Number of bytes written to the buffer
     * @param[out] n_last_bits_p  Number of valid bits in last byte copied to the
     *                            buffer.
     *
     * @retval     PCD_OK Bytes were copied
     * @retval     PCD_ERROR No more bytes to copy
     * @retval     PCD_BAD_STATE This function can't be called in this state.
     */
    pcdresult_t (*getResponseAB)(void *inst, uint16_t buffer_size,
                                 uint8_t *buffer, uint16_t *size_copied,
                                 uint8_t *n_last_bits);

    /**
     * @brief      Acquires exclusive access to the PCD.
     *
     * @param[in] inst pointer to a Pcd structure
     */
    void (*acquireBus)(void *inst);

    /**
     * @brief      Acquires exclusive access to the PCD.
     *
     * @param[in] inst pointer to a Pcd structure
     */
    void (*releaseBus)(void *inst);

    /**
     * @brief      Checks whether this PCD supports the given extended feature.
     *
     * @param[in] inst pointer to a Pcd structure
     */
    bool (*supportsExtFeature)(void *inst, pcdfeature_t feature);

    /**
     * @brief      Invokes an extended feature.
     *
     * @param[in] inst pointer to a Pcd structure
     * @param[in] feature identifier of the feature
     * @param[in] params parameters passed to the feature function. Can be NULL
     *                   if function doesn't expect any.
     * @param[out] result result of the feature function.
     *
     * @retval     PCD_OK Feature function executed successfully
     * @retval     PCD_BAD_STATE Feature function can't be executed now
     * @retval     PCD_UNSUPPORTED This feature is not supported
     */
    pcdresult_t (*callExtFeature)(void *inst, pcdfeature_t feature,
                                  void *params, void *result);
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
 * @name    Macro functions (Pcd)
 * @details Convenience macros for easy calling of 'member functions'
 * @{
 */

/**
 * @see BasePcdVMT.getStateAB
 */
#define pcdGetStateAB(ip) ((ip)->vmt->getStateAB(ip))

/**
 * @see BasePcdVMT.activateRFAB
 */
#define pcdActivateRFAB(ip) ((ip)->vmt->activateRFAB(ip))

/**
 * @see BasePcdVMT.deactivateRFAB
 */
#define pcdDeactivateRFAB(ip) ((ip)->vmt->deactivateRFAB(ip))

/**
 * @see BasePcdVMT.getSupportedParamsAB
 */
#define pcdGetSupportedParamsAB(ip) ((ip)->vmt->getSupportedParamsAB(ip))

/**
 * @see BasePcdVMT.setParamsAB
 */
#define pcdSetParamsAB(ip, rx_spd, tx_spd, mode)                              \
        ((ip)->vmt->setParamsAB(ip, rx_spd, tx_spd, mode))

/**
 * @see BasePcdVMT.transceiveShortFrameA
 */
#define pcdTransceiveShortFrameA(ip, data, resp_len_p)                        \
        ((ip)->vmt->transceiveShortFrameA(ip, data, resp_len_p))

/**
 * @see BasePcdVMT.transceiveStandardFrameA
 */
#define pcdTransceiveStandardFrameA(ip, data, size, resp_len_p)               \
        ((ip)->vmt->transceiveStandardFrame(ip, data, size, resp_len_p))

/**
 * @see BasePcdVMT.transceiveAnticollFrameA
 */
#define pcdTransceiveAnticollFrameA(ip, data, size, n_last_bits, resp_len_p)  \
        ((ip)->vmt->transceiveAnticollFrameA(ip, data, size, n_last_bits,     \
                                             resp_len_p))
/**
 * @see BasePcdVMT.getResponseLengthA
 */
#define pcdGetRespLengthA(ip)  ((ip)->vmt->getResponseLengthA(ip))

/**
 * @see BasePcdVMT.getResponseAB
 */
#define pcdGetRespAB(ip, buf_size, buffer, size_copied, n_last_bits_p)        \
        ((ip)->vmt->getResponseAB(ip, buf_size, buffer, size_copied,          \
                                  n_last_bits_p))

/**
 * @see BasePcdVMT.acquireBus
 */
#define pcdAcquireBus(ip)  ((ip)->vmt->acquireBus(ip))

/**
 * @see BasePcdVMT.releaseBus
 */
#define pcdReleaseBus(ip)  ((ip)->vmt->releaseBus(ip))

/**
 * @see BasePcdVMT.supportsExtFeature
 */
#define pcdSupportsExtFeature(ip, feature)                                    \
        ((ip)->vmt->supportsExtFeature(ip, feature))

/**
 * @see BasePcdVMT.callExtFeature
 */
#define pcdCallExtFeature(ip, feature, params, result)                        \
        ((ip)->vmt->callExtFeature(ip, feature, params, result))

/** @} */

#endif /* HAL_ABSTRACT_ISO14443_PCD */

/** @} */
