#include "hal.h"
#include "hal_custom.h"

#include "hal_mfrc522_internal.h"

#if (HAL_USE_MFRC522 == TRUE) || defined(__DOXYGEN__)

// Also it should conform to
// http://www.chibios.org/dokuwiki/doku.php?id=chibios:articles:style_guide

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

const PcdSParams supported_params = {
    // Supported tx / rx speeds in mode A
    (PCD_TX_SPEED_106 | PCD_TX_SPEED_212 | PCD_TX_SPEED_424 |
     PCD_TX_SPEED_848 | PCD_RX_SPEED_106 | PCD_RX_SPEED_212 |
     PCD_RX_SPEED_424 | PCD_RX_SPEED_848),
    // Supported tx / rx speeds in mode B
    0,
    // Support for assymetric speed setting
    true,
    // Supported operation modes (A or B)
    PCD_ISO14443_A,
    // Max tx frame size
    64,
    // Max rx frame size
    64
};

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

void ext_callback(EXTDriver *extp, expchannel_t);

const EXTChannelConfig interrupt_config = {
    EXT_CH_MODE_RISING_EDGE | EXT_MODE_GPIOA,
    &ext_callback
};

Mfrc522Driver* active_drivers[MFRC522_MAX_DEVICES];

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

void ext_callback(EXTDriver *extp, expchannel_t channel) {
    // Interrupt request handler. Find and wake up the proper thread
    // Find module which should be woken up and wake it up.
    osalSysLockFromISR();
    uint8_t i;
    // MAX_DEVICES is usually 1, almost definitely less than 16, this loop
    // should't take long, therefore we can afford it in a lock zone.
    for (i = 0; i < MFRC522_MAX_DEVICES; i++) {
        if (active_drivers[i]->extp == extp &&
            active_drivers[i]->interrupt_channel == channel) {
            active_drivers[i]->interrupt_pending = true;
            osalThreadResumeI(&active_drivers[i]->tr, MFRC522_MSG_INTERRUPT);
            break;
        }
    }
    osalSysUnlockFromISR();
}

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

void mfrc522Init(void) {
    // Nothing to do.
}

#if MFRC522_USE_SPI || defined(__DOXYGEN__)
void mfrc522ObjectInitSPI(Mfrc522Driver *mdp, SPIDriver *spip) {
    mdp->connection_type = MFRC522_CONN_SPI;
    mdp->iface.spip = spip;
    mdp->state = PCD_STOP;
    mdp->pcd.vmt = &mfrc522VMT;
    mdp->pcd.data = mdp;
}
#endif

#if MFRC522_USE_I2C || defined(__DOXYGEN__)
void mfrc522ObjectInitI2C(Mfrc522Driver *mdp, I2CDriver *i2cp) {
    osalSysHalt("Not implemented!");
}
#endif

#if MFRC522_USE_UART || defined(__DOXYGEN__)
void mfrc522ObjectInitSerial(Mfrc522Driver *mdp, SerialDriver *sdp) {
    osalSysHalt("Not implemented!");
}
#endif

void mfrc522Start(Mfrc522Driver *mdp, const Mfrc522Config *config) {
    osalDbgCheck(mdp != NULL);
    osalDbgAssert(mdp->state == PCD_STOP, "Incorrect state!");
    osalDbgCheck(config != NULL);

    osalSysLock();
    // MAX_DEVICES is usually 1, almost definitely less than 16, this loop
    // should't take long, therefore we can afford it in a lock zone.
    uint8_t i = 0;
    while(true) {
        if (i == MFRC522_MAX_DEVICES) {
            osalSysHalt("Maximum number of active MFRC522 modules exceeded!");
        }
        if (active_drivers[i] == NULL) {
            active_drivers[i] = mdp;
            break;
        }
        i++;
    }
    osalSysUnlock();


    // Enable the MFRC522
    mdp->reset_line = config->reset_line;
    palSetLine(config->reset_line);
    osalThreadSleepMicroseconds(40);  // Oscillator start-up time

    mfrc522_command(mdp, Command_SoftReset);

    // Interrupt pin setup
    // Disable propagation of all communication interrupts
    mfrc522_write_register(mdp, ComIEnReg, 0);
    // Set IRQ pin to push-pull and disable propagation of the rest of the
    // interrupts
    mfrc522_write_register(mdp, DivIEnReg, (1 << DivIEnReg_IRQPushPull));

    // Interrupt handler setup
    osalDbgCheck(config->extp != NULL);
    mdp->extp = config->extp;
    mdp->interrupt_channel = config->interrupt_channel;

    // TODO don't forget to document that each reader must be connected to a
    // **UNIQUE** interrupt channel and **ONLY** the reader may be connected to
    // that interrupt channel.
    extSetChannelMode(config->extp, config->interrupt_channel,
                      &interrupt_config);
    extChannelEnable(config->extp, config->interrupt_channel);
    mdp->extp = config->extp;
    mdp->interrupt_channel = config->interrupt_channel;

    osalMutexObjectInit(&(mdp->mutex));

    mdp->state = PCD_RF_OFF;

    // Apply the provided configuration
    mfrc522Reconfig(mdp, config);
    // Apply default transmission params
    pcdSetParamsAB(&mdp->pcd, PCD_RX_SPEED_106, PCD_TX_SPEED_106,
                   PCD_ISO14443_A);
}

