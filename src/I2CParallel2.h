// (c) Copyright 2021-2026 Aaron Kimball
// This library is licensed under the terms of the BSD 3-Clause license.
// See the accompanying LICENSE.txt file for full license text.
//
// Library to use  various I2C parallel bus expander ICs: PCF8574, PCA9534, etc.
// See the README.md file for a complete list of supported devices.
//
// This depends on the <Wire> library for I2C communication.

#ifndef I2C_PARALLEL2_H
#define I2C_PARALLEL2_H

#include <Arduino.h>

// An "i2c address" that no PCF8574[A] can have. We use this to note that
// the device driver has not been initialized, and disable I/O until this
// condition is lifted.
static constexpr uint8_t UNINITIALIZED_I2C_ADDR = 0;

// PCF8574 and '8574A are identical, except for their configurable address
// ranges: PCF8574  address bits: 0 1 0 0 A2 A1 A0 0 PCF8574A address bits: 0 1
// 1 1 A2 A1 A0 0

// The PCF8574 uses addresses 0x20 .. 0x27
static constexpr uint8_t I2C_PCF8574_MIN_ADDR = 0x20;
static constexpr uint8_t I2C_PCF8574_MAX_ADDR = 0x27;

// The PCF8574A uses addresses 0x38 .. 0x3F
static constexpr uint8_t I2C_PCF8574A_MIN_ADDR = 0x38;
static constexpr uint8_t I2C_PCF8574A_MAX_ADDR = 0x3F;

// Valid I2C address ranges (7-bit): PCA9534/PCA9554 0100xxx, PCA9534A/PCA9554A
// 0111xxx, PCA9538 11100xx, TCA6408 010000x (subset of 0100xxx).
static constexpr uint8_t I2C_PCA9534_MIN_ADDR = 0x20; // 0100xxx
static constexpr uint8_t I2C_PCA9534_MAX_ADDR = 0x27;
static constexpr uint8_t I2C_PCA9534A_MIN_ADDR = 0x38; // 0111xxx
static constexpr uint8_t I2C_PCA9534A_MAX_ADDR = 0x3F;
static constexpr uint8_t I2C_PCA9538_MIN_ADDR = 0x70; // 11100xx
static constexpr uint8_t I2C_PCA9538_MAX_ADDR = 0x73;

static constexpr uint32_t I2C_SPEED_FAST = 400000L;
static constexpr uint32_t I2C_SPEED_STANDARD = 100000L;
// The PCF8574 standard is specified at 100kHz but TI-manufactured chips are
// rated for 400kHz "fast mode" I2C. (See p.12 of
// https://www.ti.com/lit/an/scpa032/scpa032.pdf) The pin-compatible NXP-mfr'd
// PCA8574 is also 400kHz device. All other devices in this library are 400kHz
// devices.
static constexpr uint32_t I2C_PARALLEL_MAX_BUS_SPEED = I2C_SPEED_FAST;

// This is an 8-bit unsigned int output device.
static constexpr uint8_t I2C_PARALLEL_MAX_VAL = 0xFF;

static constexpr uint8_t I2C_PARALLEL_STARTUP_INPUT_STATE = 0xFF;

// bitmasks can refer to bits 0--7 in the output byte.
static constexpr uint8_t I2C_MAX_BIT_POS = 7;

/** A value for `resetPin` that indicates it is not connected to a GPIO pin. */
static constexpr uint8_t INVALID_GPIO_PIN = 255;

// Error codes that can be returned by I2CParallel::getError():

static constexpr uint8_t I2C_PARALLEL_ERR_OK = 0;        // No error.
static constexpr uint8_t I2C_PARALLEL_ERR_ADDR = 1;      // Invalid I2C address.
static constexpr uint8_t I2C_PARALLEL_ERR_BUS_SPEED = 2; // Invalid bus speed.
static constexpr uint8_t I2C_PARALLEL_ERR_UNINITIALIZED = 3; // Uninitialized.
static constexpr uint8_t I2C_PARALLEL_ERR_BUS_IO =
    4; // Error reading or writing over I2C bus.
static constexpr uint8_t I2C_PARALLEL_ERR_CARRY =
    5; // Arithmetic carry overflow; the increment() operation rolled back to 0.
