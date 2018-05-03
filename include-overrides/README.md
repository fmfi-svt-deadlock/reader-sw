Include overrides ugly hack
===========================

Some libraries we depend on use includes which are either unavailable when compiling for
embedded MCUs or have incorrect implementation. This folder contains fake headers which
override these includes and provide proper implementation.

  - `assert.h`: Required by `cn-cbor` for function `assert()`. We define this function to use
    `chDbgAssert()` instead.
  - `arpa/inet.h`: Required by `cn-cbor` for byte-order conversion functions. These functions
    are provided by `machine/endian.h` in our toolchain, so we define them to use those.
