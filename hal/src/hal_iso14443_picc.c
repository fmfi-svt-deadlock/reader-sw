#include "hal.h"
#include "hal_custom.h"

#if (HAL_USE_ISO14443_PICC == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

typedef enum {
	PICC_SEL_UID_INCOMPLETE,
	PICC_SEL_COMPLETE_COMPLIANT,
	PICC_SEL_COMPLETE_NONCOMPLIANT,
	PICC_SEL_TIMEOUT,
	PICC_SEL_ERROR
} PiccSelectResult;

// Commands
#define      			ISO14443_REQA				0x26
#define      			ISO14443_WUPA				0x52
static const uint8_t 	ISO14443_SEL[] = 			{0x93, 0x95, 0x97};
#define      			ISO14443_HLTA				0x50

// Protocol
#define 				SAK_UID_INCOMPLETE			2
#define 				SAK_COMPLETE_COMPLIANT		5

// Frame Delay Times
// For REQA, WUPA, SEL and HLTA the standard mandates FDT to be:
//   - 1236 / fc if last bit is 1 (~92 us)
//   - 1172 / fc if last bit is 0 (~87 us)
// fc = 13.56 MHz +- 7kHz
// For ease of implementation, we will just go with 100us
#define 				ISO14443_FDT				100

// How long does it take to receive num_bytes at 106kBaud (anticoll speed)
// 85 uS per byte (including parity bit) + 19 uS for start and stop bits
#define ISO14443_RX_TIME(num_bytes)  (85*(num_bytes) + 19)

// Additional wait time due to reader, driver and OS overhead
#define                 ISO14443_ADDITIONAL_WAIT	50

// Cards should respond to WUPA with ATQA, 2 bytes
#define 				ISO14443_WUPA_FDT	(ISO14443_FDT +                   \
                                             ISO14443_ADDITIONAL_WAIT +       \
                                             ISO14443_RX_TIME(2))

// Cards should respond to REQA with ATQA, 2 bytes
#define 				ISO14443_REQA_FDT	(ISO14443_FDT +                   \
                                             ISO14443_ADDITIONAL_WAIT +       \
                                             ISO14443_RX_TIME(2))

// Cards should respond to ANTICOLLISION with UID CLn, 5 bytes
#define 				ISO14443_ANTI_FDT	(ISO14443_FDT +                   \
                                             ISO14443_ADDITIONAL_WAIT +       \
                                             ISO14443_RX_TIME(5))

// Cards should respond to SELECt with SAK, 3 bytes
#define 				ISO14443_SAK_FDT	(ISO14443_FDT +                   \
                                             ISO14443_ADDITIONAL_WAIT +       \
                                             ISO14443_RX_TIME(3))

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

// TODO unit tests!
static PiccSelectResult select(Pcd *pcd, uint8_t uid_cln[5], unsigned int anticoll_level) {

	// SELECT command should be transmitted in a standard frame with CRC.
	// Enable automatic CRC gen and verification.
	pcdSetParamsAB(pcd, PCD_RX_SPEED_106, PCD_TX_SPEED_106, PCD_ISO14443_A, true, true);

	uint8_t select_command[7] = {
		ISO14443_SEL[anticoll_level],
		0x70, 							// In select command all bits are valid
		uid_cln[0], uid_cln[1], uid_cln[2], uid_cln[3], uid_cln[4]
	};
	uint16_t response_length;
	pcdresult_t result = pcdTransceiveStandardFrameA(pcd, select_command,
	                                                 sizeof(select_command), &response_length,
	                                                 500000);

	// Restore default anticollision settings (turn off CRC generation and verification)
	pcdSetParamsAB(pcd, PCD_RX_SPEED_106, PCD_TX_SPEED_106, PCD_ISO14443_A, false, false);

	if (result == PCD_OK) {
		// We should receive a SAK command, length 1 byte. Otherwise we have a protocol error.
		if (response_length != 1) {
			return PICC_SEL_ERROR;
		}
		uint16_t size_copied;
		uint8_t n_last_bits;
		uint8_t sak;
		result = pcdGetRespAB(pcd, 1, &sak, &size_copied, &n_last_bits);

		if (result != PCD_OK) {
			return PICC_SEL_ERROR;
		}

		if (sak & (1 << SAK_UID_INCOMPLETE)) {
			return PICC_SEL_UID_INCOMPLETE;
		}

		if (sak & (1 << SAK_COMPLETE_COMPLIANT)) {
			return PICC_SEL_COMPLETE_COMPLIANT;
		} else {
			return PICC_SEL_COMPLETE_NONCOMPLIANT;
		}

	} else if (result == PCD_OK_TIMEOUT) {
		return PICC_SEL_TIMEOUT;
	}

	return PICC_SEL_ERROR;
}

