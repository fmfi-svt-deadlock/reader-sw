/**
 * @file    hal_mfrc522.h
 * @brief   Driver for the MFRC522 module.
 * @details This is a driver for the MFRC522 Mifare and NTAG frontend. It
 *          supports the MFRC522 chip connected over various interfaces.
 *          It exports an Pcd object for use by other layers.
 *
 * Note: This driver requires the EXT driver to be enabled and configured
 * with non-`const` EXTConfig strcture (as opposed to one documented by
 * function extStart)!
 *
 * This driver invokes the extSetChannelMode function, see its docs for
 * explanation.
 *
 * @addtogroup HAL_MFRC522
 * @{
 */

#include "hal.h"
#include "hal_custom.h"

#if (HAL_USE_MFRC522 == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @brief   Enables SPI support.
 * @note    Disabling this option saves both code and data space.
 */
#if !defined(MFRC522_USE_SPI) || defined(__DOXYGEN__)
#define MFRC522_USE_SPI             FALSE
#endif

/**
 * @brief   Enables I2C support.
 * @note    Disabling this option saves both code and data space.
 */
#if !defined(MFRC522_USE_I2C) || defined(__DOXYGEN__)
#define MFRC522_USE_I2C             FALSE
#endif

/**
 * @brief   Enables UART support.
 * @note    Disabling this option saves both code and data space.
 */
#if !defined(MFRC522_USE_UART) || defined(__DOXYGEN__)
#define MFRC522_USE_UART            FALSE
#endif

/**
 * @brief   Maximum number of simultaneously active devices this driver should
 *          handle.
 * @note    Lowering this value saves data space and increases driver
 *          performance.
 */
#if !defined(MFRC522_MAX_DEVICES) || defined(__DOXYGEN__)
#define MFRC522_MAX_DEVICES			5
#endif

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if (HAL_USE_MFRC522 == TRUE) || defined(__DOXYGEN__)

#if MFRC522_USE_SPI && !HAL_USE_SPI
#error "This driver is configured to use SPI, but HAL is not!"
#endif

#if MFRC522_USE_I2C && !HAL_USE_I2C
#error "This driver is configured to use I2C, but HAL is not!"
#endif

#if MFRC522_USE_UART && !HAL_USE_UART
#error "This driver is configured to use UART, but HAL is not!"
#endif

#if !HAL_USE_EXT
#error "MFRC522 driver requires the EXT subsystem!"
#endif

#if !HAL_USE_PAL
#error "MFRC522 driver requires the PAL subsystem!"
#endif

#if !MFRC522_USE_SPI && !MFRC522_USE_I2C && !MFRC522_USE_UART
#error "MFRC522 driver is enabled, but no interface is configured."
#endif

#endif // HAL_USE_MFRC522

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief Config options for the MFRC522 module
 *
 * In order for the driver to function properly you have to specify:
 *   - `EXTDriver *extp`
 *   - `expchannel_t interrupt_channel`
 *   - `void *reset_line`
 *
 * Rest of these options are intended for advanced configuration of the module
 * if you have special needs, otherwise default values are just fine.
 *
 * If you need to modify something consult the MFRC522 Datasheet.
 *
 */
