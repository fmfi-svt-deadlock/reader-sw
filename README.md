# Reader firmware

[![Build status](https://travis-ci.org/fmfi-svt-deadlock/reader-sw.svg?branch=master)](https://travis-ci.org/fmfi-svt-deadlock/reader-sw)
[![Docs](https://readthedocs.org/projects/pip/badge/?version=latest)](http://deadlock-reader-sw.readthedocs.io/en/latest/)

Firmware for the Reader component of project Deadlock. For the project overview, and more information about what the reader does and what communication protocols it utilizes see https://github.com/fmfi-svt-deadlock/server/wiki.

## Submodules

Dependencies (ChibiOS, testing framework) are submodules of this repository. Either clone with `--recursive` or run `git submodule update --init --recursive` after cloning / checkout to get them.

## Documentation

The documentation (including internal documentation) is built using [Sphinx](http://www.sphinx-doc.org/). Current version is also available on deadlock-reader-sw.readthedocs.com.

If you want to build the documentation for yourself install doxygen and then go to folder `docs/` and do the following:

```bash
# Create a new virtualenv
python3 -m venv _venv
source _venv/bin/activate
# Install dependencies
pip install -r requirements.txt
# Build the docs
make html
```

This will build the documentation (including source code documentation, it will run doxygen as a part of the build process).

### ChibiOS STM32F0xx port documentation

If you want to hack on this firmware, you may need ChibiOS documentation as well. Also specific documentation for the
STM32F0xx port of the HAL may be useful. To get that modify tag `INPUT=` in `doc/hal/Doxyfile_html` in the ChibiOS root as follows:
  - Remove `../../os/hal/templates`
  - Add `../../os/hal/ports/STM32/STM32F0xx`
  - Add all relevant folders in `../../os/hal/ports/STM32/LLD`. You can find which folders are relevant by looking at `PLATFORMINC` filed in `os/hal/ports/STM32/STM32F0xx/platform.mk`.

Then build documentation as described in `doc/hal/readme.txt`.

## Compiling

This firmware is based on the ChibiOS (http://www.chibios.org/).

To compile this firmware without docker:
  - Install the appropriate arm toolchain, see ChibiOS documentaion for the
    list of supported compilers.
    - This project is tested on the GNU Compiler Collection `arm-none-eabi-` toolchain (ARM EABI, bare-metal). If you choose this toolchain, you will need:
      - `arm-none-eabi-gcc` (tested on gcc version 7.3.0 (Arch Repository))
      - `arm-none-eabi-binutils` (tested on 2.30)
      - `arm-none-eabi-newlib` (tested on 3.0.0)
  - Edit the `Makefile`:
    - set `BOARD_FOLDER = ` variable to the name of the board you want to compile this firmware for.
  - `make` the project.

Alternatively you can use Docker image `projectdeadlock/embedded-fw-builder:latest` (available in
Docker Hub) which provides everything needed to build this fw. Example invocation:

```
docker run -t -v `pwd`:/srv -u `id -u` sh -c "cd /srv && make clean && make"
```

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

## Testing

This project uses [ThrowTheSwitch/Unity](https://github.com/ThrowTheSwitch/Unity) for unit tests and [meekrosoft/fff](https://github.com/meekrosoft/fff) for mocking.

Run `make test` to execute these tests.

### Writing tests

Test files are in the `test/` folder. The `test/` folder has the same internal folder structure as this project.

Let's say you want to test file `hal/src/myawesome_driver.c`. Then create file `test/hal/src/myawesome_driver-test.c`. The path and filename must match, and the file must have a `-test.c` suffix. There can be at most one test file per source file. This file should contain the following:

  - `include "myawesome_driver.h"`: Include of header of file under test
  - FFF mocks of required functions
  - `void setUp(void)`: Run before every test
  - `void tearDown(void)`: Run after every test
  - `test.*` or `should.*` or `spec.*`: Tests themselves (returning void, no args).

`test/src/main-test.c` does nothing useful, but serves as an example you can base your own test files on.

## License

The firmware itself is licensed under the MIT license unless noted otherwise
in the file header.

Most notably files `chconf.h`, `halconf.h`, `mcuconf.h`, `boards/reader-revA/board.h`
and `boards/reader-revA/board.c` are licensed under the Apache 2.0 license with
Project Deadlock-related changes sublicensed under the MIT license.
