LDSCRIPT 		 = $(STARTUPLD)/STM32F072xB.ld
FW_FLASH_ADDRESS = 0x08000000
MCU      		 = cortex-m0
BOARDSRC 		 = $(CUSTOM_HAL)/boards/reader-revA/board.c
BOARDINC 		 = $(CUSTOM_HAL)/boards/reader-revA
