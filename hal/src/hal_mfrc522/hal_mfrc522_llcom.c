/**
 * @file    hal_mfrc522_llcom.c
 * @brief   Low-level communication functions
 * @detial	This file defines the functions for the low-level communication
 *          (reading and writing the registers) with the MFRC522 over
 *          various interfaces.
 *
 */

#include "hal_mfrc522_internal.h"

// TODO one-shot functions could use burst functions (DRY!)

void mfrc522_write_register(Mfrc522Driver *mdp, Mfrc522Register reg,
                            uint8_t value) {
    switch(mdp->connection_type) {
#if MFRC522_USE_SPI
        case MFRC522_CONN_SPI:
            //TODO call conditionally if the SPI has mutual exclusion enabled
            spiAcquireBus(mdp->iface.spip);
            spiSelect(mdp->iface.spip);

            const uint8_t txbuf[2] = {
                // Bit 7 is zero: write. 6 bit address starting on bit 1.
                ((reg & 0x3F) << 1),
                value,
            };
            uint8_t rxbuf[2];

            spiExchange(mdp->iface.spip, 2, txbuf, rxbuf);

            spiUnselect(mdp->iface.spip);
            spiReleaseBus(mdp->iface.spip);
            return;
#endif
#if MFRC522_USE_I2C
        case MFRC522_CONN_I2C:
            osalSysHalt("MFRC522: I2C not implemented!");
            return;
#endif
#if MFRC522_USE_UART
        case MFRC522_CONN_SERIAL:
            osalSysHalt("MFRC522: Serial not implemented!");
            return;
#endif
    }
    osalSysHalt("MFRC522: corrupted driver structure!");
}

uint8_t mfrc522_read_register(Mfrc522Driver *mdp, Mfrc522Register reg) {
    switch(mdp->connection_type) {
#if MFRC522_USE_SPI
        case MFRC522_CONN_SPI:
            //TODO call conditionally if the SPI has mutual exclusion enabled
            spiAcquireBus(mdp->iface.spip);
            spiSelect(mdp->iface.spip);

            const uint8_t txbuf[2] = {
                // Bit 7 is one: read. 6 bit address starting on bit 1.
                0x80 | ((reg & 0x3F) << 1),
                0x00,
            };
            uint8_t rxbuf[2];

            spiExchange(mdp->iface.spip, 2, txbuf, rxbuf);

            spiUnselect(mdp->iface.spip);
            spiReleaseBus(mdp->iface.spip);
            return rxbuf[1];
#endif
#if MFRC522_USE_I2C
        case MFRC522_CONN_I2C:
            osalSysHalt("MFRC522: I2C not implemented!");
            return 0;
#endif
#if MFRC522_USE_UART
        case MFRC522_CONN_SERIAL:
            osalSysHalt("MFRC522: Serial not implemented!");
            return 0;
#endif
    }
    osalSysHalt("MFRC522: corrupted driver structure!");
    return 0;
}

void mfrc522_write_register_burst(Mfrc522Driver *mdp, Mfrc522Register reg,
                                  const uint8_t *values, uint8_t length) {
    switch(mdp->connection_type) {
#if MFRC522_USE_SPI
        case MFRC522_CONN_SPI:
            //TODO call conditionally if the SPI has mutual exclusion enabled
            spiAcquireBus(mdp->iface.spip);
            spiSelect(mdp->iface.spip);

            const uint8_t txbuf[1] = {
                // Bit 7 is zero: write. 6 bit address starting on bit 1.
                ((reg & 0x3F) << 1)
            };

            spiSend(mdp->iface.spip, 1, txbuf);
            spiSend(mdp->iface.spip, length, values);

            spiUnselect(mdp->iface.spip);
            spiReleaseBus(mdp->iface.spip);
            return;
#endif
#if MFRC522_USE_I2C
        case MFRC522_CONN_I2C:
            osalSysHalt("MFRC522: I2C not implemented!");
            return;
#endif
#if MFRC522_USE_UART
        case MFRC522_CONN_SERIAL:
            osalSysHalt("MFRC522: Serial not implemented!");
            return;
#endif
    }
    osalSysHalt("MFRC522: corrupted driver structure!");
}

void mfrc522_read_register_burst(Mfrc522Driver *mdp, Mfrc522Register reg,
                                 uint8_t *values, uint8_t length) {
    switch(mdp->connection_type) {
#if MFRC522_USE_SPI
        case MFRC522_CONN_SPI:
            //TODO call conditionally if the SPI has mutual exclusion enabled
            spiAcquireBus(mdp->iface.spip);
            spiSelect(mdp->iface.spip);

            uint8_t txbuf[length+1];
            txbuf[0] = 0x80 | ((reg & 0x3F) << 1);
            uint8_t i;
            for(i = 0; i < length; i++) {
                txbuf[i+1] = 0x80 | ((reg & 0x3F) << 1);
            }

            spiSend(mdp->iface.spip, 1, txbuf);
            spiExchange(mdp->iface.spip, length, txbuf, values);

            spiUnselect(mdp->iface.spip);
            spiReleaseBus(mdp->iface.spip);
            return;
#endif
#if MFRC522_USE_I2C
        case MFRC522_CONN_I2C:
            osalSysHalt("MFRC522: I2C not implemented!");
            return;
#endif
#if MFRC522_USE_UART
        case MFRC522_CONN_SERIAL:
            osalSysHalt("MFRC522: Serial not implemented!");
            return;
#endif
    }
    osalSysHalt("MFRC522: corrupted driver structure!");
}
