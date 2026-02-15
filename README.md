
i2cparallel
===========

An Arduino driver library for the PCF8574 / PCF8574A / PCA8574 family of 8-bit parallel ICs
you can interface over I2C.

Dependencies
------------

This requires only the Arduino `Wire` library.

Compiling
---------

I build this with my [Arduino makefile](https://github.com/kimballa/arduino-makefile).

* Clone the makefile project such that `arduino-makefile/` is a sibling of this project directory.
* Create `~/arduino_mk.conf` from the template in that directory and customize it to your board
  and local environment. See other one-time setup instructions in that project's README.md and/or
  the comment header of `arduino.mk`.
* You also need to compile `Wire` for your architecture by running `arduino-makefile/install-wire.sh`.
* Build this library with `make install`

Usage
-----

* Include `I2CParallel.h` in your sketch source file.
* Add `libs := Wire i2cparallel` to your arduino.mk-driven Makefile.

License
-------

This project is licensed under the BSD 3-Clause license. See LICENSE.txt for complete details.
