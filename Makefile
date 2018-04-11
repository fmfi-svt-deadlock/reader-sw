##############################################################################
# Build global options
# NOTE: Can be overridden externally.
#

ifeq ($(DEBUG_BUILD),)
	DEBUG_BUILD := yes
endif

# Compiler options here.
ifeq ($(USE_OPT),)
  ifeq ($(DEBUG_BUILD),yes)
  	USE_OPT = -O0 -ggdb -fomit-frame-pointer -falign-functions=16 -DUSE_CBOR_CONTEXT -DCBOR_ALIGN_READS -DCBOR_NO_STDLIB_REFERENCES
  else
  	USE_OPT = -O2 -fomit-frame-pointer -falign-functions=16 -DUSE_CBOR_CONTEXT -DCBOR_ALIGN_READS -DCBOR_NO_STDLIB_REFERENCES
  endif
endif

# C specific options here (added to USE_OPT).
ifeq ($(USE_COPT),)
  USE_COPT =
endif

# C++ specific options here (added to USE_OPT).
ifeq ($(USE_CPPOPT),)
  USE_CPPOPT = -fno-rtti
endif

# Enable this if you want the linker to remove unused code and data
ifeq ($(USE_LINK_GC),)
  USE_LINK_GC = yes
endif

# Linker extra options here.
ifeq ($(USE_LDOPT),)
  USE_LDOPT =
endif

# Enable this if you want link time optimizations (LTO)
ifeq ($(USE_LTO),)
  # If you experience an internal compiler error try turning this off as a
  # workaround of this gcc 5.3.0 bug:
  # https://gcc.gnu.org/bugzilla/show_bug.cgi?id=65380
  # The bug was still present in gcc 6.1.1, however not in 6.2.0
  USE_LTO = yes
endif

# If enabled, this option allows to compile the application in THUMB mode.
# TODO this is also MCU specific and should not be here!
ifeq ($(USE_THUMB),)
  USE_THUMB = yes
endif

# Enable this if you want to see the full log while compiling.
ifeq ($(USE_VERBOSE_COMPILE),)
  USE_VERBOSE_COMPILE = no
endif

# If enabled, this option makes the build process faster by not compiling
# modules not used in the current configuration.
#
# It is disabled for Deadlock because ChibiOS makefiles have hardcoded paths
# to `chconf.h` and `halconf.h`, but we want them to be potentially different
# for each flavour.
# This setting only makes compilation faster, it does not result in larger
# firmware file. And we don't care about compilation speed.
ifeq ($(USE_SMART_BUILD),)
  USE_SMART_BUILD = no
endif

#
# Build global options
##############################################################################

##############################################################################
# Architecture or project specific options
#

# Stack size to be allocated to the Cortex-M process stack. This stack is
# the stack used by the main() thread.
ifeq ($(USE_PROCESS_STACKSIZE),)
  USE_PROCESS_STACKSIZE = 0x200
endif

# Stack size to the allocated to the Cortex-M main/exceptions stack. This
# stack is used for processing interrupts and exceptions.
ifeq ($(USE_EXCEPTIONS_STACKSIZE),)
  USE_EXCEPTIONS_STACKSIZE = 0x400
endif

#
# Architecture or project specific options
##############################################################################

##############################################################################
# Project, sources and paths
#

SOURCES_ROOT      = src

DEADLOCK_BOARDS   = hal/boards
# TODO!
BOARD_FOLDER      = $(DEADLOCK_BOARDS)/reader-revA

# Define project name here
PROJECT = deadlock-reader

# Imported source files and paths
# Warning: order is important!
CHIBIOS = deps/ChibiOS
UNITY = deps/Unity/src/
FFF   = deps/fff/
DEADCOM_DCL2 = deps/libdeadcom/dcl2/lib/
DEADCOM_DCRCP = deps/libdeadcom/dcrcp/lib/
DEADCOM_CNCBOR = deps/libdeadcom/deps/cn-cbor/
CUSTOM_HAL = hal/

TEST_PATH = test/
TEST_BUILD = build/test/out/
TEST_RUNNERS = build/test/runners/
TEST_OBJS = build/test/objs/
TEST_RESULTS = build/test/results/
TEST_BUILD_PATHS = $(TEST_BUILD) $(TEST_OBJS) $(TEST_RESULTS) $(TEST_RUNNERS)
# So that we can test both src/ and hal/
TEST_ROOT = ./

# Startup files.
# TODO MCU-specific file should not be here!
include $(CHIBIOS)/os/common/startup/ARMCMx/compilers/GCC/mk/startup_stm32f0xx.mk
# HAL-OSAL files (optional).
include $(CHIBIOS)/os/hal/hal.mk
include $(CUSTOM_HAL)/hal.mk
# TODO MCU-specific file should not be here!
include $(CHIBIOS)/os/hal/ports/STM32/STM32F0xx/platform.mk
include $(BOARD_FOLDER)/board.mk
include $(CHIBIOS)/os/hal/osal/rt/osal.mk
# RTOS files (optional).
include $(CHIBIOS)/os/rt/rt.mk
include $(CHIBIOS)/os/common/ports/ARMCMx/compilers/GCC/mk/port_v6m.mk
# Other files (optional).
include $(CHIBIOS)/test/rt/test.mk

