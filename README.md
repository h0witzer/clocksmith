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

The host app is a small Python GUI. If you do not already have Python set up,
install **Python 3.10+** first, then open a terminal in the repository root and
run:

```sh
cd host
python3 --version
python3 -m venv .venv
. .venv/bin/activate
python3 -m pip install --upgrade pip
python3 -m pip install -r requirements.txt
python3 -m clocksmith
```

What each step does:

1. `cd host` moves into the Python app directory.
2. `python3 --version` confirms that Python is installed and available.
3. `python3 -m venv .venv` creates an isolated environment for this project.
4. `. .venv/bin/activate` activates that environment in the current shell.
5. `python3 -m pip install --upgrade pip` updates the package installer.
6. `python3 -m pip install -r requirements.txt` installs the GUI and serial
   dependencies listed in `host/requirements.txt`.
7. `python3 -m clocksmith` launches the GUI.

If `python3` is not the right command on your machine, try the same steps with
`python` instead.

On Windows PowerShell, activate the virtual environment with
`.venv\Scripts\Activate.ps1` instead of `. .venv/bin/activate`.

Once the window opens:

1. Plug the Mega in over USB.
2. Use **Refresh** if its serial port is not already listed.
3. Select the Mega port and click **Connect**.
4. Choose the axis you wired (`E0` for your first test).
5. Load or enter the motor settings, then click **Run**.

Live plots show commanded-vs-actual step rate and StallGuard load, with a large
stall/DIAG indicator; runs are logged to `results/`.

See [`docs/methodology.md`](docs/methodology.md) for how to interpret the
readings and a suggested test procedure.