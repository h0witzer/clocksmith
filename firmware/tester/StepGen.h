#pragma once
#include <Arduino.h>

// Timer1-driven STEP pulse generator with trapezoidal speed ramping. One
// compare-match interrupt emits one microstep, so the achievable rate is
// bounded by the 16 MHz AVR (~30-40 k steps/s) rather than the driver. The
// commanded and actually achieved rates are both reported so the host can see
// when the Mega, not the motor, is the limit.
namespace StepGen {

// Bind the generator to a RAMPS socket's STEP/DIR pins and prepare Timer1.
void begin(uint8_t stepPin, uint8_t dirPin);

// Re-target STEP/DIR when the host selects a different socket.
void setPins(uint8_t stepPin, uint8_t dirPin);

// Set acceleration in steps/s^2 used to ramp toward the target rate.
void setAccel(float stepsPerSec2);

// Run at a signed target rate in steps/s (sign selects direction). A non-zero
// limitSteps stops automatically after that many steps; 0 runs until stopped.
void run(float targetStepsPerSec, uint32_t limitSteps);

// Begin decelerating to a stop using the configured acceleration.
void stop();

// Immediately halt pulse output without a deceleration ramp.
void emergencyStop();

// True while the generator is producing pulses or ramping down.
bool isRunning();

// Most recently commanded target rate in steps/s (signed).
float targetRate();

// Actual achieved step rate in steps/s, measured from emitted pulses.
float actualRate();

// Total steps emitted since the last run() call.
uint32_t stepCount();

// Call frequently from loop(); advances the ramp and rate measurement.
void service();

}  // namespace StepGen
