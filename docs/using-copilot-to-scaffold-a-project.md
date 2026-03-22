# Using GitHub Copilot to Scaffold a Sculpture Project

This guide answers two related questions:

1. **How do I use Copilot to create a new sculpture project repository?**
2. **When I need to change both my sculpture project AND the clocksmith framework at the same time, how do I give Copilot context over both repositories simultaneously?**

> **New to VS Code and Copilot?** If you have only used Copilot through the web interface, start with the [`learning/`](../learning/README.md) folder first. It covers VS Code setup, how Copilot Chat works, and the GitHub CLI — all of which are used throughout this guide.

---

## Part 1 — Creating the Sculpture Repository

### Step 1 — Create a new repository on GitHub

Use the GitHub CLI in your terminal:

```sh
gh repo create my-wall-clock --public --description "Kinetic wall clock using clocksmith" --clone
cd my-wall-clock
```

Or create it through the GitHub website and clone it:

```sh
git clone https://github.com/YOUR_USERNAME/my-wall-clock.git
cd my-wall-clock
```

### Step 2 — Scaffold the PlatformIO project

```sh
pio project init --board uno --ide vscode
```

This writes `platformio.ini` and creates an empty `src/main.cpp`.

### Step 3 — Declare clocksmith as a dependency

Open `platformio.ini` and add `lib_deps`:

```ini
[env:arduino_uno]
platform    = atmelavr
board       = uno
framework   = arduino
lib_deps    =
    https://github.com/h0witzer/clocksmith.git#v1.0.0
```

### Step 4 — Add `.gitignore`

```sh
cat > .gitignore << 'EOF'
.pio/
.vscode/
*.code-workspace
.DS_Store
*.swp
*.swo
*~
EOF
```

### Step 5 — Create the `.github/copilot-instructions.md` for your project

This file teaches Copilot about your specific sculpture — which board, which motors, which display. Create `.github/copilot-instructions.md` in your sculpture project with content like this:

```markdown
# Copilot Instructions — my-wall-clock

## What this project is
A kinetic wall clock using clocksmith (https://github.com/h0witzer/clocksmith).
clocksmith is declared as a lib_deps dependency; its headers are available on the
include path after `pio pkg install`.

## Hardware
- Board: Arduino Uno
- Hour hand: 28BYJ-48 stepper + ULN2003 board, pins 8-11, 2048 steps/rev
- Minute hand: 28BYJ-48 stepper + ULN2003 board, pins 4-7, 2048 steps/rev
- Time source: DS3231 RTC over I2C (SDA=A4, SCL=A5)
- Display: 60-LED NeoPixel ring, data pin 6

## File layout
lib/drivers/ULN2003StepperMotor.hpp  — implements IMotor via AccelStepper
lib/drivers/DS3231ClockCore.hpp      — implements IClockCore via RTClib
lib/drivers/NeoPixelRing.hpp         — implements IDisplay via Adafruit NeoPixel
src/main.cpp                         — wires all drivers into ClockLogic

## Key rules (inherited from clocksmith)
- No delay() anywhere
- Normalised motor positions [0.0, 1.0]
- Static driver objects in setup(), never heap-allocated
- main.cpp is the only file that #includes concrete hardware headers
```

Tailor the hardware section to your actual components every time you start a new sculpture project.

### Step 6 — Install the dependency and open in VS Code

```sh
pio pkg install
code .
```

---

## Part 2 — Working Across Both Repos Simultaneously

When a new sculpture **requires a change to the clocksmith framework** (for example, adding a new interface method or a new mechanism handler), you need Copilot to see **both** repositories at the same time.

The tool for this is a **VS Code multi-root workspace**.

### What is a multi-root workspace?

A VS Code multi-root workspace is a single VS Code window that has two (or more) folders open simultaneously. Copilot's `@workspace` context covers **all folders** in the workspace — so it can read your sculpture project's drivers and clocksmith's interfaces in the same conversation.

### Creating the workspace file

Create a file called `clocksmith-dev.code-workspace` **outside** both repositories (e.g. in the parent folder that contains them both):

```
~/projects/
├── clocksmith/           ← the framework repo
├── my-wall-clock/        ← your sculpture project repo
└── clocksmith-dev.code-workspace   ← lives here, not inside either repo
```

The file contents:

```json
{
    "folders": [
        {
            "name": "clocksmith (framework)",
            "path": "./clocksmith"
        },
        {
            "name": "my-wall-clock (sculpture)",
            "path": "./my-wall-clock"
        }
    ],
    "settings": {
        "editor.formatOnSave": true,
        "C_Cpp.default.includePath": [
            "${workspaceFolder:clocksmith (framework)}/include",
            "${workspaceFolder:clocksmith (framework)}/lib/ClockLogic",
            "${workspaceFolder:clocksmith (framework)}/lib/mechanisms",
            "${workspaceFolder:clocksmith (framework)}/lib/curves"
        ]
    }
}
```

Open it with:

```sh
code ~/projects/clocksmith-dev.code-workspace
```

> **Why outside both repos?**
> Both `.gitignore` files exclude `*.code-workspace` to prevent machine-specific paths from being committed. Placing the workspace file in the parent folder keeps it off both repos' radar entirely.

### What Copilot sees in a multi-root workspace

Once the workspace is open, Copilot's `@workspace` agent indexes **all files across all folders**. This means:

- It knows the `IMotor` contract from `clocksmith/include/IMotor.hpp`
- It knows your driver implementation from `my-wall-clock/lib/drivers/ULN2003StepperMotor.hpp`
- It knows how `main.cpp` wires them together
- It knows both `.github/copilot-instructions.md` files — the framework-level one from clocksmith and the project-level one from your sculpture

