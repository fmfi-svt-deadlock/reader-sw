/**
 * @file    hal_abstract_CRCard.h
 * @brief   Abstract Command-Response Card object.
 *
 * This header defines an abstract interface used to communicate
 * with an Integrated Circtuit Card (either with contacts or
 * contactless) using request-response frames. Each frame is a request
 * and should generate some response frame or a timeout. Example of such a
 * card is ISO/IEC 14443 Proximity Integrated Circuit Card or
 * ISO/IEC 7816 Integrated Circuit Card exchanging command-response
 * pairs using T=0 or T=1 protocol.
 *
 * For now only synchronous API is defined. Async API may be added later if
 * needed.
 *
 * @addtogroup CR_CARD
 * @details This module defines an abstract interface for using
 *          Command-Response cards.<br>
 *          Note that no code is present, just abstract interfaces-like
 *          structures, you should look at the system as to a set of
 *          abstract C++ classes (even if written in C). This system
 *          has then advantage to make the access to block devices
 *          independent from the implementation logic.
 * @{
 */

#ifndef HAL_ABSTRACT_CRCARD
#define HAL_ABSTRACT_CRCARD

#include "hal.h"
#include "hal_custom.h"

/**
 * @brief      Operation result codes
 */
typedef enum {
    CRCARD_OK,              /**< Transmission successful, received response  */
    CRCARD_TX_ERROR,        /**< Unrecoverable transmission errror           */
    CRCARD_RX_ERRPR,        /**< Unrecoverable reception error               */
    CRCARD_TIMEOUT,         /**< Transmission successful, no response        */
    CRCARD_NONEXISTENT      /**< Card removed, no further comm possible      */
} crcard_result_t;

/**
 * @brief      Abstract CRCard Virtual Method Table
 *
 * @note       use macros defined in this file to call these functions for
 *             convenience. Name of the macro is the same as a name of this
 *             function with `crcard` prefix.
 */
struct CRCardVMT {
    /**
     * @brief      Send a frame of data to a card and wait for a response.
     *
     * The response will be stored in a response buffer. Invoking this function
     * will clear contents of the response buffer, if any.
     *
     * @param[in] inst Pointer to a CRCard structure
     * @param[in] tx_buffer Data to send
     * @param[in] tx_buffer_size Number of bytes to send
     * @param[out] response_size Number of bytes received
     *
     * @return     Operation result
     */
    crcard_result_t (*transceive)(void* inst, uint8_t *tx_buffer,
                                  uint16_t tx_buffer_size,
                                  uint16_t *response_size);

    /**
     * @brief      Get number of remaining bytes in the response buffer.
     *
     * @param[in] inst Pointer to a CRCard structure
     *
     * @return     Number of bytes in the response buffer
     */
    uint16_t (*getResponseSize)(void *inst);

    /**
     * @brief      Retrieves a response from the response buffer.
     *
     * Think of the response buffer as a queue. When this function is called
     * no more than @p buffer_size bytes are removed from this queue and copied
     * to @p data buffer.
     *
     * Function #getResponseSize returns number of bytes in this queue.
     *
     * @param[in] inst Pointer to a CRCard structure
     * @param[out] data Buffer to copy the response to
     * @param[in] buffer_size Maximum number of bytes to copy
     *
     * @return     Number of copied bytes
     */
    uint16_t (*getResponse)(void *inst, uint8_t *data, uint16_t buffer_size);
};

/**
 * @brief   Base Command-Response Card
 * @details This class represents a generic Command-Response Card.
 */
typedef struct {
  const struct CRCardVMT *vmt;              /**< Virtual Methods Table       */
  void *data;                               /**< Private data of a driver    */
} CRCard;

/**
 * @name    Macro functions (Pcd)
 * @details Convenience macros for easy calling of 'member functions'
 * @{
 */

/**
 * @see CRCardVMT.transceive
 */
#define crcardTransceive(inst, tx_buf_p, tx_buf_size, resp_size_p)            \
        ((inst)->vmt->transceive(isnt, tx_buf_p, tx_buf_size, resp_size_p))

/**
 * @see CRCardVMT.getResponseSize
 */
#define crcardGetResponseSize(inst)  ((inst)->vmt->getResponseSize(inst))

/**
 * @see CRCardVMT.getResponse
 */
#define crcardGetResponse(inst, buf_p, max_buf_size)                          \
        ((inst)->vmt->getResponse(inst, buf_p, max_buf_size))

/** @} */

#endif /* HAL_ABSTRACT_CRCARD */

/** @} */
