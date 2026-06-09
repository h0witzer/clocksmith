// clocksmith stepper viability tester
//
// Drives one TMC2209 V1.3 socket on a RAMPS 1.4 / Arduino Mega and reports
// achieved step rate plus StallGuard load so a motor can be judged for a given
// microstep / current / chopper configuration. Speaks a line-based ASCII
// protocol over USB; see docs/protocol.md.

#include "Config.h"
#include "LineReader.h"
#include "StepGen.h"
#include "TmcControl.h"

static uint8_t g_axis = 0;
static uint16_t g_fullStepsPerRev = 200;   // 1.8 deg motor default
static bool g_stream = false;
static uint32_t g_lastTelemetryMs = 0;
static uint32_t g_runStartMs = 0;
static bool g_running = false;

// DIAG StallGuard flag, set by the hardware interrupt and cleared on read.
static volatile bool g_diagStall = false;

static void onDiag() { g_diagStall = true; }

static void selectAxis(uint8_t idx) {
  if (idx >= AXIS_COUNT) return;
  g_axis = idx;
  // De-energise the previous socket and bind the generator to the new one.
  for (uint8_t i = 0; i < AXIS_COUNT; i++) {
    pinMode(AXIS[i].en, OUTPUT);
    digitalWrite(AXIS[i].en, HIGH);   // EN active LOW: HIGH disables
  }
  StepGen::setPins(AXIS[idx].step, AXIS[idx].dir);
}

static void reply(const __FlashStringHelper* s) { HOST_SERIAL.println(s); }
static void replyErr(const char* why) {
  HOST_SERIAL.print(F("ERR "));
  HOST_SERIAL.println(why);
}

// Find "key=" within a token list and return the numeric value after it.
static bool findLong(const char* s, const char* key, long& out) {
  const char* p = strstr(s, key);
  if (!p) return false;
  p += strlen(key);
  out = atol(p);
  return true;
}

static void sendTelemetry() {
  bool diag;
  noInterrupts();
  diag = g_diagStall;
  g_diagStall = false;
  interrupts();

  uint16_t sg = Tmc::stallGuardResult();
  float actual = StepGen::actualRate();
  float target = StepGen::targetRate();
  float mspr = (float)g_fullStepsPerRev * (Tmc::microsteps() ? Tmc::microsteps() : 1);
  float rpm = mspr > 0 ? (actual / mspr) * 60.0f : 0.0f;

  HOST_SERIAL.print(F("DATA t="));
  HOST_SERIAL.print(millis());
  HOST_SERIAL.print(F(" target=")); HOST_SERIAL.print(target, 0);
  HOST_SERIAL.print(F(" actual=")); HOST_SERIAL.print(actual, 0);
  HOST_SERIAL.print(F(" rpm=")); HOST_SERIAL.print(rpm, 1);
  HOST_SERIAL.print(F(" sg=")); HOST_SERIAL.print(sg);
  HOST_SERIAL.print(F(" diag=")); HOST_SERIAL.print(diag ? 1 : 0);
  HOST_SERIAL.print(F(" steps=")); HOST_SERIAL.print(StepGen::stepCount());
  HOST_SERIAL.print(F(" irun=")); HOST_SERIAL.print(Tmc::runCurrentMa());
  HOST_SERIAL.print(F(" otpw=")); HOST_SERIAL.print(Tmc::overTemperatureWarn() ? 1 : 0);
  HOST_SERIAL.print(F(" ot=")); HOST_SERIAL.print(Tmc::overTemperatureShut() ? 1 : 0);
  HOST_SERIAL.print(F(" conn=")); HOST_SERIAL.print(Tmc::connected() ? 1 : 0);
  HOST_SERIAL.println();
}

