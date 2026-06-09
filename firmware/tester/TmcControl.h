#pragma once
#include <Arduino.h>

// Thin wrapper over TMCStepper for the one driver under test. Configures
// microstepping, run/hold current, the stealthChop2 <-> spreadCycle switch and
// StallGuard, and reads back load/diagnostics for telemetry.
namespace Tmc {

// Initialise the UART link and apply safe defaults. Returns false if the
// driver does not answer (check wiring / address pads).
bool begin();

// True if the last register read communicated successfully.
bool connected();

// Microsteps per full step: 0 (fullstep) or 2,4,8,16,32,64,128,256.
void setMicrosteps(uint16_t ms);
uint16_t microsteps();

// Run and hold current in milliamps RMS, clamped to the safety ceiling.
void setRunCurrent(uint16_t mA);
void setHoldCurrent(uint16_t mA);
uint16_t runCurrentMa();

// Select stealthChop2 (true) or spreadCycle (false) at low speed.
void setStealthChop(bool on);
bool stealthChop();

// Velocity threshold (TPWMTHRS, in TSTEP units) above which the driver
// auto-switches stealthChop -> spreadCycle. 0 keeps stealthChop at all speeds.
void setTpwmThrs(uint32_t tstep);

// StallGuard sensitivity (SGTHRS, 0-255) and lower velocity gate (TCOOLTHRS).
void setStallThreshold(uint8_t sgthrs);
void setCoolThreshold(uint32_t tstep);

// Energise / de-energise the coils via the driver enable line.
void enable(bool on);

// Live reads for telemetry.
uint16_t stallGuardResult();   // SG_RESULT, drops toward 0 under load
bool overTemperatureWarn();    // otpw flag
bool overTemperatureShut();    // ot flag
bool driverError();            // any DRV_STATUS error bit

}  // namespace Tmc
