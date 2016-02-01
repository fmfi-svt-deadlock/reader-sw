#include <string.h>
#include "iso14443com.h"
#include "mfrc522.h"

/*
 * Note: This may not make sense to you unless you are familiar with
 * the ISO/IEC 14443a standard.
 */

/*
 * THIS FILE IS A ONE BIG TODO!
 *
 * This is just a C port of a previously-written driver in Python for the ISO/IEC 14443a cards. It is not optimized,
 * hard to maintain, does not utilize facilities of ChibiOS, and just generally sucks. Will be rewritten ASAP.
 *
 * This file should be *burned to death* because of *usage of dark magic* constants.
 */

typedef enum {
    REQA        = 0x26,
    ANTICOLL_1  = 0x93,
    ANTICOLL_2  = 0x95,
    ANTICOLL_3  = 0x97,
} CardCommand;

static int8_t dlCardPerformCascade(uint8_t *id) {
    uint8_t buffer[64];
    int16_t rx_status;
    int8_t id_size = 0;

    for (int i = 0; i < 3; i++) {
        uint8_t cascade_level;
        switch (i) {
            case 0:
                cascade_level = ANTICOLL_1;
                break;
            case 1:
                cascade_level = ANTICOLL_2;
                break;
            case 2:
                cascade_level = ANTICOLL_3;
                break;
            default:
                cascade_level = ANTICOLL_1;
                break;
        }

        buffer[0] = cascade_level;
        buffer[1] = 0x20;

        // Transmit ANTICOLLISION
        rx_status = dlMfrc522Transceive(buffer, 2, (buffer+2), 10);

        uint8_t uid_cln[5];

        if (rx_status == MFRC522_TRX_NOCARD) return CARD_NOT_PRESENT;
        if (rx_status == MFRC522_TRX_ERROR || rx_status != 5) return CARD_ERROR;

        memcpy(uid_cln, (buffer+2), 5);

        // Transmit SELECT
        buffer[1] = 0x70;
        uint16_t crc = dlMfrc522CalculateCRCA(buffer, 2u+(uint8_t )rx_status);
        buffer[2+rx_status] = (uint8_t)(crc & 0xFF);
        buffer[2+rx_status+1] = (uint8_t)((crc & 0xFF00) >> 8);

        rx_status = dlMfrc522Transceive(buffer, 4u+(uint8_t )rx_status, buffer, 10);

        if (rx_status == MFRC522_TRX_NOCARD) return CARD_NOT_PRESENT;
        if (rx_status == MFRC522_TRX_ERROR) return CARD_ERROR;

        if ((buffer[0] & 0x24) == 0x00) {
            // UID complete, PICC is NOT compliant with ISO/IEC 14443-4, but we don't care at this point
            memcpy(uid_cln, id, 4);
            id_size += 4;
            return id_size;
        } else if ((buffer[0] & 0x24) == 0x20) {
            // UID incomplete, PICC compliant with ISO/IEC 14443-4
            memcpy(uid_cln, id, 4);
            id_size += 4;
            return id_size;
        } else if ((buffer[0] & 0x04)) {
            // UID incomplete, continue the cascade
            memcpy(uid_cln+1, id, 3);
            id += 3;
            id_size += 3;
        } else {
            return CARD_ERROR;
        }
    }

    return CARD_ERROR;
}

int8_t dlCardGetId(uint8_t *id) {
    dlMfrc522WriteRegister(BitFramingReg, 0x07);
    uint8_t buffer[5];
    buffer[0] = REQA;
    int16_t status = dlMfrc522Transceive(buffer, 1, buffer, 0);
    if (status == MFRC522_TRX_NOCARD) return CARD_NOT_PRESENT;
    if (status == MFRC522_TRX_ERROR) return CARD_ERROR;
    dlMfrc522WriteRegister(BitFramingReg, 0x00);
    return dlCardPerformCascade(id);
}