static void handle(const char* line) {
  long v;

  if (line[0] == '\0') return;

  if (strcmp(line, "PING") == 0) {
    HOST_SERIAL.println(F("OK clocksmith-tester"));
    return;
  }
  if (strncmp(line, "AXIS=", 5) == 0) {
    const char* a = line + 5;
    for (uint8_t i = 0; i < AXIS_COUNT; i++) {
      if (strcmp(a, AXIS[i].name) == 0) { selectAxis(i); reply(F("OK")); return; }
    }
    long idx = atol(a);
    if (idx >= 0 && idx < AXIS_COUNT) { selectAxis((uint8_t)idx); reply(F("OK")); return; }
    replyErr("axis"); return;
  }
  if (strncmp(line, "MS=", 3) == 0) {
    Tmc::setMicrosteps((uint16_t)atol(line + 3)); reply(F("OK")); return;
  }
  if (strncmp(line, "IRUN=", 5) == 0) {
    Tmc::setRunCurrent((uint16_t)atol(line + 5)); reply(F("OK")); return;
  }
  if (strncmp(line, "IHOLD=", 6) == 0) {
    Tmc::setHoldCurrent((uint16_t)atol(line + 6)); reply(F("OK")); return;
  }
  if (strncmp(line, "MODE=", 5) == 0) {
    const char* m = line + 5;
    if (strcmp(m, "stealth") == 0) { Tmc::setStealthChop(true); reply(F("OK")); return; }
    if (strcmp(m, "spread") == 0) { Tmc::setStealthChop(false); reply(F("OK")); return; }
    replyErr("mode"); return;
  }
  if (strncmp(line, "TPWM=", 5) == 0) {
    Tmc::setTpwmThrs((uint32_t)atol(line + 5)); reply(F("OK")); return;
  }
  if (strncmp(line, "SGT=", 4) == 0) {
    Tmc::setStallThreshold((uint8_t)atol(line + 4)); reply(F("OK")); return;
  }
  if (strncmp(line, "TCOOL=", 6) == 0) {
    Tmc::setCoolThreshold((uint32_t)atol(line + 6)); reply(F("OK")); return;
  }
  if (strncmp(line, "STEPSPERREV=", 12) == 0) {
    long s = atol(line + 12);
    if (s > 0) { g_fullStepsPerRev = (uint16_t)s; reply(F("OK")); return; }
    replyErr("steps"); return;
  }
  if (strncmp(line, "ACCEL=", 6) == 0) {
    StepGen::setAccel((float)atol(line + 6)); reply(F("OK")); return;
  }
  if (strncmp(line, "EN=", 3) == 0) {
    bool on = atol(line + 3) != 0;
    Tmc::enable(on);
    digitalWrite(AXIS[g_axis].en, on ? LOW : HIGH);
    reply(F("OK")); return;
  }
  if (strncmp(line, "RUN", 3) == 0) {
    long sps = 0;
    if (!findLong(line, "sps=", sps)) { replyErr("sps"); return; }
    if (Tmc::overTemperatureShut()) { replyErr("overtemp"); return; }
    long steps = 0, secs = 0;
    uint32_t limit = 0;
    if (findLong(line, "steps=", steps) && steps > 0) limit = (uint32_t)steps;
    Tmc::enable(true);
    digitalWrite(AXIS[g_axis].en, LOW);
    StepGen::run((float)sps, limit);
    g_running = true;
    g_runStartMs = millis();
    reply(F("OK")); return;
  }
  if (strcmp(line, "STOP") == 0) {
    StepGen::stop(); reply(F("OK")); return;
  }
  if (strcmp(line, "ESTOP") == 0) {
    StepGen::emergencyStop();
    Tmc::enable(false);
    digitalWrite(AXIS[g_axis].en, HIGH);
    g_running = false;
    reply(F("OK")); return;
  }
  if (strncmp(line, "STREAM=", 7) == 0) {
    g_stream = atol(line + 7) != 0; reply(F("OK")); return;
  }
  if (strcmp(line, "QUERY") == 0) {
    sendTelemetry(); return;
  }
  replyErr("unknown");
}

void setup() {
  HOST_SERIAL.begin(HOST_BAUD);
  selectAxis(0);
  StepGen::begin(AXIS[0].step, AXIS[0].dir);
  StepGen::setAccel(20000.0f);

  if (DIAG_PIN >= 0) {
    pinMode(DIAG_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(DIAG_PIN), onDiag, RISING);
  }

  bool ok = Tmc::begin();
  delay(50);
  HOST_SERIAL.print(F("READY clocksmith-tester axis="));
  HOST_SERIAL.print(AXIS[0].name);
  HOST_SERIAL.print(F(" tmc="));
  HOST_SERIAL.println(ok ? F("ok") : F("nc"));
}

void loop() {
  if (LineReader::poll()) {
    handle(LineReader::line());
  }
  StepGen::service();

  // Safety: bound continuous run time so held current cannot cook a motor.
  if (g_running && (millis() - g_runStartMs) > (MAX_RUN_SECONDS * 1000UL)) {
    StepGen::stop();
    g_running = false;
    reply(F("ERR runtime"));
  }
  // Safety: shut down on driver over-temperature.
  if (Tmc::overTemperatureShut()) {
    StepGen::emergencyStop();
    Tmc::enable(false);
    digitalWrite(AXIS[g_axis].en, HIGH);
    g_running = false;
  }
  if (!StepGen::isRunning()) g_running = false;

  uint32_t now = millis();
  if (g_stream && (now - g_lastTelemetryMs) >= TELEMETRY_PERIOD_MS) {
    g_lastTelemetryMs = now;
    sendTelemetry();
  }
}
