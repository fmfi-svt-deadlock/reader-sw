/*
    ChibiOS - Copyright (C) 2006..2015 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

    ----

    This file was modified to be used in the Project Deadlock. Changelist:
      - Customized I/O pin assignment names
      - Customized initial PAL setup
      - Added SPI configuration

    These changes are licensed under:

    The MIT License (MIT)

    Copyright (c) 2016 FMFI ŠVT / Project Deadlock

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#ifndef _BOARD_H_
#define _BOARD_H_

/*
 * Setup for ST STM32F072B-Discovery board.
 */

/*
 * Board identifier.
 */
#define BOARD_SVT_DEADLOCK_READER_REVA
#define BOARD_NAME                  "SVT Deadlock Reader revA"

/*
 * Board oscillators-related settings.
 * NOTE: LSE not fitted.
 * NOTE: HSE not fitted.
 */
#if !defined(STM32_LSECLK)
#define STM32_LSECLK                0U
#endif

#define STM32_LSEDRV                (3U << 3U)

#if !defined(STM32_HSECLK)
#define STM32_HSECLK                0U
#endif

#define STM32_HSE_BYPASS

/*
 * MCU type as defined in the ST header.
 */
#define STM32F072xB

/*
 * IO pins assignments.
 *
 * Note: GPIOA_SWCLK and GPIOA_RDR_TXD is the same pin!
 * SWD has priority at startup. GPIOA 10 is not connected.
 *
 * Please refer to the Reader revA schematic for more details
 * (https://github.com/fmfi-svt-deadlock/reader-hw/tree/revA)
 */
#define GPIOA_RFID_IRQ              0U
#define GPIOA_RFID_SS               1U
#define GPIOA_V_SENSE               2U
#define GPIOA_RFID_RST              3U
#define GPIOA_AUDIO_OUT             4U
#define GPIOA_RFID_SCK              5U
#define GPIOA_RFID_MISO             6U
#define GPIOA_RFID_MOSI             7U
#define GPIOA_LED_G2                8U
#define GPIOA_LED_R2                9U
#define GPIOA_USB_DM                11U
#define GPIOA_USB_DP                12U
#define GPIOA_SWDIO                 13U
#define GPIOA_SWCLK                 14U
#define GPIOA_RDR_TXD               14U
#define GPIOA_RDR_RXD               15U

#define GPIOB_LED_R1                0U
#define GPIOB_LED_G1                1U
#define GPIOB_T_SWO                 3U

/*
 * I/O ports initial setup, this configuration is established soon after reset
 * in the initialization code.
 * Please refer to the STM32 Reference Manual for details.
 */
#define PIN_MODE_INPUT(n)           (0U << ((n) * 2U))
#define PIN_MODE_OUTPUT(n)          (1U << ((n) * 2U))
#define PIN_MODE_ALTERNATE(n)       (2U << ((n) * 2U))
#define PIN_MODE_ANALOG(n)          (3U << ((n) * 2U))
#define MODER_DEFAULT_INPUT         0x00000000
#define PIN_ODR_LOW(n)              (0U << (n))
#define PIN_ODR_HIGH(n)             (1U << (n))
#define ODR_DEFAULT_LOW             0x00000000
#define PIN_OTYPE_PUSHPULL(n)       (0U << (n))
#define PIN_OTYPE_OPENDRAIN(n)      (1U << (n))
#define OTYPER_DEFAULT_PUSHPULL     0x00000000
#define PIN_OSPEED_VERYLOW(n)       (0U << ((n) * 2U))
#define PIN_OSPEED_LOW(n)           (1U << ((n) * 2U))
#define PIN_OSPEED_MEDIUM(n)        (2U << ((n) * 2U))
#define PIN_OSPEED_HIGH(n)          (3U << ((n) * 2U))
#define OSPEEDR_DEFAULT_VERYLOW     0x00000000
#define PIN_PUPDR_FLOATING(n)       (0U << ((n) * 2U))
#define PIN_PUPDR_PULLUP(n)         (1U << ((n) * 2U))
#define PIN_PUPDR_PULLDOWN(n)       (2U << ((n) * 2U))
#define PUPDR_DEFAULT_FLOATING      0x00000000
#define PIN_AFIO_AF(n, v)           ((v) << (((n) % 8U) * 4U))
#define AFIO_DEFAULT_0              0x00000000

/*
 * GPIOA setup
 */

