// (C) Copyright 2021-2026 Aaron Kimball
// This library is licensed under the terms of the BSD 3-Clause license.
// See the accompanying LICENSE.txt file for full license text.
//
// PCA9538 / TCA6408A Implementation
// See datasheet: https://www.ti.com/lit/ds/symlink/pca9538.pdf
//
// To use, include I2CParallel2.h, and instantiate an I2CParallel9538 object.

#include <Arduino.h>
#include <cstdint>

#include "I2CParallel2.h"

void I2CParallel9538::init(const uint8_t i2cAddr, const uint32_t busSpeed) {
  I2CParallel9534::init(i2cAddr, busSpeed);

  if (_resetPin != INVALID_GPIO_PIN) {
    pinMode(_resetPin, OUTPUT);
    digitalWrite(_resetPin, HIGH);
  }
}

void I2CParallel9538::reset() {
  if (_resetPin == INVALID_GPIO_PIN) {
    _error = I2C_PARALLEL_ERR_INVALID_PIN;
    return;
  }
  digitalWrite(_resetPin, LOW);
  delayMicroseconds(1);
  digitalWrite(_resetPin, HIGH);
}
