# clocksmith

A minimal stepper-motor **viability tester** for an Arduino Mega 2560 + RAMPS
1.4 + BigTreeTech TMC2209 V1.3 drivers (UART mode). Plug a motor into a driver
socket at 12 V, configure microstepping / current / chopper from a PC GUI, and
watch achieved step rate and StallGuard load to judge whether the motor can run
at the speed and torque you need.

Built to swap 12 V pancake steppers from a drawer and try to stall them by hand
to see if they are a viable spec for fast linear-rail motion.

## Layout

```
firmware/tester/     Arduino Mega sketch (UART config + Timer1 step generator)
host/clocksmith/     Python app: serial transport, protocol, torque, logging, GUI
host/configs/        per-motor YAML profiles
docs/                wiring, serial protocol, methodology
results/             CSV logs (gitignored)
```

## Firmware

Open `firmware/tester/tester.ino` in the Arduino IDE, install the **TMCStepper**
library (Library Manager), select **Arduino Mega 2560**, and upload. Wiring —
including the one UART wire RAMPS lacks — is in [`docs/wiring.md`](docs/wiring.md).

The firmware drives one socket with a Timer1 interrupt step generator (so the
AVR step-rate ceiling is visible), configures the TMC2209 over UART
(stealthChop2 / spreadCycle, current, microstepping, StallGuard), and streams
telemetry over USB. Protocol: [`docs/protocol.md`](docs/protocol.md).

## Host app

```
cd host
python -m venv .venv && . .venv/bin/activate   # optional
pip install -r requirements.txt
python -m clocksmith
```

Select the Mega's serial port, Connect, set the motor config, and Run. Live
plots show commanded-vs-actual step rate and StallGuard load, with a large
stall/DIAG indicator; runs are logged to `results/`.

See [`docs/methodology.md`](docs/methodology.md) for how to interpret the
readings and a suggested test procedure.