
I2CParallel v2
==============

An Arduino driver library for various families of 8-bit parallel ICs
you can interface over I2C.

Supported hardware:
-------------------

### Quasi-bidirectional I/O

* PCF8574
* PCF8574A
* PCA8574
* PCA8574A

### Programmable I/O

* PCA9534
* PCA9534A
* TCA9534
* TCA9534A
* PCA9554
* PCA9554A
* TCA9554
* TCA9554A

### Programmable I/O and RESET\_L pin

* PCA9538
* TCA9538
* PCA6408A
* TCA6408A


Dependencies
------------

This requires only the Arduino `Wire` library.

Differences in hardware
-----------------------

The various IC part numbers enumerated above represent families of related parts. Many of them are drop-in
replacements for one another from a pin-compatibility standpoint.

Some general rules:
* The "TCAxxxx" chips are Texas Instruments-specific "improved" versions of the analogous PCAxxxx part.
  (Tighter timing, lower power consumption, etc.)
* Each part ending in "A" is identical to its non-A counterpart but for having a different fixed component
  in its I2C address. e.g., if you use all 8 available addresses for PCA9534 ICs, you can add 8 more
  bus expanders just by switching to PCA9534A for #9-16.

**8574** is an older bus-expander component that uses quasi-bidirectional I/O. The IC has a single
register for the 8-bit output value. Setting an output pin to the `1` state uses a 100uA current
source to drive the high output state (instead of using a PMOSFET connected directly to Vcc), which
also allows an external source to drive the value so the '8574 can read the pin as an input.

This is a simpler chip, but makes startup more complicated as the pins output logic high on startup,
requiring pull-down resistors if logic-low is required on startup, which can decrease the output voltage
range (V_OH,min).

**9534** is the "base" programmable I/O component. It supports separate input, output, and configuration
registers. Pins start High-Z (i.e., configured as inputs). A configuration register sets each pin as an
input or output. Output values are latched from the output register, and reads are performed by latching
the pins, saving state to an input register, and reading that back to the I2C master. A separate "polarity"
configuration register instructs the device to invert the latched values before reading them back over I2C.

**9538** is identical to the '9534 but includes a `RESET_L` pin for asynchronous hardware reset.

**9554** is identical to '9534 but has weak (~100kOhm) pullup resistors for each output pin
*integrated on the device.

**6408A** is identical to the '9534 but includes a `RESET_L` pin for asynchronous hardware reset. This
device also has separate Vcc's for the I2C bus and logic and for the I/O port, allowing it to translate
voltage levels up or down from the I2C bus' common voltage layer.

As an exception to the naming conventions described earlier, there is no "base" PCA6408. Texas Instrument's
TCAL6408 is compatible with the '6408A, but includes several additional features for programmable GPIO
including output drive strength, and configurable pullup/pulldown resistors.

Using in Arduino projects
-------------------------

Select the I2CParallel2 library in the library manager and add `#include <I2CParallel2.h>` at the
top of your sketch.

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

* Include `I2CParallel2.h` in your sketch source file.
* Add `libs := Wire i2cparallel` to your arduino.mk-driven Makefile.

License
-------

This project is licensed under the BSD 3-Clause license. See LICENSE.txt for complete details.

Original version
----------------

This is version 2.0 of the I2CParallel library. v1 is available at https://github.com/kimballa/i2cparallel.