#define VAL_GPIOA_MODER             (MODER_DEFAULT_INPUT |                  \
                                     PIN_MODE_INPUT(GPIOA_RFID_IRQ) |       \
                                     PIN_MODE_OUTPUT(GPIOA_RFID_SS) |       \
                                     PIN_MODE_ANALOG(GPIOA_V_SENSE) |       \
                                     PIN_MODE_OUTPUT(GPIOA_RFID_RST) |      \
                                     PIN_MODE_ANALOG(GPIOA_AUDIO_OUT) |     \
                                     PIN_MODE_ALTERNATE(GPIOA_RFID_SCK) |   \
                                     PIN_MODE_ALTERNATE(GPIOA_RFID_MISO) |  \
                                     PIN_MODE_ALTERNATE(GPIOA_RFID_MOSI) |  \
                                     PIN_MODE_OUTPUT(GPIOA_LED_G2) |        \
                                     PIN_MODE_OUTPUT(GPIOA_LED_R2) |        \
                                     PIN_MODE_ALTERNATE(GPIOA_USB_DP) |     \
                                     PIN_MODE_ALTERNATE(GPIOA_USB_DM) |     \
                                     PIN_MODE_ALTERNATE(GPIOA_SWDIO) |      \
                                     PIN_MODE_ALTERNATE(GPIOA_SWCLK) |      \
                                     PIN_MODE_ALTERNATE(GPIOA_RDR_RXD))

#define VAL_GPIOA_OTYPER            (OTYPER_DEFAULT_PUSHPULL)

#define VAL_GPIOA_OSPEEDR           (OSPEEDR_DEFAULT_VERYLOW |              \
                                     PIN_OSPEED_MEDIUM(GPIOA_RFID_SCK) |    \
                                     PIN_OSPEED_MEDIUM(GPIOA_RFID_MISO) |   \
                                     PIN_OSPEED_MEDIUM(GPIOA_RFID_MOSI) |   \
                                     PIN_OSPEED_HIGH(GPIOA_USB_DP) |        \
                                     PIN_OSPEED_HIGH(GPIOA_USB_DM) |        \
                                     PIN_OSPEED_HIGH(GPIOA_SWDIO) |         \
                                     PIN_OSPEED_HIGH(GPIOA_SWCLK) |         \
                                     PIN_OSPEED_HIGH(GPIOA_RDR_RXD))

#define VAL_GPIOA_PUPDR             (PUPDR_DEFAULT_FLOATING |               \
                                     PIN_PUPDR_PULLDOWN(GPIOA_RFID_IRQ) |   \
                                     PIN_PUPDR_PULLUP(GPIOA_SWDIO) |        \
                                     PIN_PUPDR_PULLDOWN(GPIOA_SWCLK))

#define VAL_GPIOA_ODR               (ODR_DEFAULT_LOW |                      \
                                     PIN_ODR_HIGH(GPIOA_RFID_SS))

#define VAL_GPIOA_AFRL              (AFIO_DEFAULT_0 |                       \
                                     PIN_AFIO_AF(GPIOA_RFID_SCK, 0) |       \
                                     PIN_AFIO_AF(GPIOA_RFID_MISO, 0) |      \
                                     PIN_AFIO_AF(GPIOA_RFID_MOSI, 0))

#define VAL_GPIOA_AFRH              (AFIO_DEFAULT_0 |                       \
                                   /*PIN_AFIO_AF(GPIOA_RDR_TXD, 1) |      \ (This would block SWD) */ \
                                     PIN_AFIO_AF(GPIOA_RDR_RXD, 1))

/*
 * GPIOB setup
 */
#define VAL_GPIOB_MODER             (MODER_DEFAULT_INPUT |                  \
                                     PIN_MODE_OUTPUT(GPIOB_LED_R1) |        \
                                     PIN_MODE_OUTPUT(GPIOB_LED_G1))
#define VAL_GPIOB_OTYPER            (OTYPER_DEFAULT_PUSHPULL)
#define VAL_GPIOB_OSPEEDR           (OSPEEDR_DEFAULT_VERYLOW)
#define VAL_GPIOB_PUPDR             (PUPDR_DEFAULT_FLOATING)
#define VAL_GPIOB_ODR               (ODR_DEFAULT_LOW)
#define VAL_GPIOB_AFRL              (AFIO_DEFAULT_0)
#define VAL_GPIOB_AFRH              (AFIO_DEFAULT_0)

