Reader Firmware Architecture
============================

This firmware has several goals:

  - Robustness
  - Maintainability
  - Extensibility
  - Testability

Good software architecture is instrumental in achieving those goals. In order to be extensible and testable the firmware must be modular. In order to be robust, those modules must be watched over and be restartable in case of a problem. In order to be maintainable, internal functioning of those modules must be well-documented, and they must have clearly defined interfaces to the rest of the system.

This firmware is built on top of a [ChibiOS](http://www.chibios.org). ChibiOS provides threading, synchronization primitives and a very feature-rich HAL. Using the threading paradigm simplifies design of modules and helps to isolate them from the rest of the system. It also makes implementing a watchdog to watch over those modules (and restart them ifneedbe) a simple task.

Reader types, Cores, Modules and Flavours
-----------------------------------------

Primary task of the Reader is to obtain user identification data (usually by reading an RFID card), send the identification data to the Controller and inform the user of an authentication result.

Reader is a replaceable component of a larger system. Therefore multiple versions of the Reader may exist. The basic version is only able to read ID of a ISO/IEC 14443-3 (A) compliant card and inform the user of the result using a simplistic user interface consisting of a speaker and two bi-color LEDs. However, in the future, there may be a version of the Reader which authenticates users using fingerprints and a personal code entered on a keyboard, displaying results on a color LCD.

It is presumed that each Reader type will have a fundamentally different business logic and therefore can be uniquely identified by that logic. A business logic will be referred to as a Core from now on. Cores are stored in folder `src/cores`.

A Core will use one or more Modules. Module is a piece of code, usually run as a thread that Does One Thing and Does It Well. Modules are stored in folder `src/modules`.



### Basic ISIC/ITIC Reader

`src/cores/basic-isic-reader`

This is the only Reader version in existence right now. It can read ISO/IEC 14443-3 (A) compliant cards. It has a speaker and two bi-color LEDs.
