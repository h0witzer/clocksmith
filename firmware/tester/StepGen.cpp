#include "StepGen.h"

namespace StepGen {

static volatile uint8_t* s_stepPort = nullptr;
static uint8_t s_stepMask = 0;
static uint8_t s_dirPin = 0;

static volatile uint32_t s_isrSteps = 0;   // steps emitted, updated in ISR
static volatile bool s_active = false;     // ISR currently pulsing
static volatile uint32_t s_limit = 0;      // stop after this many steps (0 = none)

static float s_accel = 20000.0f;           // steps/s^2
static float s_target = 0.0f;              // signed commanded rate
static float s_current = 0.0f;             // signed ramped rate
static uint32_t s_runBaseSteps = 0;        // step count at start of run()

static float s_actual = 0.0f;              // measured steps/s
static uint32_t s_lastServiceUs = 0;
static uint32_t s_lastServiceSteps = 0;

// Convert an unsigned rate (steps/s) into a Timer1 compare value, choosing a
// prescaler that keeps the count in range. Returns false if the rate is too
// low or zero to schedule.
static bool computeTimer(float rate, uint16_t& prescalerBits, uint16_t& compare) {
  if (rate < 1.0f) return false;
  // Available prescalers: 1, 8, 64, 256, 1024.
  const struct { uint16_t div; uint16_t bits; } presc[] = {
    { 1,    _BV(CS10) },
    { 8,    _BV(CS11) },
    { 64,   _BV(CS11) | _BV(CS10) },
    { 256,  _BV(CS12) },
    { 1024, _BV(CS12) | _BV(CS10) },
  };
  for (uint8_t i = 0; i < 5; i++) {
    float ticks = (16000000.0f / presc[i].div) / rate;
    if (ticks <= 65535.0f) {
      if (ticks < 1.0f) ticks = 1.0f;
      prescalerBits = presc[i].bits;
      compare = (uint16_t)(ticks + 0.5f) - 1;
      return true;
    }
  }
  return false;  // rate too low for any prescaler
}

static void applyRate(float signedRate) {
  float mag = fabs(signedRate);
  noInterrupts();
  if (mag < 1.0f) {
    TIMSK1 &= ~_BV(OCIE1A);   // disable compare interrupt
    s_active = false;
    interrupts();
    return;
  }
  digitalWrite(s_dirPin, signedRate >= 0 ? HIGH : LOW);
  uint16_t pbits, compare;
  if (computeTimer(mag, pbits, compare)) {
    TCCR1B = _BV(WGM12) | pbits;  // CTC mode, prescaler
    OCR1A = compare;
    if (TCNT1 > compare) TCNT1 = 0;
    TIMSK1 |= _BV(OCIE1A);
    s_active = true;
  }
  interrupts();
}

void begin(uint8_t stepPin, uint8_t dirPin) {
  setPins(stepPin, dirPin);
  noInterrupts();
  TCCR1A = 0;
  TCCR1B = 0;
  TIMSK1 = 0;
  interrupts();
  s_lastServiceUs = micros();
}

void setPins(uint8_t stepPin, uint8_t dirPin) {
  emergencyStop();
  s_dirPin = dirPin;
  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);
  digitalWrite(stepPin, LOW);
  s_stepPort = portOutputRegister(digitalPinToPort(stepPin));
  s_stepMask = digitalPinToBitMask(stepPin);
}

void setAccel(float stepsPerSec2) {
  if (stepsPerSec2 > 0.0f) s_accel = stepsPerSec2;
}

void run(float targetStepsPerSec, uint32_t limitSteps) {
  noInterrupts();
  s_runBaseSteps = s_isrSteps;
  s_limit = limitSteps;
  interrupts();
  s_target = targetStepsPerSec;
  s_lastServiceUs = micros();
  s_lastServiceSteps = s_isrSteps;
}

void stop() {
  s_target = 0.0f;
}

void emergencyStop() {
  noInterrupts();
  TIMSK1 &= ~_BV(OCIE1A);
  s_active = false;
  s_target = 0.0f;
  s_current = 0.0f;
  s_actual = 0.0f;
  interrupts();
}

bool isRunning() {
  return s_active || fabs(s_current) >= 1.0f || fabs(s_target) >= 1.0f;
}

float targetRate() { return s_target; }
float actualRate() { return s_actual; }

uint32_t stepCount() {
  noInterrupts();
  uint32_t base = s_runBaseSteps;
  uint32_t now = s_isrSteps;
  interrupts();
  return now - base;
}

void service() {
  uint32_t nowUs = micros();
  float dt = (nowUs - s_lastServiceUs) * 1e-6f;
  if (dt < 0.002f) return;  // service at most ~500 Hz
  s_lastServiceUs = nowUs;

  // Auto-stop when a step limit is reached.
  if (s_limit) {
    noInterrupts();
    uint32_t done = s_isrSteps - s_runBaseSteps;
    interrupts();
    if (done >= s_limit) {
      s_target = 0.0f;
    }
  }

  // Ramp the current rate toward the target by the configured acceleration.
  float maxDelta = s_accel * dt;
  if (s_current < s_target) {
    s_current += maxDelta;
    if (s_current > s_target) s_current = s_target;
  } else if (s_current > s_target) {
    s_current -= maxDelta;
    if (s_current < s_target) s_current = s_target;
  }
  applyRate(s_current);

  // Measure achieved rate from emitted pulses.
  noInterrupts();
  uint32_t steps = s_isrSteps;
  interrupts();
  uint32_t deltaSteps = steps - s_lastServiceSteps;
  s_lastServiceSteps = steps;
  float measured = deltaSteps / dt;
  s_actual = s_current >= 0 ? measured : -measured;
}

}  // namespace StepGen

// Compare-match ISR: emit one microstep pulse and count it.
ISR(TIMER1_COMPA_vect) {
  if (StepGen::s_stepPort) {
    *StepGen::s_stepPort |= StepGen::s_stepMask;
    // ~2 us high pulse; comfortably above the TMC2209 minimum.
    __asm__ __volatile__(
      "nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n"
      "nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n"
      "nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n"
      "nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n");
    *StepGen::s_stepPort &= ~StepGen::s_stepMask;
    StepGen::s_isrSteps++;
  }
}
