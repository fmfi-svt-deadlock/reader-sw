Automated testing of the firmware
=================================

We want to test our code. However, testing code for embedded devices is not easy since in production environment the code runs on an embedded hardware. There are basically 3 options:

  - Compile the code for PC and run tests locally
  - Run tests on an emulator of given embedded platform
  - Run tests on physical embedded hardware

Some of them have advantages, all of them have disadvantages.

Compiling code for the PC and running the tests locally is very fast compared to other options. However, only hardware-independent pieces of code can be tested in this way. Moreover, since different compiler is used to compile the unit tests these tests won't catch compiler-introduced bugs (which is not unheard of in embedded development). However, logical error is still a logical error no matter the platform or compiler, so these tests can test business-logic well. Although it is possible to test higher-layers of device drivers by mocking the comm interface and emulating the hardware, it is pointless since it is way harder to write a flawless hardware emulator (bug-for-bug compatible with the real hardware) than it is to write a solid driver for it.

Running code on an emulator has an advantage of using the same compiler for both tests and production code. Also the registers may be changed at will and error conditions introduced. Unfortunately, we have not found suitable emulator for our platform.

Running tests on physical embedded device is the most difficult approach. Physical hardware can't be forced to deterministically create a specific failure mode through software when debugging drivers. Collecting test results is also difficult. Unfortunatelly, this is often the only applicable choice.

### Chosen testing strategy

We want to do 3 types of testing: unit testing, integration testing and system testing.

Let's start from the lowest layer: device drivers. They will not have automated tests for reasons described above. We won't write many drivers, just one or two, since we are using ChibiOS HAL. They will be tested manually as much as possible though.

The firmware has modular design. Each module runs in its own thread and communicates with other modules using mailboxes. Therefore each module can be separated from other modules and tested on its own. This way we can test how well units integrate into modules and with ChibiOS. These integration tests will run on physical embedded hardware, since ChibiOS must run to run the thread, handle the mailboxes, etc.

Modules can be broken down to units, and these will be unit tested. Unit tests will run on a PC so that they can be written, debugged and run easily.

System testing will be entirely different. Production firmware will run on a production hardware and the whole reader will be treated as a black box. Interaction with other components over external interfaces will be tested using a bed of nails.

Unit tests
----------

