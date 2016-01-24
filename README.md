# Reader firmware

Firmware for the Reader component of project Deadlock. For the project overview, and more information about what the reader does and what communication protocols it utilizes see https://github.com/fmfi-svt-deadlock/server/wiki.

## Documentation

The documentation (including internal documentation) is available on the project wiki https://github.com/fmfi-svt-deadlock/server/wiki.

## Compiling

This firmware is based on the ChibiOS (http://www.chibios.org/).

To compile this firmware:
  - Install the appropriate arm toolchain, see ChibiOS documentaion for the
    list of supported compilers.
    - This project is tested on the GNU Compiler Collection `arm-none-eabi-` toolchain (ARM EABI, bare-metal). If you choose this toolchain, you will need:
      - `arm-none-eabi-gcc` (tested on gcc version 5.3.0 (Arch Repository))
      - `arm-none-eabi-binutils` (tested on 2.25.1)
      - `arm-none-eabi-newlib` (tested on 2.3.0.20160104)
  - Download and extract the ChibiOS.
    - The project is tested on ChibiOS 16.1.2 and may not work on other versions.
  - Edit the `Makefile`:
    - set `CHIBIOS = ` variable to point to the extracted ChibiOS.
    - set `BOARD = ` variable to the name of the board you want to compile this firmware for.
  - `make` the project.

## Flashing the firmware

After building the firmware you can use any STM32-compatible flashing tool and hardware.
The following guide is for using `STLINKv2` and https://github.com/texane/stlink tools.

  - Download and install https://github.com/texane/stlink
  - Connect the debugger (make sure you have proper udev rules set, if applicable)
  - `st-flash write build/deadlock-reader.bin 0x08000000`

## Debugging

Again, you can use any of the compatible debugging tools.
The following guide is again for `stlink`.

  - Download and install https://github.com/texane/stlink
  - Download and install `arm-none-eabi-gdb`
  - Connect the debugger (check udev rules)
  - Launch `st-util`
  - `arm-none-eabi-gdb build/deadlock-reader.elf`
  - `(gdb) target extended :4242`

For further information please consult `stlink` documentation.


## License

The firmware itself is licensed under the MIT license unless noted otherwise
in the file header.

Most notably files `chconf.h`, `halconf.h`, `mcuconf.h`, `boards/reader-revA/board.h`
and `boards/reader-revA/board.c` are licensed under the Apache 2.0 license with
Project Deadlock-related changes sublicensed under the MIT license.