---

## Part 3 — Copilot Chat Prompts

With the multi-root workspace open in VS Code, open Copilot Chat (`Ctrl+Shift+I` / `Cmd+Shift+I`) and use these prompts. Copy and adapt them for your specific hardware.

### Create a motor driver

```
@workspace I need a new motor driver for my sculpture project.

Hardware: 28BYJ-48 stepper motor driven by a ULN2003 board.
Library: AccelStepper (waspinator/AccelStepper in lib_deps).
Pins: pin1=8, pin2=9, pin3=10, pin4=11.
Steps per revolution: 2048 (half-step mode).
Output file: my-wall-clock/lib/drivers/ULN2003StepperMotor.hpp

Follow the IMotor interface from clocksmith/include/IMotor.hpp and match the
pattern shown in clocksmith/hal/motors/StepperMotorStub.hpp. Apply all
clocksmith rules: no delay(), clamp only negative positions, static objects in
setup(), non-blocking update().
```

### Create a display driver

```
@workspace I need a display driver for a 60-LED NeoPixel ring.

Library: Adafruit NeoPixel (adafruit/Adafruit NeoPixel in lib_deps).
Pixel count: 60. Data pin: 6. Colour order: NEO_GRB + NEO_KHZ800.
Output file: my-wall-clock/lib/drivers/NeoPixelRing.hpp

Follow the IDisplay interface from clocksmith/include/IDisplay.hpp and the
pattern in clocksmith/hal/displays/NeoPixelDisplayStub.hpp. setPixel() must
bounds-check silently. update() must be non-blocking.
```

### Create a clock core (RTC)

```
@workspace I need an IClockCore implementation for a DS3231 RTC module.

Library: RTClib (adafruit/RTClib in lib_deps).
I2C pins: SDA=A4, SCL=A5 (Arduino Uno default).
Output file: my-wall-clock/lib/drivers/DS3231ClockCore.hpp

Follow the IClockCore interface from clocksmith/include/IClockCore.hpp.
update() must be non-blocking — cache the last read value and serve it if
the I2C bus is busy. Hours must be 0–23 (24-hour format).
isSynchronised() should return false if the DS3231 lost power.
```

### Wire main.cpp

```
@workspace Generate src/main.cpp for my sculpture project.

Drivers available:
- my-wall-clock/lib/drivers/ULN2003StepperMotor.hpp  (IMotor)
- my-wall-clock/lib/drivers/DS3231ClockCore.hpp       (IClockCore)
- my-wall-clock/lib/drivers/NeoPixelRing.hpp          (IDisplay)

Hour motor:   ULN2003StepperMotor, 2048 steps, pins 8-11
Minute motor: ULN2003StepperMotor, 2048 steps, pins 4-7
Ring display: NeoPixelRing, 60 pixels, pin 6
Clock core:   DS3231ClockCore

Use HardwareRegistry and ClockLogic from clocksmith. Register motors under
Slots::Motor::HOUR_HAND and Slots::Motor::MINUTE_HAND. Register the display
under Slots::Display::MAIN_RING. Declare all driver instances as static locals
in setup(). Call update() on every motor and the display in loop().
```

### Add a new slot to the clocksmith framework

This is the cross-repo case — you need to add something to the framework and immediately use it in the sculpture project.

```
@workspace I need to add a SECOND_HAND motor slot to the clocksmith framework
and then use it in my sculpture project.

Step 1: In clocksmith/include/HardwareRegistry.hpp, add Slots::Motor::SECOND_HAND
alongside the existing motor slot constants.

Step 2: In my-wall-clock/src/main.cpp, add a third ULN2003StepperMotor
for the second hand (2048 steps, pins 12-A2) and register it under
Slots::Motor::SECOND_HAND. Add its update() call in loop().

Show both files with the minimal changes needed.
```

### Explain what needs to change in both repos

When you are not sure which repo needs to change, ask Copilot to analyse first:

```
@workspace I want to add a buzzer that chimes on the hour to my wall clock.

Look at clocksmith's interfaces and ClockLogic to tell me:
1. Does the framework need a new interface (e.g. IBuzzer)?
2. Does ClockLogic need to know about the buzzer, or can I drive it directly from main.cpp?
3. What is the minimal set of changes across both repositories?
```

---

## Part 4 — Quick Reference: Which File Lives Where

| What you are creating | Where it lives | Committed to |
|---|---|---|
| New interface (e.g. `IBuzzer.hpp`) | `clocksmith/include/` | clocksmith repo |
| New mechanism handler | `clocksmith/lib/mechanisms/` | clocksmith repo |
| Motor / display driver for your sculpture | `my-sculpture/lib/drivers/` | sculpture repo |
| `main.cpp` for your sculpture | `my-sculpture/src/` | sculpture repo |
| `platformio.ini` with `lib_deps` | `my-sculpture/` | sculpture repo |
| Multi-root workspace file | parent folder (outside both repos) | neither |
| `copilot-instructions.md` | both `.github/` folders | each respective repo |

---

## Further Reading

- [using-as-a-library.md](using-as-a-library.md) — full explanation of the library dependency model
- [architecture.md](architecture.md) — the framework design and the framework/project boundary
- [adding-a-motor-driver.md](adding-a-motor-driver.md) — detailed motor driver walkthrough
- [adding-a-display-driver.md](adding-a-display-driver.md) — detailed display driver walkthrough
