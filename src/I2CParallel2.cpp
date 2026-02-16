// (c) Copyright 2026 Aaron Kimball
// This library is licensed under the terms of the BSD 3-Clause license.
// See the accompanying LICENSE.txt file for full license text.
//
// Library to use  various I2C parallel bus expander ICs: PCF8574, PCA9534, etc.
// See the README.md file for a complete list of supported devices.
//
// This depends on the <Wire> library for I2C communication.

#include "I2CParallel2.h"
#include <Arduino.h>

void I2CParallel::initInterrupt(const uint8_t digitalPinNum, void (*isr)()) {
  pinMode(digitalPinNum, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(digitalPinNum), isr, FALLING);
}