# C sources that can be compiled in ARM or THUMB mode depending on the global
# setting.
CSRC = $(STARTUPSRC) \
       $(KERNSRC) \
       $(PORTSRC) \
       $(OSALSRC) \
       $(HALSRC) \
       $(PLATFORMSRC) \
       $(BOARDSRC) \
	   $(shell find $(DEADCOM_DCL2)/src -type f -name '*.c') \
	   $(shell find $(DEADCOM_DCRCP)/src -type f -name '*.c') \
	   $(shell find $(DEADCOM_CNCBOR)/src -type f -name '*.c') \
	   $(shell find $(SOURCES_ROOT) -type f -name '*.c')

# C test sources.
TEST_CSRC = $(shell find $(TEST_PATH) -type f -regextype sed -regex '.*-test[0-9]*\.c')

# C++ sources that can be compiled in ARM or THUMB mode depending on the global
# setting.
CPPSRC =

# C sources to be compiled in ARM mode regardless of the global setting.
# NOTE: Mixing ARM and THUMB mode enables the -mthumb-interwork compiler
#       option that results in lower performance and larger code size.
ACSRC =

# C++ sources to be compiled in ARM mode regardless of the global setting.
# NOTE: Mixing ARM and THUMB mode enables the -mthumb-interwork compiler
#       option that results in lower performance and larger code size.
ACPPSRC =

# C sources to be compiled in THUMB mode regardless of the global setting.
# NOTE: Mixing ARM and THUMB mode enables the -mthumb-interwork compiler
#       option that results in lower performance and larger code size.
TCSRC =

# C sources to be compiled in THUMB mode regardless of the global setting.
# NOTE: Mixing ARM and THUMB mode enables the -mthumb-interwork compiler
#       option that results in lower performance and larger code size.
TCPPSRC =

# List ASM source files here
ASMSRC =
ASMXSRC = $(STARTUPASM) $(PORTASM) $(OSALASM)

INCDIR = include-overrides \
		 $(CHIBIOS)/os/license \
         $(STARTUPINC) \
		 $(KERNINC) \
		 $(PORTINC) \
		 $(OSALINC) \
         $(HALINC) \
		 $(PLATFORMINC) \
		 $(BOARDINC) \
		 $(TESTINC) \
         $(CHIBIOS)/os/various \
		 $(DEADCOM_DCL2)/inc \
		 $(DEADCOM_DCRCP)/inc \
		 $(DEADCOM_CNCBOR)/include \
		 $(SOURCES_ROOT) \
		 $(SOURCES_ROOT)/conf

#
# Project, sources and paths
##############################################################################

##############################################################################
# Compiler settings
#

#TRGT = arm-elf-
TRGT = arm-none-eabi-
CC   = $(TRGT)gcc
CPPC = $(TRGT)g++
# Enable loading with g++ only if you need C++ runtime support.
# NOTE: You can use C++ even without C++ support if you are careful. C++
#       runtime support makes code size explode.
LD   = $(TRGT)gcc
#LD   = $(TRGT)g++
CP   = $(TRGT)objcopy
AS   = $(TRGT)gcc -x assembler-with-cpp
AR   = $(TRGT)ar
OD   = $(TRGT)objdump
SZ   = $(TRGT)size
HEX  = $(CP) -O ihex
BIN  = $(CP) -O binary

TEST_TRGT   =
TEST_CC     = $(TEST_TRGT)gcc
TEST_LD     = $(TEST_TRGT)gcc
TEST_INCDIR = $(INCDIR) $(SOURCES_ROOT) $(FFF)
TEST_INCPARAMS = $(foreach d, $(TEST_INCDIR), -I$d)
TEST_CFLAGS = -I. -I$(UNITY) $(TEST_INCPARAMS) -DTEST

# ARM-specific options here
AOPT =

# THUMB-specific options here
TOPT = -mthumb -DTHUMB

# Define C warning options here
CWARN = -Wall -Wextra -Wundef -Wstrict-prototypes -Wimplicit-fallthrough=0

# Define C++ warning options here
CPPWARN = -Wall -Wextra -Wundef

#
# Compiler settings
##############################################################################

##############################################################################
# Start of user section
#

# List all user C define here, like -D_DEBUG=1
ifeq ($(DEBUG_BUILD),yes)
  UDEFS = -DDEBUG
else
  UDEFS =
endif

# Define ASM defines here
ifeq ($(DEBUG_BUILD),yes)
  UADEFS = -DDEBUG
else
  UADEFS =
endif

# List all user directories here
UINCDIR =

# List the user directory to look for the libraries here
ULIBDIR =

