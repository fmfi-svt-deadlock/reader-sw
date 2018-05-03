/**
 * @file    common.h
 * @brief   Common constants and data structures
 *
 * This file contains definitions of common constants and data structures that may be shared
 * between tasks.
 */

#ifndef _COMMON_H
#define _COMMON_H

#include "hal.h"
#include "hal_custom.h"

#define DL_MAX_CARDS_PER_POLL_CYCLE    10

/**
 * @brief       Card ID structure
 *
 * This structure carries the Card ID and information about how long the ID is.
 */
typedef struct {
    uint8_t     uid[MAX_PICC_UID_SIZE];
    PiccUidLen  uid_len;
} dl_picc_uid;

#endif
