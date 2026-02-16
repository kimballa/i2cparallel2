// (C) Copyright 2021-2026 Aaron Kimball
// This library is licensed under the terms of the BSD 3-Clause license.
// See the accompanying LICENSE.txt file for full license text.
//
// PCF8574 / PCF8574A Implementation
// See datasheet: https://www.ti.com/lit/ds/symlink/pcf8574a.pdf
//
// To use, include I2CParallel2.h, and instantiate an I2CParallel8574 object.

#include <Arduino.h>
#include <Wire.h>
#include <cstdint>

#include "I2CParallel2.h"

// The time delay from I2C acknowledge until the output is valid.
// (Also the hold time needed for driven inputs before the data can be reported
// back.)
static constexpr unsigned int I2C_PARALLEL_HOLD_TIME_MICROS = 4;

// Always end our i2c transmissions with the STOP signal.
static constexpr uint8_t SEND_STOP = 1;

// Use only the 7 less-significant bits of the address.
// This will be left-shifted by 1 and a r/w flag bit appended as the lsb
// for actual communication.
static constexpr uint8_t I2C_PARALLEL_ADDR_MASK = 0x7F;

// Timeout duration for I2C communications in microseconds; use 25 ms.
static constexpr unsigned int I2C_PARALLEL_WIRE_TIMEOUT = 25000;

void I2CParallel8574::init(const uint8_t i2cAddr, const uint32_t busSpeed) {
  _error = I2C_PARALLEL_ERR_OK; // Clear any previous errors.
  _i2cAddr = i2cAddr & I2C_PARALLEL_ADDR_MASK;

  if (_i2cAddr < I2C_PCF8574_MIN_ADDR ||
      (_i2cAddr > I2C_PCF8574_MAX_ADDR && _i2cAddr < I2C_PCF8574A_MIN_ADDR) ||
      _i2cAddr > I2C_PCF8574A_MAX_ADDR) {
    // Invalid I2C address range.
    _error = I2C_PARALLEL_ERR_ADDR;
  }

  if (busSpeed > I2C_PARALLEL_MAX_BUS_SPEED) {
    // '8574 devices are not known to support speeds higher than 400kHz.
    _error = I2C_PARALLEL_ERR_BUS_SPEED;
  }

  Wire.setClock(busSpeed);
#if defined(ARDUINO_ARCH_SAMD) || defined(ARDUINO_TEENSY41)
  Wire.setTimeout(I2C_PARALLEL_WIRE_TIMEOUT);
#else
  // This is the default for the __ARCH_AVR__ Wire library.
  Wire.setWireTimeout(I2C_PARALLEL_WIRE_TIMEOUT, true);
#endif
}

size_t I2CParallel8574::setByte(const uint8_t val) {
  size_t numWritten = 0;
  if (_i2cAddr == UNINITIALIZED_I2C_ADDR) {
    // Only transmit if we have initialized the i2c bus.
    _error = I2C_PARALLEL_ERR_UNINITIALIZED;
  } else {
    Wire.beginTransmission(_i2cAddr);
    numWritten = Wire.write(val);
    if (numWritten != 1) {
      _error = I2C_PARALLEL_ERR_BUS_IO;
    }
    Wire.endTransmission(SEND_STOP);
  }
  // Update the local "intended output state" regardless of whether the write
  // succeeded.
  _outputState = val;
  return numWritten;
}

uint8_t I2CParallel8574::getByte(uint8_t &nBytesRead) {
  nBytesRead = 0;
  if (_i2cAddr == UNINITIALIZED_I2C_ADDR) {
    // Only actually perform I/O if I2C has been initialized.
    _error = I2C_PARALLEL_ERR_UNINITIALIZED;
  } else {
    // Request 1 byte of data from the "read address" of the device
    // (@ write_addr + 1)
    nBytesRead = Wire.requestFrom((uint8_t)_i2cAddr, (uint8_t)1);
    if (nBytesRead != 1) {
      // Do not update the _inputState; keep it at the last-known value.
      // The error flag and nBytesRead <- 0 indicate to the caller that this
      // value is not trustworthy.
      _error = I2C_PARALLEL_ERR_BUS_IO;
    } else {
      _inputState = Wire.read();
    }
  }
  return _inputState;
}

void I2CParallel8574::enableInputs(const uint8_t mask) {
  // Quasi-bidirectional I/O: set the specified bits high to enable inputs.
  setOr(mask);
};

void I2CParallel8574::waitForValid() {
  delayMicroseconds(I2C_PARALLEL_HOLD_TIME_MICROS);
}
