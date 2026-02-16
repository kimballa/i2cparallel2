// (C) Copyright 2021-2026 Aaron Kimball
// This library is licensed under the terms of the BSD 3-Clause license.
// See the accompanying LICENSE.txt file for full license text.
//
// PCA9534 / PCA9554 Implementation
// Also covers TCA9534 and TCA9554, and "A" address variants of each.
// See datasheet: https://www.ti.com/lit/ds/symlink/pca9534.pdf
//
// To use, include I2CParallel2.h, and instantiate an I2CParallel9534 object.

#include <Arduino.h>
#include <Wire.h>
#include <cstdint>

#include "I2CParallel2.h"

// The time delay from I2C acknowledge until the output is valid (350ns).
// delayMicroseconds(1) is a conservative over-estimate on platforms without delayNanoseconds.
static constexpr unsigned int I2C_PARALLEL_HOLD_TIME_MICROS = 1;

static constexpr uint8_t POLARITY_INVERTED = 0x01;
static constexpr uint8_t POLARITY_NORMAL = 0x00;

static constexpr uint8_t CONFIG_DIRECTION_INPUT = 0x01;
static constexpr uint8_t CONFIG_DIRECTION_OUTPUT = 0x00;

// Register addresses for the command byte to send to the device.
static constexpr uint8_t REG_INPUT = 0x00;
static constexpr uint8_t REG_OUTPUT = 0x01;
static constexpr uint8_t REG_CONFIG = 0x02;
static constexpr uint8_t REG_POLARITY = 0x03;

// Always end our i2c transmissions with the STOP signal.
static constexpr uint8_t SEND_STOP = 1;

// Use only the 7 less-significant bits of the address.
static constexpr uint8_t I2C_PARALLEL_ADDR_MASK = 0x7F;

// Timeout duration for I2C communications in microseconds; use 25 ms.
static constexpr unsigned int I2C_PARALLEL_WIRE_TIMEOUT = 25000;

void I2CParallel9534::init(const uint8_t i2cAddr, const uint32_t busSpeed) {
  _error = I2C_PARALLEL_ERR_OK;
  _i2cAddr = i2cAddr & I2C_PARALLEL_ADDR_MASK;

  if ((_i2cAddr < I2C_PCA9534_MIN_ADDR || _i2cAddr > I2C_PCA9534_MAX_ADDR) &&
      (_i2cAddr < I2C_PCA9534A_MIN_ADDR || _i2cAddr > I2C_PCA9534A_MAX_ADDR) &&
      (_i2cAddr < I2C_PCA9538_MIN_ADDR || _i2cAddr > I2C_PCA9538_MAX_ADDR)) {
    _error = I2C_PARALLEL_ERR_ADDR;
  }

  if (busSpeed > I2C_PARALLEL_MAX_BUS_SPEED) {
    _error = I2C_PARALLEL_ERR_BUS_SPEED;
  }

  Wire.setClock(busSpeed);
#if defined(ARDUINO_ARCH_SAMD) || defined(ARDUINO_TEENSY41)
  Wire.setTimeout(I2C_PARALLEL_WIRE_TIMEOUT);
#else
  Wire.setWireTimeout(I2C_PARALLEL_WIRE_TIMEOUT, true);
#endif
}

size_t I2CParallel9534::setByte(const uint8_t val) {
  size_t numWritten = 0;
  if (_i2cAddr == UNINITIALIZED_I2C_ADDR) {
    _error = I2C_PARALLEL_ERR_UNINITIALIZED;
  } else {
    Wire.beginTransmission(_i2cAddr);
    numWritten = Wire.write(REG_OUTPUT);
    if (numWritten == 1) {
      numWritten += Wire.write(val);
    }
    if (numWritten != 2) {
      _error = I2C_PARALLEL_ERR_BUS_IO;
      numWritten = 0;
    } else {
      Wire.endTransmission(SEND_STOP);
    }
  }
  _outputState = val;
  return (numWritten == 2) ? 1 : 0;
}

uint8_t I2CParallel9534::getByte(uint8_t &nBytesRead) {
  nBytesRead = 0;
  if (_i2cAddr == UNINITIALIZED_I2C_ADDR) {
    _error = I2C_PARALLEL_ERR_UNINITIALIZED;
    return _inputState;
  }

  // 9534 read protocol: write register byte -> ACK -> repeated start -> read addr -> read byte
  Wire.beginTransmission(_i2cAddr);
  Wire.write(REG_INPUT);
  Wire.endTransmission(false);  // Repeated start, no STOP

  nBytesRead = Wire.requestFrom((uint8_t)_i2cAddr, (uint8_t)1);
  if (nBytesRead != 1) {
    _error = I2C_PARALLEL_ERR_BUS_IO;
    return _inputState;
  }
  _inputState = Wire.read();

  // TI PCA9534, PCA9538 interrupt bug: INT pin does not work if last-accessed register is REG_INPUT.
  // Do a write to REG_OUTPUT so the device's internal pointer is no longer on REG_INPUT.
  // See e.g. https://www.ti.com/lit/ds/scps126g/scps126g.pdf section 7.2.4.1 "Interrupt Errata".
  Wire.beginTransmission(_i2cAddr);
  Wire.write(REG_OUTPUT);
  Wire.endTransmission(SEND_STOP);

  return _inputState;
}

void I2CParallel9534::enableInputs(const uint8_t mask) {
  if (_i2cAddr == UNINITIALIZED_I2C_ADDR) {
    _error = I2C_PARALLEL_ERR_UNINITIALIZED;
    return;
  }

  // Write the specified input mask to the CONFIG register.
  Wire.beginTransmission(_i2cAddr);
  Wire.write(REG_CONFIG);
  Wire.write(mask);
  Wire.endTransmission(false);
}

void I2CParallel9534::waitForValid() {
  // conservative delay (> 350ns hold time) to ensure propagation of output register to pins.
  delayMicroseconds(I2C_PARALLEL_HOLD_TIME_MICROS);
}

void I2CParallel9534::setInputPolarity(const uint8_t polarity) {
  _polarityState = polarity;
  if (_i2cAddr == UNINITIALIZED_I2C_ADDR) {
    _error = I2C_PARALLEL_ERR_UNINITIALIZED;
    return;
  }
  Wire.beginTransmission(_i2cAddr);
  Wire.write(REG_POLARITY);
  Wire.write(_polarityState);
  Wire.endTransmission(SEND_STOP);
}
