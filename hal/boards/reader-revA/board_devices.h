#ifndef _BOARD_DEVICES_H
#define _BOARD_DEVICES_H

#include "hal.h"
#include "hal_custom.h"

// Devices present on this board

// Object representing a MFRC522 module on the board
extern Mfrc522Driver MFRC522;
// Object representing a generic PCD MFRC522 module
extern Pcd *PCD;
// Watchdog
extern WDGConfig wdgcfg;

#endif /* _BOARD_DEVICES_H */
