# Wiring — clocksmith tester

Arduino Mega 2560 + RAMPS 1.4 + BigTreeTech TMC2209 V1.3 (UART mode).

## Power

- 12 V into the RAMPS main power input (the stepper power terminal). USB powers
  only the Mega logic.
- Confirm each TMC2209 module's orientation against the RAMPS silkscreen (the
  `EN` pin must line up). Reversing a driver destroys it.
- Stall tests hold current continuously, so fit heatsinks and a fan.

## STEP / DIR / EN

The firmware drives one socket at a time using the standard RAMPS 1.4 map
(`firmware/tester/Config.h`):

| Socket | STEP | DIR | EN |
|--------|------|-----|----|
| X      | 54   | 55  | 38 |
| Y      | 60   | 61  | 56 |
| Z      | 46   | 48  | 62 |
| E0     | 26   | 28  | 24 |
| E1     | 36   | 34  | 30 |

`EN` is active LOW. The firmware disables every socket on start and on axis
change, enabling only the selected one.

## TMC2209 UART (the one signal RAMPS lacks)

RAMPS 1.4 has no UART trace to the driver sockets, so add one wire to the
driver's `PDN_UART` pad. The firmware uses the Mega's spare hardware UART
`Serial1` (more reliable than SoftwareSerial at 16 MHz):

- `PDN_UART` → Mega **D19 (Serial1 RX)** directly.
- `PDN_UART` → Mega **D18 (Serial1 TX)** through a **~1 kΩ** resistor.

This forms the single-wire half-duplex link TMC2209 expects.

### UART address

The driver's `MS1` / `MS2` pads set the UART node address (0–3). The firmware
default is address `0` (both pads to GND). To put several drivers on one UART
line, give each a distinct address and update `TMC_ADDRESS` per build, or wire
each to its own Mega pin.

## DIAG (hardware stall flag)

Wire the driver `DIAG` pad to Mega **D2** (external interrupt). The firmware
trips an interrupt on a StallGuard stall and reports it as `diag=1` in
telemetry. Set `DIAG_PIN` to `-1` in `Config.h` to disable.

## R_sense

The BigTreeTech TMC2209 V1.3 uses a **0.11 Ω** sense resistor, set as
`TMC_RSENSE` in `Config.h`. This is required for correct current scaling.
