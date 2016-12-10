# List of all the Deadlock/HAL files, there is no need to remove the files
# from this list, you can disable parts of the HAL by editing halconf.h.

# This file should integrate nicely with HAL of ChibiOS

ifeq ($(USE_SMART_BUILD),yes)
HALCONF := $(strip $(shell cat halconf.h | egrep -e "define"))

HALSRC += $(CUSTOM_HAL)/src/hal_custom.c

ifneq ($(findstring HAL_USE_MFRC522 TRUE,$(HALCONF)),)
HALSRC += $(CUSTOM_HAL)/src/hal_mfrc522/hal_mfrc522.c             \
          $(CUSTOM_HAL)/src/hal_mfrc522/hal_mfrc522_ext_api.c     \
          $(CUSTOM_HAL)/src/hal_mfrc522/hal_mfrc522_llcom.c       \
          $(CUSTOM_HAL)/src/hal_mfrc522/hal_mfrc522_pcd_api.c
endif

ifneq ($(findstring HAL_USE_ISO14443_PICC TRUE,$(HALCONF)),)
HALSRC += $(CUSTOM_HAL)/src/hal_iso14443_picc.c
endif

else
HALSRC += $(CUSTOM_HAL)/src/hal_custom.c                          \
          $(CUSTOM_HAL)/src/hal_mfrc522/hal_mfrc522.c             \
          $(CUSTOM_HAL)/src/hal_mfrc522/hal_mfrc522_ext_api.c     \
          $(CUSTOM_HAL)/src/hal_mfrc522/hal_mfrc522_llcom.c       \
          $(CUSTOM_HAL)/src/hal_mfrc522/hal_mfrc522_pcd_api.c     \
          $(CUSTOM_HAL)/src/hal_iso14443_picc.c
endif

# Required include directories
HALINC += $(CUSTOM_HAL)/include