// TODO unit test the heck out of this thing
// Warning: This implementation is known to be buggy, since it is a work in progress.
// It often gets caught in an infinite loop and does not handle error cases at all.
// Don't even think about production until that is fixed!
static unsigned int anticoll(Pcd *reader, Picc *found_cards,
                             unsigned int max_cards, bool *is_that_all,
                             unsigned int anticoll_level, uint8_t *uid_prefix_cl1,
                             uint8_t *uid_prefix_cl2) {

    // Ensure the PCD is running at standard anticollision speeds and settings
    pcdSetParamsAB(reader, PCD_RX_SPEED_106, PCD_TX_SPEED_106,
                   PCD_ISO14443_A, false, false);

	// Anticollision command, number of valid bits and known part of the UID
	// CLn.
	uint8_t anticoll_command[7] = {0};

	// Shortcut to known UID CLn from the anticoll command
	uint8_t *uid_cln = (anticoll_command + 2);

	// Each bit here indicates that a collision has occured on this position
	// and value '1' was set in the known UID on this position.
	uint32_t collision_map = 0;

	// Number of bits of the UID CLn we already know
	uint8_t valid_bits = 0;

	// Number of cards we've already found
	unsigned int n_found_cards = 0;

	// How many times have we tried this?
	unsigned int retry_count = 0;

	// ANTICOLLISION command code
	anticoll_command[0] = ISO14443_SEL[anticoll_level];

	// Anticollision frame is a 7-byte frame in the following form:
	// | CMD | NVB | UID0 | UID1 | UID2 | UID3 | BCC |
	//   - CMD is the command
	//   - NVB: Number of Valid Bits
	//   - UIDx: Part of the UID CLn (UID in Cascade Level n)
	//   - BCC: Checksum, all previous bytes XORed
	//
	// This frame may be split into two parts anywhere after the
	// second byte and before the last byte. First part is transmitted
	// by the PCD, second part is transmitted by the PICC.
	//
	// There may be a collision in the second part if there are multiple
	// PICCs in the RF field. In that case, we will store received valid
	// bits, and remember the collision position. Then we will transmit the
	// first part of the anticoll frame again, with known bits and the collided
	// bit set to 1. Only PICCs UIDs of which match this prefix will respond
	// again with the second part of the frame. There may be a collision again
	// so we repeat the process in a loop.
	//
	// When we know the full ID we append a CRC to the full anticoll frame.
	// This is called the SELECT command, which will activate the PICC
	// (or tell us to proceed to the next cascade).
	//
	// After that, we go back to the last collision position where we
	// previously set the collided bit to 1. This time we set it to 0 and
	// repeat the whole process.
	//
	// This way we can get UIDs of all cards in the RF field. If the number
	// of found cards exceeds `max_cards` then we simply won't mark the
	// collision position and we will set the collided bit to '1'. That way
	// we ignore all cards with lower IDs.

	while(true) {

		// --- First, send Anticollision frame with part of UID CLn we
		//     already know

		// Number of Valid Bits. Upper nibble: valid bytes (command and NVB
		// bytes themselves). Lower nibble: additional valid bits
		uint8_t valid_bytes = (valid_bits / 8);
		uint8_t bits_in_incomplete_byte = (valid_bits % 8);
		anticoll_command[1] = ((2 + valid_bytes) << 4) |
		                      bits_in_incomplete_byte;

		// Number of bytes to transmit.
		//   - Command and Number of Valid Bits (2)
		//   - Known part of the UID CLn
		//   - 1 additional byte if there are some valid bits in it
		uint8_t cmd_len = 2 + (valid_bits / 8) +
		                  (bits_in_incomplete_byte == 0 ? 0 : 1);

		uint16_t response_length;
		pcdresult_t r = pcdTransceiveAnticollFrameA(reader, anticoll_command,
		                                            cmd_len,
		                                            bits_in_incomplete_byte,
		                                            bits_in_incomplete_byte,
		                                            &response_length,
		                                            500000);

		// --- Copy valid received bits, handle possible errors

		if (r == PCD_OK || r == PCD_OK_COLLISION) {
			retry_count = 0;

			// We have received a response. If we have transmitted incomplete
			// byte in the anticoll command the first byte of the response
			// will be the rest of this incomplete byte due to the way we align
			// the received bits. Otherwise it will be a new byte.

			uint8_t incomplete_byte_backup = uid_cln[valid_bytes];
			uint16_t size_copied;
			uint8_t valid_last_bits_in_resp;

			// Append the response to already known UID CLn. Max length of
			// UID CLn is 4 bytes + 1 BSS byte (5 bytes total).
			// If there was a collision it should be somewhere in the first
			// 4 bytes, therefore valid_last_bits_in_resp is always correct
			// in this case.
			// If the collision is not in the first 4 bytes then it is in the
			// BSS bytes, which means that two PICCs computed different XOR
			// of the same bytes (therefore there is a severe PICC error,
			// transmission error or a missed collision).
			// TODO handle that case, however improbable it may be!.
			pcdresult_t r2 = pcdGetRespAB(reader, 5, uid_cln + valid_bytes,
			                              &size_copied,
			                              &valid_last_bits_in_resp);

			if (r2 != PCD_OK) {
				osalSysHalt("Internal PCD driver error!");
			}

			if (bits_in_incomplete_byte) {
				// Appending the response has overwritten the last incomplete
				// byte. Restore known part of this byte.
				// first, zero-out previously known bits
				uid_cln[valid_bytes] &= ~((1 << bits_in_incomplete_byte) - 1);
				// zero-out unknown bits in the backup
				incomplete_byte_backup &= (1 << bits_in_incomplete_byte) - 1;
				// restore known bits from the backup
				uid_cln[valid_bytes] |= incomplete_byte_backup;
			}

			// Increase the number of known bits by the number of new bits
			// we've received right now
			uint8_t received_new_bits = ((size_copied - 1) * 8) + valid_last_bits_in_resp;

			// If there were some bits in an incomplete byte we've received
			// the whole byte back, including those bits (their values were
			// invalid, but they were there), due to the way we align the
			// response. Therefore they should not be present in
			// `received_new_bits`.
			if (bits_in_incomplete_byte) {
				received_new_bits -= bits_in_incomplete_byte;
			}

			// TODO handle case where we receive 0 valid bytes
			valid_bits += received_new_bits;

			if (r == PCD_OK_COLLISION) {
				// There was a collision. We should mark its position and
				// set the collided bit to '1'
				collision_map |= (1 << valid_bits);
				uid_cln[valid_bits / 8] |= (1 << (valid_bits % 8));
				valid_bits++;
			}

		} else if (r == PCD_OK_TIMEOUT) {
			// Card has disappeared. Continue the loop as if it never existed
			retry_count = 0;
		} else if (r == PCD_TX_ERROR || r == PCD_RX_ERROR) {
			if (retry_count > 3) {
				// We shouldn't continue in this case. Abort.
				return n_found_cards;
			} else {
				retry_count++;
			}
		} else {
			// well, fuck. Abort.
			return n_found_cards;
		}

		// --- Check if we know the complete UID CLn. If we do, issue a
		//     SELECT command. Additionally we will break the loop if there
		//     are no collided bits pending. If there are, we will find them,
		//     set it to zero and continue the anticoll loop.

		if (valid_bits == 40) {
			// We have the whole UID CLn, issue a SELECT command
			PiccSelectResult sel_result = select(reader, uid_cln, anticoll_level);

			unsigned int next_n_cards;

			switch (sel_result) {
				case PICC_SEL_UID_INCOMPLETE:
					switch(anticoll_level) {
						case 0:
							next_n_cards = anticoll(reader, found_cards, max_cards - n_found_cards,
					         						is_that_all, anticoll_level + 1, uid_cln, NULL);
							break;
						case 1:
							next_n_cards = anticoll(reader, found_cards, max_cards - n_found_cards,
							   						is_that_all, anticoll_level + 1, uid_prefix_cl1,
							   						uid_cln);
							break;
						case 2:
							// There should be no IDs longer than 10 bytes. This is a severe
							// protocol error, ignore the card.
							break;
					}

					n_found_cards += next_n_cards;
					found_cards += next_n_cards;
					break;
				case PICC_SEL_COMPLETE_COMPLIANT:
				case PICC_SEL_COMPLETE_NONCOMPLIANT:
					// UID is complete (at this point we don't care whether the card is compliant
					// with ISO14443-4 or not).

					n_found_cards++;

					// Copy part of the UID obtained in this cascade to appropriate place.
					// When copying uid_prefix_cln we are skipping the first byte, which is
					// a Cascade Tag and not actually a UID byte.
					switch(anticoll_level) {
						case 0:
							memcpy(found_cards->uid, uid_cln, 4);
							found_cards->uid_len = PICC_UID_4;
							break;
						case 1:
							memcpy(found_cards->uid, uid_prefix_cl1 + 1, 3);
							memcpy(found_cards->uid + 3, uid_cln, 4);
							found_cards->uid_len = PICC_UID_7;
							break;
						case 2:
							memcpy(found_cards->uid, uid_prefix_cl1 + 1, 3);
							memcpy(found_cards->uid + 3, uid_prefix_cl2 + 1, 3);
							memcpy(found_cards->uid + 6, uid_cln, 4);
							found_cards->uid_len = PICC_UID_10;
							break;
					}
					found_cards->iso_compliant = (sel_result == PICC_SEL_COMPLETE_COMPLIANT);

					break;
				case PICC_SEL_TIMEOUT:
				case PICC_SEL_ERROR:
					// Well, let's just ignore the card. Or retry, maybe? TODO.
					break;
			}

			if (collision_map == 0) {
				// It's done.
				return n_found_cards;
			} else {
				// Find the most significant collision bit
				uint8_t coll_pos = 32;
				while(!(collision_map & (1 << coll_pos))) {
					if (coll_pos == 0) {
						osalSysHalt("Internal ISO14443 driver error!");
					}
					coll_pos--;
				}

				collision_map &= ~(1 << coll_pos);
				valid_bits -= 32 - coll_pos;
				uid_cln[coll_pos / 8] &= ~(1 << (coll_pos % 8));
				valid_bits++;

				// We are going to continue anticollision process with a card
				// UID of which previously didn't match. Receiving an
				// Anticollision frame with a non-matching UID has put the
				// card back to IDLE (or HALT) state. We need to wake it up
				// again for it to participate in a further anticollision.
				pcdresult_t act_result = pcdTransceiveShortFrameA(reader, ISO14443_WUPA,
	                                                  			  &response_length,
	                                                  			  ISO14443_WUPA_FDT);

				// Now all cards are woken up. However, if this is not the first level of the
				// anticoll cascade only cards which passed through previous cascades should be
				// woken up. Therefore we will transmit SELECT commands with parts of the UID
				// obtained in previous anticollision cascade levels. This will put cards with
				// different ID prefix back to sleep mode where they belong.
				// TODO handle result codes
				if (uid_prefix_cl1 != NULL) {
					PiccSelectResult sel_result = select(reader, uid_prefix_cl1, 0);
				}
				if (uid_prefix_cl2 != NULL) {
					PiccSelectResult sel_result = select(reader, uid_prefix_cl2, 1);
				}

				if (act_result != PCD_OK && act_result != PCD_OK_COLLISION) {
					// TODO we cannot continue in this case.
				}
			}
		}

	}

}

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
	                                                  &resp_len,
	                                                  1000000);

	if (act_result != PCD_OK && act_result != PCD_OK_COLLISION) {
		// There are no cards or there was an error. Either way we couldn't
		// find any cards.
		return 0;
	}

	// Start a recursive anticollision loop
	return anticoll(reader, found_cards, max_cards, is_that_all, 0, NULL, NULL);
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
