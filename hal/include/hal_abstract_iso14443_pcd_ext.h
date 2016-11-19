/**
 * @file    hal_abstract_iso14443_pcd_ext.h
 * @brief   Structures for extended / optional features of an abstract
 *          ISO/IEC 14443 PCD.
 * @details Real-world ISO/IEC 14443 PCDs (card readers) support a number of
 *          extended features which are not covered by the abstract PCD driver.
 *          These features can be still used using the pcdCallExtraFeatures
 *          API. Structures used as parameters and results of various commands
 *          are defined here.
 *
 * @{
 */

#ifndef HAL_ABSTRACT_ISO14443_PCD_EXT
#define HAL_ABSTRACT_ISO14443_PCD_EXT

#include "hal.h"
#include "hal_custom.h"
#include "hal_abstract_iso14443_pcd.h"

/* === PCD_EXT_SELFTEST structures === */

/**
 * @brief      Parameters for #PCD_EXT_SELFTEST command
 *
 * #PCD_EXT_SELFTEST takes no parameters, this structure is empty and
 * @p params argument can be `NULL`.
 */
struct PcdExtSelftest_params {
};

/**
 * @brief      Result of the #PCD_EXT_SELFTEST command
 */
struct PcdExtSelftest_result {
    bool passed;                /**< Did the self-test passed?               */
};

/* === PCD_EXT_CALCULATE_CRC_A and PCD_EXT_CALCULATE_CRC_B structures */

/**
 * @brief      Parameters for #PCD_EXT_CALCULATE_CRC_A and
 *             #PCD_EXT_CALCULATE_CRC_B commands.
 */
struct PcdExtCalcCRC_params {
    uint16_t length;            /**< Size of the data to calculate CRC of    */
    uint8_t *buffer;            /**< Pointer to data buffer                  */
};

/**
 * @brief      Result of #PCD_EXT_CALCULATE_CRC_A and
 *             #PCD_EXT_CALCULATE_CRC_B commands.
 */
struct PcdExtCalcCRC_result {
    uint16_t crc;               /**< Resulting CRC                           */
};

/* === PCD_EXT_MIFARE_AUTH structures === */

/**
 * @brief      Parameters for the #PCD_EXT_MIFARE_AUTH command
 */
struct PcdExtMifareAuth_params {
    uint8_t authCommandCode;
    uint8_t blockAddr;
    uint8_t sectorKey[6];
    uint8_t cardSerialNumber[4];
};

/**
 * @brief      Result of the #PCD_EXT_MIFARE_AUTH command
 */
struct PcdExtMifareAuth_result {
    bool authSuccess;
};

#endif /* HAL_ABSTRACT_ISO14443_PCD_EXT */

/** @} */