typedef struct {

	/**
	 * EXT driver to use for handling MFRC522-issued interrupts
	 */
	EXTDriver *extp;

	/**
	 * EXT channel to which the IRQ pin (**and only the IRQ pin**) of the
	 * MFRC522 is connected.
	 */
	expchannel_t interrupt_channel;

	/**
	 * PAL line, which when set low resets the connected MFRC522
	 */
	void *reset_line;

	/**
	 * @brief Defines polarity of pin MFIN
	 *
	 * true: MFIN is active HIGH
	 * false: MFIN is active LOW
	 *
	 * Default: true
	 *
	 * MFRC522 Datasheet page 48
	 */
	bool MFIN_polarity;

	/**
	 * @brief Modulation of transmitted data should be inverted
	 *
	 * Default: false
	 *
	 * MFRC522 Datasheet page 49
	 */
	bool inverse_modulation;

	/**
	 * @brief Value of the transmission control register
	 *
	 * Default: 0x80
	 *
	 * MFRC522 Datasheet page 50
	 */
	uint8_t tx_control_reg;

	/**
	 * @brief Selects the input of drivers TX1 and TX2
	 *
	 * Default: MFRC522_DRSEL_MPE
	 *
	 * MFRC522 Datasheet page 51
	 */
	enum driver_input_select_t {
		MFRC522_DRSEL_3STATE = 0b00,	///< 3-state mode during soft powerdown
		MFRC522_DRSEL_MPE    = 0b01,    ///< modulation signal (envelope) from
		                                ///< the internal encoder, Miller pulse
		                                ///< encoded
		MFRC522_DRSEL_MFIN   = 0b10,	///< modulation signal (envelope) from
		                            	///< pin MFIN
		MFRC522_DRSEL_HIGH   = 0b11,	///< HIGH; the HIGH level depends on
		                            	///< the setting of bits
		                            	///< InvTx1RFOn/InvTx1RFOff and
		                            	///< InvTx2RFOn/InvTx2RFOff
	} driver_input_select;

	/**
	 * @brief Selects the input for pin MFOUT
	 *
	 * Default: MFRC522_MFSEL_3STATE
	 *
	 * MFRC522 Datasheet page 52
	 */
	enum mfout_select_t {
		MFRC522_MFSEL_3STATE = 0b0000,	///< 3-state
		MFRC522_MFSEL_LOW    = 0b0001,  ///< LOW
		MFRC522_MFSEL_HIGH   = 0b0010,  ///< HIGH
		MFRC522_MFSEL_TBUS   = 0b0011,  ///< test bus signal as defined by the
		                                ///< test_bus_bit_sel
		MFRC522_MFSEL_MPE    = 0b0100,  ///< modulation signal (envelope) from
		                                ///< the internal encoder, Miller
		                                ///< pulse encoded
		MFRC522_MFSEL_SSTRT  = 0b0101,  ///< serial data stream to be
		                                ///< transmitted, data stream before
		                                ///< Miller encoder
		MFRC522_MFSEL_SSTRR  = 0b0111,  ///< serial data stream received, data
		                                ///< stream after Manchester decoder
	} mfout_select;

	/**
	 * @brief Selects the input of the contactless UART
	 *
	 * Default: MFRC522_UINSEL_ANALOG
	 *
	 * MFRC522 Datasheet page 52
	 */
	enum cl_uart_in_sel_t {
		MFRC522_UINSEL_LOW      = 0b00,	///< constant LOW
		MFRC522_UINSEL_MAN_MFIN = 0b01,	///< Manchester with subcarrier from
		                               	///< pin MFIN
		MFRC522_UINSEL_ANALOG   = 0b10, ///< modulated signal from the internal
		                                ///< analog module, default
		MFRC522_UINSEL_NRZ_MFIN = 0b11, ///< NRZ coding without subcarrier from
		                                ///< pin MFIN which is only valid for
		                                ///< transfer speeds above 106 kBd
	} cl_uart_in_sel;

	/**
	 * @brief Minimum signal strength which will be accepted by the decoder
	 *
	 * Only 4 lowest bits are taken into account.
	 *
	 * Default: 8
	 *
	 * MFRC522 Datasheet page 53
	 */
	uint8_t min_rx_signal_strength;

	/**
	 * @brief Minimum collision signal strength
	 *
	 * Minimum signal strength at the decoder input that must be
     * reached by the weaker half-bit of the Manchester encoded signal to
     * generate a bit-collision relative to the amplitude of the stronger
	 * half-bit.
	 *
	 * Only 3 lowest bits are taken into account.
	 *
	 * Default: 4
	 *
	 * MFRC522 Datasheet page 53
	 */
	uint8_t min_rx_collision_level;

	/**
	 * @brief Demodulator settings
	 *
	 * Default: 0x4D
	 *
	 * MFRC522 Datasheet page 53
	 */
	uint8_t demod_reg;

	/**
	 * @brief      Gain of the receiver
	 *
	 * Default: MFRC522_GAIN_33
	 *
	 * MFRC522 Datasheet page 59
	 */
	enum receiver_gain_t {
		MFRC522_GAIN_18 = 0b000,	///< 18 dB
		MFRC522_GAIN_23 = 0b001,	///< 23 dB
		MFRC522_GAIN_33 = 0b100,	///< 33 dB
		MFRC522_GAIN_38 = 0b101,	///< 38 dB
		MFRC522_GAIN_43 = 0b110, 	///< 43 dB
		MFRC522_GAIN_48 = 0b111,	///< 48 dB
	} receiver_gain;

	/**
	 * @brief Conductance of the output n-driver (CWGsN) which can be used to
	 *        regulate the output power
	 *
	 * Default: 8
	 *
	 * MFRC522 Datasheet page 59
	 */
	uint8_t transmit_power_n;

	/**
	 * @brief Conductance of the output n-driver (ModGsN) which can be used to
	 *        regulate the modulation index
	 *
	 * Default: 8
	 *
	 * MFRC522 Datasheet page 59
	 */
	uint8_t modulation_index_n;

	/**
	 * @brief Conductance of the output p-driver (CWGsP) which can be used to
	 *        regulate the output power
	 *
	 * Default: 32
	 *
	 * MFRC522 Datasheet page 60
	 */
	uint8_t transmit_power_p;

	/**
	 * @brief Conductance of the output n-driver (ModGsP) which can be used to
	 *        regulate the modulation index
	 *
	 * Default: 32
	 *
	 * MFRC522 Datasheet page 60
	 */
	uint8_t modulation_index_p;
} Mfrc522Config;

/**
 * @brief Structure representing a MFRC522 driver.
 *
 * For functions expecting a Pcd object use the `pcd` memeber of this
 * structure. Otherwise don't modify any of these values.
 */