void mfrc522Reconfig(Mfrc522Driver *mdp, const Mfrc522Config *config) {
    osalDbgCheck(mdp != NULL);
    osalDbgAssert(mdp->state == PCD_RF_OFF || mdp->state == PCD_READY,
                  "Incorrect state!");
    osalDbgCheck(config != NULL);

    // TODO state explicitly that interrupt-related settings will **NOT** be
    // changed.

    mfrc522_write_register_bitmask(mdp, ModeReg,
        (ModeReg_PolMFin | Mask_ModeReg_CRCPreset),
        (config->MFIN_polarity ? 1 : 0 << ModeReg_PolMFin) |
        (ModeReg_CRCPreset_6363 << ModeReg_CRCPreset));

    mfrc522_write_register_bitmask(mdp, TxModeReg, TxModeReg_InvMod,
        (config->inverse_modulation ? 1 : 0 << TxModeReg_InvMod));

    mfrc522_write_register(mdp, TxControlReg, config->tx_control_reg);

    mfrc522_write_register_bitmask(mdp, TxSelReg,
        (Mask_TxSelReg_DriverSel | Mask_TxSelReg_MFOutSel),
        (config->driver_input_select << TxSelReg_DriverSel) |
        (config->mfout_select << TxSelReg_MFOutSel));

    mfrc522_write_register_bitmask(mdp, RxSelReg, Mask_RxSelReg_UARTSel,
                           (config->cl_uart_in_sel << RxSelReg_UARTSel));

    mfrc522_write_register_bitmask(mdp, RxThresholdReg,
        (Mask_RxThresholdReg_CollLevel | Mask_RxThresholdReg_CollLevel),
        ((config->min_rx_signal_strength & 0xF) << RxThresholdReg_MinLevel) |
        ((config->min_rx_collision_level & 0x7) << RxThresholdReg_CollLevel));

    mfrc522_write_register(mdp, DemodReg, config->demod_reg);

    mfrc522_write_register_bitmask(mdp, RFCfgReg, Mask_RFCfgReg_RxGain,
                           (config->receiver_gain) << RFCfgReg_RxGain);

    mfrc522_write_register(mdp, GsNReg,
        ((config->transmit_power_n & 0xF) << GsNReg_CWGsN) |
        ((config->modulation_index_n & 0xF) << GsNReg_ModGsN));

    mfrc522_write_register_bitmask(mdp, CWGsPReg, Mask_CWGsPReg_CWGsP,
        (config->transmit_power_p & 0x3F) << CWGsPReg_CWGsP);

    mfrc522_write_register_bitmask(mdp, ModGsPReg, Mask_ModGsPReg_ModGsP,
        (config->modulation_index_p & 0x3F) << ModGsPReg_ModGsP);

    // Without this, the CollPosNotValid bit in CollReg would be set until
    // the last bit was received, despite CollErr error being set and
    // interrupt firing sooner. The driver would then not be able to
    // determine the collision position.
    mfrc522_clear_register_bits(mdp, CollReg, (1 << CollReg_ValuesAfterColl));

    mdp->current_config = config;
}

void mfrc522Stop(Mfrc522Driver *mdp) {
    osalDbgCheck(mdp != NULL);
    osalDbgAssert(mdp->state == PCD_READY || mdp->state == PCD_RF_OFF,
                  "Incorrect state!");
    extChannelDisable(mdp->extp, mdp->interrupt_channel);
    palClearLine(mdp->reset_line);
    mdp->state = PCD_STOP;

    osalSysLock();
    // MAX_DEVICES is usually 1, almost definitely less than 16, this loop
    // should't take long, therefore we can afford it in a lock zone.
    uint8_t i = 0;
    while(true) {
        if (i == MFRC522_MAX_DEVICES) {
            osalSysHalt("Internal driver data corrupted!");
        }
        if (active_drivers[i] == mdp) {
            active_drivers[i] = NULL;
            break;
        }
        i++;
    }
    osalSysUnlock();
}

#endif /* HAL_USE_MFRC522 == TRUE */
