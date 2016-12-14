#include "hal.h"
#include "hal_custom.h"

#if (HAL_USE_ISO14443_PICC == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

// Commands
#define      			ISO14443_REQA				0x26
#define      			ISO14443_WUPA				0x52
static const uint8_t 	ISO14443_SEL[] = 			{0x93, 0x95, 0x97};
#define      			ISO14443_HLTA				0x50

// Frame Delay Times
// For REQA, WUPA, SEL and HLTA the standard mandates FDT to be:
//   - 1236 / fc if last bit is 1 (~92 us)
//   - 1172 / fc if last bit is 0 (~87 us)
// fc = 13.56 MHz +- 7kHz
// For ease of implementation, we will just go with 100us
#define 				ISO14443_FDT				100

// How long does it take to receive num_bytes at 106kBaud (anticoll speed)
#define ISO14443_RX_TIME(num_bytes)  (85*(num_bytes) + 19)

// Additional wait time due to reader, driver and OS overhead
#define                 ISO14443_ADDITIONAL_WAIT	50

#define 				ISO14443_WUPA_FDT	(ISO14443_FDT +                   \
                                             ISO14443_ADDITIONAL_WAIT +       \
                                             ISO14443_RX_TIME(2))

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

void iso14443PiccInit() {

}

unsigned int iso14443FindCards(Pcd *reader, Picc *found_cards,
                               unsigned int max_cards, bool *is_that_all) {
	osalDbgCheck(reader != NULL);
	osalDbgCheck(found_cards != NULL);
	osalDbgCheck(is_that_all != NULL);
	osalDbgAssert(pcdGetStateAB(reader) == PCD_READY, "bad reader state");
	// TODO check whether there are cards active for this reader

	// Let's activate all cards (including the ones in HALT state), if any
	uint16_t resp_len;
	pcdresult_t act_result = pcdTransceiveShortFrameA(reader, ISO14443_WUPA,
	                                                  &resp_len, ISO14443_FDT);

	if (act_result == PCD_OK_TIMEOUT) {
		return 0;
	}

}

iso14443result_t iso14443ActivateCard(Picc *card) {
	(void)card;
	osalSysHalt("not implemented");
	return ISO14443_ERROR;
}

iso14443result_t iso14443DeactivateCard(Picc *active_card) {
	(void)active_card;
	osalSysHalt("not implemented");
	return ISO14443_ERROR;
}


#endif /* HAL_USE_ISO14443_PICC == TRUE */
