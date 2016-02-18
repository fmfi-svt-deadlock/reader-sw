/**
 * @file    hal_abstract_CRCard.h
 * @brief   Abstract Command-Response Card object.
 * @details This header defines an abstract interface used to communicate
 *          with an Integrated Circtuit Card (either with contacts or
 *          contactless) using request-response blocks. Each block is a request
 *          and should generate some response or a timeout. Example of such a
 *          card is ISO/IEC 14443 Proximity Integrated Circuit Card or
 *          ISO/IEC 7816 Integrated Circuit Card exchanging command-response
 *          pairs using T=0 or T=1 protocol.
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


#endif /* HAL_ABSTRACT_CRCARD */

/** @} */
