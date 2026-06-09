#pragma once
#include <Arduino.h>

// ---------------------------------------------------------------------------
// Board: Arduino Mega 2560 + RAMPS 1.4 + BigTreeTech TMC2209 V1.3 (UART mode)
// ---------------------------------------------------------------------------

// USB serial link to the host PC (clocksmith Python app).
#define HOST_SERIAL        Serial
#define HOST_BAUD          115200UL

// TMC2209 single-wire UART runs on a spare Mega hardware UART. Tie the driver
// PDN_UART pad to Serial1 RX (D19) and to Serial1 TX (D18) through a ~1k
// resistor on the TX side so the half-duplex line shares one wire.
#define TMC_SERIAL         Serial1
#define TMC_BAUD           115200UL

// BigTreeTech TMC2209 V1.3 sense resistor.
#define TMC_RSENSE         0.11f

// UART node address selected by the MS1/MS2 address pads on the driver (0-3).
#define TMC_ADDRESS        0b00

// DIAG output of the driver under test, wired to a Mega external-interrupt pin
// for hardware StallGuard stall detection. Set to -1 to disable.
#define DIAG_PIN           2

// Number of selectable RAMPS driver sockets.
#define AXIS_COUNT         5

// RAMPS 1.4 STEP/DIR/EN pin map per socket. EN is active LOW.
struct AxisPins {
  const char* name;
  uint8_t step;
  uint8_t dir;
  uint8_t en;
};

static const AxisPins AXIS[AXIS_COUNT] = {
  { "X",  54, 55, 38 },
  { "Y",  60, 61, 56 },
  { "Z",  46, 48, 62 },
  { "E0", 26, 28, 24 },
  { "E1", 36, 34, 30 },
};

// Safety ceilings. Stall tests hold current continuously, so cap RMS current
// and total run time, and refuse motion above the driver over-temperature
// pre-warning flag.
#define MAX_RMS_CURRENT_MA 2000U
#define MAX_RUN_SECONDS    120U

// Telemetry stream period in milliseconds.
#define TELEMETRY_PERIOD_MS 100U