Unit tests are built on the [Unity](http://www.throwtheswitch.org/unity) unit testing framework and is using the [Fake Function Framework](https://github.com/meekrosoft/fff).

Unit tests are stored in the `test/` folder. This folder contains subfolders `src/` and `hal/`. Overally, folder structure in the `test/` folder is the same as the folder structure of the sources. For each source file there may be several test files (with the same name as the file under test with suffix `--testNUM.c`).

### Technicalities of unit-testing C code with mocking

If one `.c` file represents one unit then unit-testing is relatively easy: compile the file under test, compile the test file, let the test file define mock functions, link them. Test file will call a function from the file under test, it will call some library function which is provided by the mock in the test file.

However, usually a single `.c` file is a single module whereas a function is an unit. Functions in a `.c` file may use other functions from that file, so in order to write an unit test for given function from the `.c` file, other functions from this `.c` file which are called by the function under test may need to be mocked. Even worse, these functions may be static (due to optimalizations). There are several solutions to this problem:

  - Split the `.c` file into multiple files so that each file is an unit. Advantage is that building and writing tests is easy. Also it may encourage better and more modular design. Disadvantages are that you need to produce more  `.c` files and the code may be harder to write. Also static functions can't be used.
  - You can use (through some preprocessor magic like custom defined `testable` macro or by `objcopy --weaken`) weak references. You can then provide your own implementation of custom symbols in the test file and linker will replace weak references in the file under test. Advantage is that writing tests is easy, disadvantage is that you may need magic macros or a bit more complicated build process. You still need preprocessor magic to solve `static` functions, as compiler may do what it wants with `static` functions (such as inline them or change their calling convention).
  - You can manipulate (rape?) the GOT table at runtime and force the running program to use your function instead. Building tests is easy, writing them is hard and the whole process is messy. Still won't solve the problem with `static` functions.

We will do the following: where possible and logical we will split the code to multiple `.c` files. It is quite possible that the split will not be needed in most cases, and when it is needed the file should anyway be logically splitted. Additionally the file will be processed with `objcopy --weaken`, so if it **really** is not logical to split the file under test it is still possible to mock the internal functions. Problem with `static` functions will be solved using `testable_static` macro (where it is warranted), which resolves to `static` when compiled normally, otherwise it will resolve to nothing.

### Used mocking framework

We are using [Unity](http://www.throwtheswitch.org/unity) unit testing framework. This framework is also used by [CMock](http://www.throwtheswitch.org/cmock/) mocking framework, from the same creators, so it would be logical to use it.

That, however, turned out to be a bit problematic.

The CMock is able to parse a header file and produce a mock file for functions it finds. That automatically implies that CMock treats one file as one unit and makes it impossible to mock functions inside that file.

The other problem is the build system. The mocks should be generated automatically and tests should be build automatically as well. Officially recommended option is to use Ceedling as a build system. However, this is not applicable to this project. This firmware is based on ChibiOS, build system of which is based on Makefiles including other ChibiOS-specific makefiles from within ChibiOS folder structure. CMock supports Makefiles, however it is quite inconvenient to use it. It works by generating a new Makefile using a Ruby script. This script makes assumptions about the project which don't hold true for this firmware (e.g. flat source code structure). Moreover, it was required to run `make` twice for this to work. The last option is to use ruby build system `rake`. This was actually feasible, since you are free to write what you want, and the example was easily modified to build the tests with our code structure. Rake was invoked from the Makefile with proper enviroment variables (because they are known only during `make` execution). However, this solution is not elegant since it mixes two different build systems.

All that would be still quite acceptable. However, the biggest problem is that CMock doesn't work with ChibiOS. This is a somewhat innacurate statement, I'll explain. CMock is able to parse a header file and find function definitions within. However, it is not able (nor it should be able) to preprocess that file. In case of ChibiOS one includes only `ch.h`. Other parts of the system are included by this header. Of course, CMock won't look for them and will only see file without function declarations. Workaround around this was to include a real ChibiOS header file, however it was somewhat inelegant, and not universally applicable. Due to preprocessor magic the resulting set of included header files is dependent on compile-time definitions. This makes perfect sense for embedded projects, as it minimizes footprint of the compiled code. However, it also means that in order to reliably generate mocks the files have to be preprocessed. Official advice on this was to run the header through the C preprocessor. So I've done that, and then CMock choked itself with a parsing error on the resulting file.

That's when I've decided that although CMock is a decent framework it is not applicable for my use case.

I've decided to use the [Fake Function Framework](https://github.com/meekrosoft/fff) instead. It's usage: `#include "fff.h"`. Done. Works. I have to do mocks manually, however, i **can** do mocks manually. Usually I don't have to mock that many functions anyway, and all it takes is `FAKE_VOID_FUNC(halInit);`. And if I forget to mock something the linker kicks me in the balls. Bonus: I can use the weak-reference trick and mock functions from the same file that the function under test comes from.

### Building and running tests

Unity includes an example Makefile which can be used to build and run unit tests. It needs to be modified to be usable in our case, because:

  - It presumes a flat source / test folder structure
  - It doesn't automatically generate test runners
  - It doesn't automatically weaken references of object file under test
  - It supports only one test for each file

So, this will be explanation very similar to the [official Unity Makefile explanation](http://www.throwtheswitch.org/build/make), however modified for our purposes.

Let's start by finding all test source files in the test folder with suffix `-testNUM.c`.

```make
TEST_CSRC = $(shell find $(TEST_PATH) -type f -regextype sed -regex '.*-test[0-9]*\.c')
```

For each of this test file we will want to create a `.result` file which will store the results. Folder structure in any of the temp folders (either `results` folder or any other test build folder) will be the same as the one in the `test/` directory.

```make
RESULTS = $(patsubst $(TEST_PATH)%.c,$(TEST_RESULTS)%.result,$(TEST_CSRC))
```

Now let's begin from the end. At the and we will want to print the test statistics by reading all `.results` files.

```make
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
```

This recipe of course depends on all result files. Other dependencies are a bit hacky:

  - `$(TEST_BUILD_PATHS)`: folders temporarily build files live in
  - `$(TEST_EXECS)`: test executables. Technically this should not be here, since each `.result` file should depend on a single executable file. However, this causes executable files to be build in bulk before the tests are run and that makes the test output much more readable and less cluttered.

Each `.result` file is created by executing a single test:

```make
$(TEST_RESULTS)%.result: $(TEST_BUILD)%.out
    @echo
    @echo '----- Running test $<:'
    @mkdir -p `dirname $@`
    @-./$< > $@ 2>&1
    @cat $@
```

Each executable test is created by linking the following:

  - Test runner (contains the entry point and executes tests)
  - Unity framework
  - Test file
  - File under test

```make
.SECONDEXPANSION:
FILE_UNDER_TEST := sed -e 's|$(TEST_BUILD)\(.*\)-test[0-9]*\.out|$(TEST_OBJS)\1-under_test.o|'
$(TEST_BUILD)%.out: $$(shell echo $$@ | $$(FILE_UNDER_TEST)) $(TEST_OBJS)%.o $(TEST_OBJS)unity.o $(TEST_OBJS)%-runner.o
    @echo 'Linking test $@'
    @mkdir -p `dirname $@`
    @$(TEST_LD) -o $@ $^
```

Now there is a bit of Makefile hackery going on there. This recipe generates an executable which runs tests, named for example `build/tests/out/src/main-test12.out`. It does that by linking `build/test/objs/unity.o`, `build/test/objs/src/main-test12.o` and `build/test/objs/src/main-test12-runner.o`. It also needs `build/test/objs/src/main-under_test.o`, which is the file under test.

The question is how to get the name of the file under test. If there was only one test file per source file it would be easy, `%` could be used as with other things. However, there are more test files per source file. Name of the test file must match a certain pattern (regex `\(.*\)-test[0-9]*\.out`), and this pattern needs to be replaced. This is not possible with make's `%`. That's why we use the `sed` command, which makes for example `build/test/objs/src/main-under_test.o` from `build/tests/out/src/main-test12.out`.

The last problem is how to run `%` or `$@` through the shell with sed. Makefiles are [read in two phases](https://www.gnu.org/software/make/manual/html_node/Reading-Makefiles.html): in the first phase make reads in the Makefile and includes, expands so-called `immediate` variables and constructs a dependency graph. In the second phase make determines what needs to be rebuilt and rebuilds it. The problem is that recipe name and dependencies are immediate variables, therefore they are expanded immediatelly before the dependency tree is known internally. Therefore, during this expansion `%` nor `$@` is **not** set and can't be used in the `shell` directive. The workaround is to use the [secondary expansion](https://www.gnu.org/software/make/manual/html_node/Secondary-Expansion.html) feature (hack), which runs between the 2 phases, when the recipe name is already expanded and the value of `$@` is set. To use this one needs to use the `.SECONDEXPANSION:` rule and rules defined below this rule will be expanded twice. Variables with a single dollar sign will be expanded during the first expansion. When the first expansion sees two dollar signs it interprets it as escaped dollar sign and so it produces a single dollar sign. Then when the second expansion runs variables which are already expanded are treated as text and are left alone, and when it sees single dollar sign (created from the double dollar sign by the previous run) it expands this variable. This way we can force make to have variable `$@` set when calling shell and therefore generate the proper dependency name from recipe name.

Now that we know what we need, let's build it. Building the test file, unity framework and test runner is trivial:

```make
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
```

Test runner is generated automatically, so in order to build it we must generate it from the test file. Unity provides us with a script for this purpose.

```make
$(TEST_RUNNERS)%-runner.c:: $(TEST_PATH)%.c
    @echo 'Generating runner for $@'
    @mkdir -p `dirname $@`
    @ruby $(UNITY)../auto/generate_test_runner.rb $< $@
```

Last thing to do is to build the file under test itself:

```make
$(TEST_OBJS)%-under_test.o: $(TEST_ROOT)%.c
    @echo 'Compiling $<'
    @mkdir -p `dirname $@`
    @$(TEST_CC) $(TEST_CFLAGS) -c $< -o $@
    @objcopy --weaken $@
```

The difference is that there is an additional step `objcopy --weaken` for reasons described above.

And that's it, except minor details like creating folders and cleaning up. Several things are a low priority and are not finished:

  - There is no `testable_static` macro. It will be added when it is needed.
  - There is no dependency management. There is a rule which cleans everything after the tests have run. Proper dependency management is hard as hell, and it is easier and more reliable to rebuild it every time. Tests are built really fast, executed really fast and can even be executed individually if needed. If this becomes a real problem dependency management can be added.

Note: This is the ugliest piece of Makefile I've ever written and I'm seriously considering rewriting it to different build system. But for now it works, is maintainable and well documented, and there are more pressing things to do (like writing some **actual code for the project**).

Integration testing
-------------------

I will get around to it once I have some actual modules to test.