# List all user libraries here
ULIBS =

#
# End of user defines
##############################################################################

RULESPATH = $(CHIBIOS)/os/common/startup/ARMCMx/compilers/GCC
include $(RULESPATH)/rules.mk

##############################################################################
# Start of helper flash and debug commands
#

flash: build/deadlock-reader.bin
	st-flash erase
	st-flash write build/deadlock-reader.bin $(FW_FLASH_ADDRESS)

debug: build/deadlock-reader.elf
	pgrep st-util || st-util &
	arm-none-eabi-gdb build/deadlock-reader.elf -ex "target extended :4242"
	pkill st-util

debugl: build/deadlock-reader.elf
	pgrep st-util || st-util &
	arm-none-eabi-gdb build/deadlock-reader.elf -ex "target extended :4242" -ex "load" -ex "break chSysHalt" -ex "step"
	pkill st-util

#
# End of helper flash and debug commands
###############################################################################


###############################################################################
# Start of gpg signing helper commands
#

%.sig: %
	gpg --output $@ --detach-sig $<

sign: build/deadlock-reader.bin.sig build/deadlock-reader.elf.sig

#
# End of gpg signing helper commands
##############################################################################

##############################################################################
# Start of Unit Testing with Unity rules
#

$(TEST_BUILD):
	@mkdir -p $(TEST_BUILD)

$(TEST_OBJS):
	@mkdir -p $(TEST_OBJS)

$(TEST_RESULTS):
	@mkdir -p $(TEST_RESULTS)

$(TEST_RUNNERS):
	@mkdir -p $(TEST_RUNNERS)

$(TEST_RUNNERS)%-runner.c:: $(TEST_PATH)%.c
	@echo 'Generating runner for $@'
	@mkdir -p `dirname $@`
	@ruby $(UNITY)../auto/generate_test_runner.rb $< $@

$(TEST_OBJS)unity.o:: $(UNITY)unity.c $(UNITY)unity.h
	@echo 'Compiling Unity'
	@$(TEST_CC) $(TEST_CFLAGS) -c $< -o $@

$(TEST_OBJS)%.o:: $(TEST_PATH)%.c
	@echo 'Compiling test $<'
	@mkdir -p `dirname $@`
	@$(TEST_CC) $(TEST_CFLAGS) -c $< -o $@

$(TEST_OBJS)%-runner.o: $(TEST_RUNNERS)%-runner.c
	@echo 'Compiling test runner $<'
	@mkdir -p `dirname $@`
	@$(TEST_CC) $(TEST_CFLAGS) -c $< -o $@

$(TEST_OBJS)%-under_test.o: $(TEST_ROOT)%.c
	@echo 'Compiling $<'
	@mkdir -p `dirname $@`
	@$(TEST_CC) $(TEST_CFLAGS) -c $< -o $@
	@objcopy --weaken $@

.SECONDEXPANSION:
FILE_UNDER_TEST := sed -e 's|$(TEST_BUILD)\(.*\)-test[0-9]*\.out|$(TEST_OBJS)\1-under_test.o|'
$(TEST_BUILD)%.out: $$(shell echo $$@ | $$(FILE_UNDER_TEST)) $(TEST_OBJS)%.o $(TEST_OBJS)unity.o $(TEST_OBJS)%-runner.o
	@echo 'Linking test $@'
	@mkdir -p `dirname $@`
	@$(TEST_LD) -o $@ $^

$(TEST_RESULTS)%.result: $(TEST_BUILD)%.out
	@echo
	@echo '----- Running test $<:'
	@mkdir -p `dirname $@`
	@-./$< > $@ 2>&1
	@cat $@

RESULTS = $(patsubst $(TEST_PATH)%.c,$(TEST_RESULTS)%.result,$(TEST_CSRC))
TEST_EXECS = $(patsubst $(TEST_RESULTS)%.result,$(TEST_BUILD)%.out,$(RESULTS))

run-tests: $(TEST_BUILD_PATHS) $(TEST_EXECS) $(RESULTS)
	@echo
	@echo "----- SUMMARY -----"
	@echo "PASS:   `for i in $(RESULTS); do grep -s :PASS $$i; done | wc -l`"
	@echo "IGNORE: `for i in $(RESULTS); do grep -s :IGNORE $$i; done | wc -l`"
	@echo "FAIL:   `for i in $(RESULTS); do grep -s :FAIL $$i; done | wc -l`"
	@echo
	@echo "DONE"
	@if [ "`for i in $(RESULTS); do grep -s FAIL $$i; done | wc -l`" != 0 ]; then \
	exit 1; \
	fi

clean-tests:
	@rm -rf $(TEST_BUILD) $(TEST_OBJS) $(TEST_RESULTS) $(TEST_RUNNERS)

test: run-tests clean-tests

print_tcsrc:
	echo $(TEST_CSRC)

#
# End of Unit Testing with Unity rules
##############################################################################

.PHONY: test run-tests clean-tests print_tcsrc sign