/*
 * GPIOC setup
 */
#define VAL_GPIOC_MODER             (MODER_DEFAULT_INPUT)
#define VAL_GPIOC_OTYPER            (OTYPER_DEFAULT_PUSHPULL)
#define VAL_GPIOC_OSPEEDR           (OSPEEDR_DEFAULT_VERYLOW)
#define VAL_GPIOC_PUPDR             (PUPDR_DEFAULT_FLOATING)
#define VAL_GPIOC_ODR               (ODR_DEFAULT_LOW)
#define VAL_GPIOC_AFRL              (AFIO_DEFAULT_0)
#define VAL_GPIOC_AFRH              (AFIO_DEFAULT_0)

/*
 * GPIOD setup
 */
#define VAL_GPIOD_MODER             (MODER_DEFAULT_INPUT)
#define VAL_GPIOD_OTYPER            (OTYPER_DEFAULT_PUSHPULL)
#define VAL_GPIOD_OSPEEDR           (OSPEEDR_DEFAULT_VERYLOW)
#define VAL_GPIOD_PUPDR             (PUPDR_DEFAULT_FLOATING)
#define VAL_GPIOD_ODR               (ODR_DEFAULT_LOW)
#define VAL_GPIOD_AFRL              (AFIO_DEFAULT_0)
#define VAL_GPIOD_AFRH              (AFIO_DEFAULT_0)

/*
 * GPIOE setup
 */
#define VAL_GPIOE_MODER             (MODER_DEFAULT_INPUT)
#define VAL_GPIOE_OTYPER            (OTYPER_DEFAULT_PUSHPULL)
#define VAL_GPIOE_OSPEEDR           (OSPEEDR_DEFAULT_VERYLOW)
#define VAL_GPIOE_PUPDR             (PUPDR_DEFAULT_FLOATING)
#define VAL_GPIOE_ODR               (ODR_DEFAULT_LOW)
#define VAL_GPIOE_AFRL              (AFIO_DEFAULT_0)
#define VAL_GPIOE_AFRH              (AFIO_DEFAULT_0)

/*
 * GPIOF setup
 */
#define VAL_GPIOF_MODER             (MODER_DEFAULT_INPUT)
#define VAL_GPIOF_OTYPER            (OTYPER_DEFAULT_PUSHPULL)
#define VAL_GPIOF_OSPEEDR           (OSPEEDR_DEFAULT_VERYLOW)
#define VAL_GPIOF_PUPDR             (PUPDR_DEFAULT_FLOATING)
#define VAL_GPIOF_ODR               (ODR_DEFAULT_LOW)
#define VAL_GPIOF_AFRL              (AFIO_DEFAULT_0)
#define VAL_GPIOF_AFRH              (AFIO_DEFAULT_0)

/*
 * SPI setup
 * Setup of the SPI peripheral used to communicate with the MFRC522 module.
 *
 * We are using SPI1 peripheral
 * LSB-first
 * f_{PCLK}/8 (clock frequency will be ~6MHz => 6Mb/s. MFRC522 can handle 10Mb/s),
 * Clock polarity: to 0 when idle,
 * Clock phase: first clock transition is the data capture edge,
 * 8-bit data size.
 */
#define SPI_MFRC522                 SPID1
#define SPI_MFRC522_CS_PORT         GPIOA
#define SPI_MFRC522_CS_PIN          GPIOA_RFID_SS
#define SPI_MFRC522_VAL_CR1         (SPI_CR1_MSTR |                         \
                                     SPI_CR1_BR_1)
#define SPI_MFRC522_VAL_CR2         (SPI_CR2_DS_2 |                         \
                                     SPI_CR2_DS_1 |                         \
                                     SPI_CR2_DS_0)
// This is the initializer of platform-specific SPIConfig structure.
#define SPI_MFRC522_HAL_CONFIG      {NULL,                                  \
                                     SPI_MFRC522_CS_PORT,                   \
                                     SPI_MFRC522_CS_PIN,                    \
                                     SPI_MFRC522_VAL_CR1,                   \
                                     SPI_MFRC522_VAL_CR2 }

#if !defined(_FROM_ASM_)
#ifdef __cplusplus
extern "C" {
#endif
  void boardInit(void);
#ifdef __cplusplus
}
#endif
#endif /* _FROM_ASM_ */

#endif /* _BOARD_H_ */
