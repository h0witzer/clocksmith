#include "TmcControl.h"
#include "Config.h"
#include <TMCStepper.h>

namespace Tmc {

static TMC2209Stepper driver(&TMC_SERIAL, TMC_RSENSE, TMC_ADDRESS);

static uint16_t s_microsteps = 16;
static uint16_t s_runCurrent = 800;
static bool s_stealth = true;
static bool s_connected = false;

static uint16_t clampCurrent(uint16_t mA) {
  if (mA > MAX_RMS_CURRENT_MA) return MAX_RMS_CURRENT_MA;
  return mA;
}

bool begin() {
  TMC_SERIAL.begin(TMC_BAUD);
  driver.begin();
  driver.toff(0);                 // coils off until enabled
  driver.blank_time(24);
  driver.rms_current(s_runCurrent);
  driver.microsteps(s_microsteps);
  driver.intpol(true);            // 256 interpolation for smoothness
  driver.TCOOLTHRS(0xFFFFF);      // StallGuard active across the speed range
  driver.SGTHRS(60);
  driver.en_spreadCycle(!s_stealth);
  driver.pwm_autoscale(true);     // required for stealthChop2
  driver.pwm_autograd(true);
  // Verify communication by reading the version field.
  uint8_t ver = driver.version();
  s_connected = (ver != 0 && ver != 0xFF);
  return s_connected;
}

bool connected() { return s_connected; }

void setMicrosteps(uint16_t ms) {
  s_microsteps = ms;
  driver.microsteps(ms);
}
uint16_t microsteps() { return s_microsteps; }

void setRunCurrent(uint16_t mA) {
  s_runCurrent = clampCurrent(mA);
  driver.rms_current(s_runCurrent);
}
void setHoldCurrent(uint16_t mA) {
  uint16_t c = clampCurrent(mA);
  // ihold is expressed as a fraction (0-31) of the run current scale.
  uint8_t hold = s_runCurrent ? (uint8_t)((uint32_t)c * 31 / s_runCurrent) : 0;
  if (hold > 31) hold = 31;
  driver.ihold(hold);
}
uint16_t runCurrentMa() { return s_runCurrent; }

void setStealthChop(bool on) {
  s_stealth = on;
  driver.en_spreadCycle(!on);
}
bool stealthChop() { return s_stealth; }

void setTpwmThrs(uint32_t tstep) { driver.TPWMTHRS(tstep); }
void setStallThreshold(uint8_t sgthrs) { driver.SGTHRS(sgthrs); }
void setCoolThreshold(uint32_t tstep) { driver.TCOOLTHRS(tstep); }

void enable(bool on) {
  driver.toff(on ? 4 : 0);
}

uint16_t stallGuardResult() {
  uint16_t v = driver.SG_RESULT();
  s_connected = true;
  return v;
}

bool overTemperatureWarn() { return driver.otpw(); }
bool overTemperatureShut() { return driver.ot(); }
bool driverError() {
  return driver.ot() || driver.s2ga() || driver.s2gb() ||
         driver.ola() || driver.olb();
}

}  // namespace Tmc