typedef struct {
	/**
	 * Abstract Pcd structure to be used with other parts of this stack
	 */
	Pcd pcd;

	/**
	 * Driver state
	 */
	pcdstate_t state;

	/**
	 * How is the MFRC522 connected.
	 */
	enum mfrc522_conntype_t {
#if MFRC522_USE_SPI || defined(__DOXYGEN__)
		MFRC522_CONN_SPI,
#endif
#if MFRC522_USE_I2C || defined(__DOXYGEN__)
		MFRC522_CONN_I2C,
#endif
#if MFRC522_USE_UART || defined(__DOXYGEN__)
		MFRC522_CONN_SERIAL,
#endif
	} connection_type;

	/**
	 * @brief Pointer to a given interface driver.
	 */
	union iface_u {
#if MFRC522_USE_SPI || defined(__DOXYGEN__)
		SPIDriver *spip;
#endif
#if MFRC522_USE_I2C || defined(__DOXYGEN__)
		I2CDriver *i2cp;
#endif
#if MFRC522_USE_UART || defined(__DOXYGEN__)
		SerialDriver *sdp;
#endif
	} iface;

	/**
	 * EXT driver to use for handling MFRC522-issued interrupts
	 */
	EXTDriver *extp;

	/**
	 * EXT channel to which the IRQ pin is connected
	 */
	expchannel_t interrupt_channel;

	/**
	 * PAL line, which when set low resets the connected MFRC522
	 */
	void *reset_line;

	/**
	 * Lastly applied config
	 */
	const Mfrc522Config *current_config;

	/**
	 * Interrupt is pending for this reader
	 */
	volatile bool interrupt_pending;

	/**
	 * Thread reference the reader will sleep on
	 */
	thread_reference_t tr;

	/**
	 * Response buffer
	 */
	uint8_t response[64];

	/**
	 * Number of last valid bits in the response
	 */
	uint8_t resp_last_valid_bits;

	/**
	 * Response length
	 */
	uint8_t resp_length;

	/**
	 * Number of already retreived response bytes
	 */
	uint8_t resp_read_bytes;

} Mfrc522Driver;

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
 * @brief      Initializes the MFRC522 driver.
 *
 * @note       This function is called implicitly by halCustomInit, no need
 *             to call it explicitly.
 */
void mfrc522Init(void);

#if MFRC522_USE_SPI || defined(__DOXYGEN__)
/**
 * @brief      Initializes the driver object for a MFRC522 module connected
 *             over the SPI.
 *
 * @param      mdp   Uninitialized driver object.
 * @param      spip  Initialized and started SPI driver object.
 */
void mfrc522ObjectInitSPI(Mfrc522Driver *mdp, SPIDriver *spip);
#endif

#if MFRC522_USE_I2C || defined(__DOXYGEN__)
/**
 * @brief      Initializes the driver object for a MFRC522 module connected
 *             over the I2C
 *
 * @note 	   Not yet implemented!
 *
 * @param      mdp   Uninitialized driver object.
 * @param      i2cp  Initialized and started I2C driver object
 */
void mfrc522ObjectInitI2C(Mfrc522Driver *mdp, I2CDriver *i2cp);
#endif

#if MFRC522_USE_UART || defined(__DOXYGEN__)
/**
 * @brief      Initializes the driver object for a MFRC522 module connected
 *             over the Serial
 *
 * @note 	   Not yet implemented!
 *
 * @param      mdp   Uninitialized driver object.
 * @param      sdp   Initialized and started Serial driver object
 */
void mfrc522ObjectInitSerial(Mfrc522Driver *mdp, SerialDriver *sdp);
#endif

/**
 * @brief      Starts the MFRC522 module
 *
 * This function powers up, soft-resets the MFRC522 module, initializes,
 * configures it and register is for use with this driver.
 * It also reconfigures the provided Ext driver and register its own interrupt
 * handler to handle interrupts on channel config->interrupt_channel.
 *
 * @param      mdp     Initialized driver object
 * @param      config  Configuration to apply
 */
void mfrc522Start(Mfrc522Driver *mdp, const Mfrc522Config *config);

/**
 * @brief      Reconfigures the MFRC522 module without resetting it
 *
 * This function reprograms control registers of the MFRC522 module without
 * resetting it, which is useful for hot-swapping MFRC522-specific config
 * options during the module run-time.
 *
 * This function won't reconfig the following options:
 *
 *   - `EXTDriver *extp`
 *   - `expchannel_t interrupt_channel`
 *   - `void *reset_line`
 *
 * The only way to change these is to stop and restart the module.
 *
 * @param      mdp     Started driver object
 * @param      config  Configuration to apply
 */
void mfrc522Reconfig(Mfrc522Driver *mdp, const Mfrc522Config *config);

/**
 * @brief      Stops the MFRC522 module
 *
 * Unregisters this module from this driver and powers it down.
 *
 * @param      mdp   Started driver object
 */
void mfrc522Stop(Mfrc522Driver *mdp);

#ifdef __cplusplus
}
#endif


#endif /* HAL_USE_ISO14443_PICC */

/** @} */
