/**
 * @file    hal_iso14443_picc.h
 * @brief   ISO14443 Proximity Integrated Circuit Card driver code.
 * @details This is a driver for ISO/IEC 14443 Proximity Integrated
 *          Circuit Cards. It handles detection, initialization/anticollision
 *          and communication with these cards. It exports an CRCard object
 *          for use by other layers.
 *
 * @addtogroup ISO14443_PICC
 * @{
 */

#include "hal.h"
#include "hal_custom.h"

#if (HAL_USE_ISO14443_PICC == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

typedef enum {
	ISO14443_OK,
	ISO14443_NO_SUCH_CARD,
	ISO14443_TOO_MANY_ACTIVE_CARDS,
	ISO14443_READER_ERROR,
	ISO14443_ERROR
} iso14443result_t;

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

typedef struct {
	CRCard crcard;
} Picc;

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief      Initializes the ISO/IEC 14443 PICC driver.
 *
 * @note       This function is called implicitly by halCustomInit, no need
 *             to call it explicitly.
 */
void iso14443PiccInit(void);

/**
 * @brief      Finds and returns IDs of all cards in the RF field.
 *
 * Internally it performs an ISO/IEC 14443 Anticollision loop and detects
 * at most max_cards cards. If more cards than max_cards are present the ones
 * with higher IDs will be preferred.
 *
 * @param[in]  reader       The reader to use for scan
 * @param[out] found_cards  Array to write found cards to
 * @param[in]  max_cards    Maximum number of cards to be returned
 * @param[out] is_that_all  Indicates if there are more cards in the RF field
 *                          than were returned.
 *
 * @return     Number of returned cards in found_cards array.
 */
unsigned int iso14443FindCards(Pcd *reader, Picc *found_cards,
                               unsigned int max_cards, bool *is_that_all);

/**
 * @brief      Activates the given card, if possible. When the card is active
 *             it can be communicated with.
 *
 * After the activation you can use the crcard member of the Picc object
 * in CRCard API functions.
 *
 * Multiple cards *may* be activated simultaneously, if they support it.
 * Otherwise an error will be returned.
 *
 * @param      card  	Card to be activated
 *
 * @return     Result of the activation process
 */
iso14443result_t iso14443ActivateCard(Picc *card);

/**
 * @brief      Deactivates a card
 *
 * @param      active_card  The active card to be deactivated
 *
 * @return     Result of the deactivation process
 */
iso14443result_t iso14443DeactivateCard(Picc *active_card);

#ifdef __cplusplus
}
#endif


#endif /* HAL_USE_ISO14443_PICC */

/** @} */
