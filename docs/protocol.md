# Serial protocol — clocksmith tester

Line-based ASCII over the Mega USB serial port at **115200 baud**, 8N1, no flow
control. Every command is one line terminated by `\n`. The firmware answers
commands on the same channel with `OK` / `ERR <reason>`, and streams telemetry
as `DATA ...` lines.

## Handshake

Opening the port toggles DTR and resets the Mega. After reset the firmware
prints:

```
READY clocksmith-tester axis=X tmc=ok
```

(`tmc=nc` means the driver did not answer over UART — check wiring/address.)

The host opens the port, waits for `READY`, then sends commands. `PING` returns
`OK clocksmith-tester` at any time.

## Commands

| Command | Effect | Reply |
|---------|--------|-------|
| `PING` | liveness check | `OK clocksmith-tester` |
| `AXIS=<X\|Y\|Z\|E0\|E1\|0..4>` | select RAMPS socket | `OK` |
| `MS=<n>` | microsteps: 0 (full), 2,4,8,16,32,64,128,256 | `OK` |
| `IRUN=<mA>` | run current, RMS mA (clamped) | `OK` |
| `IHOLD=<mA>` | hold current, RMS mA | `OK` |
| `MODE=<stealth\|spread>` | chopper at low speed | `OK` |
| `TPWM=<tstep>` | stealth→spread switch velocity (0 = always stealth) | `OK` |
| `SGT=<0..255>` | StallGuard sensitivity (SGTHRS) | `OK` |
| `TCOOL=<tstep>` | StallGuard lower velocity gate (TCOOLTHRS) | `OK` |
| `STEPSPERREV=<n>` | full steps/rev, for the RPM readout | `OK` |
| `ACCEL=<steps/s²>` | ramp acceleration | `OK` |
| `EN=<0\|1>` | de-energise / energise coils | `OK` |
| `RUN sps=<signed>[ steps=<n>]` | ramp to a signed step rate; optional step limit | `OK` |
| `STOP` | decelerate to a stop | `OK` |
| `ESTOP` | immediate halt + de-energise | `OK` |
| `STREAM=<0\|1>` | enable/disable periodic telemetry | `OK` |
| `QUERY` | emit one telemetry sample now | `DATA ...` |

Unknown or malformed commands return `ERR <reason>` (e.g. `ERR unknown`,
`ERR sps`, `ERR overtemp`, `ERR runtime`).

## Telemetry

Streamed every 100 ms while `STREAM=1`, and once per `QUERY`:

```
DATA t=<ms> target=<sps> actual=<sps> rpm=<rpm> sg=<SG_RESULT> diag=<0|1> \
     steps=<n> irun=<mA> otpw=<0|1> ot=<0|1> conn=<0|1>
```

- `actual` is measured from emitted pulses — when it falls short of `target`,
  the 16 MHz AVR (not the motor) is the step-rate limit.
- `sg` (SG_RESULT) drops toward 0 under load; `diag=1` is a StallGuard stall.
- `ot` / `otpw` are driver over-temperature shutdown / pre-warning.

## Safety

The firmware enforces a maximum run time (`MAX_RUN_SECONDS`) and a current
ceiling (`MAX_RMS_CURRENT_MA`), and de-energises on driver over-temperature.
`ESTOP` always halts and de-energises immediately.