static constexpr uint8_t I2C_PARALLEL_ERR_INVALID_PIN =
    6; // Invalid GPIO pin used for the last operation.

/**
 * Base class for all I2C parallel bus expander devices.
 *
 * This defines a common interface for all I2C parallel bus expander devices.
 * It is not intended to be instantiated directly. Instead, use a concrete
 * implementation class such as I2CParallel8574 or I2CParallel9534.
 */
class I2CParallel {
public:
  I2CParallel()
      : _i2cAddr(UNINITIALIZED_I2C_ADDR), _error(I2C_PARALLEL_ERR_OK),
        _outputState(I2C_PARALLEL_STARTUP_INPUT_STATE),
        _inputState(I2C_PARALLEL_STARTUP_INPUT_STATE){};
  ~I2CParallel(){};

  // Configure the 8-bit parallel bus with its expected 7-bit I2C address.
  // This must be within the supported MIN_ADDR .. MAX_ADDR range for the
  // device.
  virtual void init(const uint8_t i2cAddr,
                    const uint32_t busSpeed = I2C_PARALLEL_MAX_BUS_SPEED) = 0;

  // Configure the specified pin as the recipient of the INT_L signal from
  // the I2C parallel bus. The specified isr method will be called when INT_L
  // is pulled low by the device (i.e., when an edge is detected on an input
  // pin). Add a pull-up between this pin and Vcc.
  void initInterrupt(const uint8_t digitalPinNum, void (*isr)());

  // Set the value to emit on the 8-bit bus. This value is latched and held
  // until overwritten. Implementations may mix this state with input state
  // based on quasi-bidirectional I/O, or may mask part of this output to
  // allow other pins to be driven by external logic.
  // Returns the number of bytes written (1 on success, 0 on failure).
  virtual size_t setByte(const uint8_t val) = 0;
  // Synonym for setByte().
  size_t write(const uint8_t val) { return setByte(val); };

  // Read back the current contents of the 8-bit bus.
  // This returns the value received and sets nBytesRead to 1 or 0 depending on
  // whether or not it has successfully performed the I/O read. If bytesRead is
  // 0, the output should not be used. (The error flag will also be set on
  // error.)
  virtual uint8_t getByte(uint8_t &nBytesRead) = 0;
  uint8_t getByte() {
    uint8_t numReceived = 0;
    return getByte(numReceived);
  };
  uint8_t read() { return getByte(); }; // synonym for getByte().

  // Read back the last known contents of the bus without actually reading over
  // i2c.
  uint8_t getLastInputState() const { return _inputState; };

  // Read back the last known contents of the output register without reading
  // from the device.
  uint8_t getLastOutputState() const { return _outputState; };

  // Configure some data lines as inputs according to the specified mask.  The
  // masked data lines will be allowed to pull up to logic HIGH and can then be
  // driven by the connected device(s). Subsequently writing a logic LOW to any
  // bits with setByte() will drive those lines low and disable input mode.
  virtual void enableInputs(const uint8_t mask) = 0;

  // Apply a bitwise OR operation to the current bus state.
  size_t setOr(const uint8_t val) { return setByte(_outputState | val); };
  // Apply a bitwise AND operation to the current bus state.
  size_t setAnd(const uint8_t val) { return setByte(_outputState & val); };
  // Apply a bitwise XOR operation to the current bus state.
  size_t setXor(const uint8_t val) { return setByte(_outputState ^ val); };

  /** Set the specified bit (0--7) high. */
  size_t setBit(const uint8_t bitPos) {
    if (bitPos > I2C_MAX_BIT_POS) {
      return 0; /* Nothing to do. */
    }
    uint8_t mask = 1 << bitPos;
    return setOr(mask);
  }

  /** Set the specified bit (0--7) low. */
  size_t clrBit(const uint8_t bitPos) {
    if (bitPos > I2C_MAX_BIT_POS) {
      return 0; /* Nothing to do. */
    }
    uint8_t mask = ~(1 << bitPos);
    return setAnd(mask);
  }

  /** Switch the state of the specified bit (0--7). */
  size_t toggleBit(const uint8_t bitPos) {
    if (bitPos > I2C_MAX_BIT_POS) {
      return 0; /* Nothing to do. */
    }
    uint8_t setMask = 1 << bitPos;
    if ((_outputState & setMask) != 0) {
      // Bit already set. Clear it.
      return clrBit(bitPos);
    } else {
      return setBit(bitPos);
    }
  }

