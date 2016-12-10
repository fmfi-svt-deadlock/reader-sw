/**
 * @file    hal_custom.h
 * @brief   HAL subsystem header (Deadlock custom drivers)
 *
 * @addtogroup HAL_CUSTOM
 * @{
 */

/* Error checks on the configuration header file. */
#ifndef HAL_CUSTOM_H
#define HAL_CUSTOM_H

#include "halconf.h"

#if !defined (HAL_USE_MFRC522)
#define HAL_USE_MFRC522             FALSE
#endif

#if !defined (HAL_USE_ISO14443_PICC)
#define HAL_USE_ISO14443A           FALSE
#endif

/* Abstract interfaces.*/
#include "hal_abstract_CRCard.h"
#include "hal_abstract_iso14443_pcd.h"
#include "hal_abstract_iso14443_pcd_ext.h"

/* Shared headers.*/

/* Normal drivers.*/

/* Complex drivers.*/
#include "hal_mfrc522.h"
#include "hal_iso14443_picc.h"

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif
  void halCustomInit(void);
#ifdef __cplusplus
}
#endif

#endif /* HAL_CUSTOM_H */

/** @} */