  // Increment the bus arithmetically by 1; 0xFF + 1 rolls back to 0 (and sets
  // the error flag to I2C_PARALLEL_ERR_CARRY).
  size_t increment() {
    if (_outputState == I2C_PARALLEL_MAX_VAL) {
      _error = I2C_PARALLEL_ERR_CARRY;
      return setByte(0);
    }

    return setByte(_outputState + 1);
  };

  // Delay until the transmitted data is ready on the parallel bus,
  // or delay until parallel bus inputs can be queried. This is not called
  // directly by the setByte() implementation; there may be a delay between the
  // setByte() function return and the data being available on the bus I/O pins.
  virtual void waitForValid() = 0;

  /** Clear the error code state. */
  void clearError() { _error = I2C_PARALLEL_ERR_OK; };

  /**
   * Get the last logged error code. Errors are 'sticky' and persist until
   * cleared, or until the next call to init().
   */
  uint8_t getError() const { return _error; };

  /** Check if the last operation had an error. */
  bool hasError() const { return _error != I2C_PARALLEL_ERR_OK; };

  /** Return the I2C address of the device. */
  uint8_t getAddress() const {
    if (_i2cAddr == UNINITIALIZED_I2C_ADDR) {
      _error = I2C_PARALLEL_ERR_UNINITIALIZED;
    }
    return _i2cAddr;
  };

protected:
  // State of the 8 output data lines.
  uint8_t _outputState;

  // State of the 8 input data lines.
  uint8_t _inputState;

  uint8_t _i2cAddr;       // Address of chip on the I2C bus.
  mutable uint8_t _error; // Error code from last operation.
};

/**
 * Implementation of I2CParallel for the PCF8574 and PCF8574A devices.
 * See datasheet: https://www.ti.com/lit/ds/symlink/pcf8574.pdf
 */
class I2CParallel8574 : public I2CParallel {
public:
  I2CParallel8574() : I2CParallel(){};
  ~I2CParallel8574(){};

  virtual void
  init(const uint8_t i2cAddr,
       const uint32_t busSpeed = I2C_PARALLEL_MAX_BUS_SPEED) override final;

  virtual size_t setByte(const uint8_t val) override final;

  virtual uint8_t getByte(uint8_t &nBytesRead) override final;

  virtual void enableInputs(const uint8_t mask) override final;

  virtual void waitForValid() override final;
};

/**
 * Implementation of I2CParallel for the '9534 and '9554 device families.
 * See datasheet: https://www.ti.com/lit/ds/symlink/pca9534.pdf
 */
class I2CParallel9534 : public I2CParallel {
public:
  I2CParallel9534() : I2CParallel(), _polarityState(0){};
  ~I2CParallel9534(){};

  virtual void
  init(const uint8_t i2cAddr,
       const uint32_t busSpeed = I2C_PARALLEL_MAX_BUS_SPEED) override;

  virtual size_t setByte(const uint8_t val) override final;

  virtual uint8_t getByte(uint8_t &nBytesRead) override final;

  virtual void enableInputs(const uint8_t mask) override final;

  // 350ns hold.
  virtual void waitForValid() override final;

  // Set the polarity of the input register.
  void setInputPolarity(const uint8_t polarity);

protected:
  uint8_t _polarityState;
};

typedef class I2CParallel9534 I2CParallel9554;

/**
 * Implementation of I2CParallel for the '6408A and '9538 device families.
 * See datasheet: https://www.ti.com/lit/ds/symlink/pca9538.pdf
 */
class I2CParallel9538 : public I2CParallel9534 {
public:
  I2CParallel9538(const uint8_t resetPin)
      : I2CParallel9534(), _resetPin(resetPin){};
  ~I2CParallel9538(){};

  virtual void
  init(const uint8_t i2cAddr,
       const uint32_t busSpeed = I2C_PARALLEL_MAX_BUS_SPEED) override final;

  // Asserts the RESET_L pin low to reset the device.
  void reset();

private:
  const uint8_t _resetPin;
};

typedef class I2CParallel9538 I2CParallel6408A;

#endif /* I2C_PARALLEL_H */
