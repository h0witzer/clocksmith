# clocksmith 🕰️

A modular, hardware-agnostic C++ framework for building **Kinetic Clock Sculptures** on Arduino and other embedded platforms.

---

## Design Philosophy

A typical Arduino clock sketch tightly couples the time logic, the motor driver, and the display library into a single file. clocksmith separates those concerns so that every piece of hardware is independently swappable:

| Concern | Interface | Example implementations |
|---|---|---|
| *What time is it?* | `IClockCore` | DS3231 RTC, NTP (ESP32), `millis()` counter |
| *Move the hands* | `IMotor` | Stepper motor, servo, DC + encoder |
| *Show a digit* | `IDigitMechanism` | SingleMotorDigit, MultiRevolutionDigit, LinkedMotorDigit |
| *Group tens + ones* | `IDigitGroup` | DigitGroup (auto-splits any integer into digits) |
| *Show the time* | `IDisplay` | NeoPixel ring, OLED, e-ink |
| *Coordinate all* | `ClockLogic` | Pure logic — zero hardware calls |

Swapping a stepper motor for a servo is a single-line change in `main.cpp`. Nothing else in the project needs to change.

---

## Quick Start — Building a Sculpture Project

> **Do not fork or clone this repo to build your sculpture.** Create a separate project and add clocksmith as a library dependency. This keeps the framework code out of your project and means you automatically receive fixes and improvements by bumping a version number.
>
> See **[docs/using-as-a-library.md](docs/using-as-a-library.md)** for the full explanation.

**Step 1 — Create a new PlatformIO project for your sculpture:**

```sh
pio project init --board uno --ide vscode
```

**Step 2 — Add clocksmith to `platformio.ini`:**

```ini
[env:arduino_uno]
platform    = atmelavr
board       = uno
framework   = arduino
lib_deps    =
    https://github.com/h0witzer/clocksmith.git
```

To pin to a specific release (recommended): append `#v1.0.0` to the URL.

**Step 3 — Write your hardware drivers in your project's `lib/` folder.** Use the stubs in [hal/motors/](hal/motors/) and [hal/displays/](hal/displays/) as starting templates. Follow the step-by-step guides:
- **[docs/adding-a-motor-driver.md](docs/adding-a-motor-driver.md)**
- **[docs/adding-a-display-driver.md](docs/adding-a-display-driver.md)**

**Step 4 — Wire everything together in `src/main.cpp` and upload.**

---

## Contributing to the Framework

If you want to fix a bug, add a new interface, or improve the documentation *inside clocksmith itself*:

1. Fork this repository on GitHub.
2. Clone your fork locally and open it in PlatformIO.
3. Make your changes, then open a pull request back to `h0witzer/clocksmith`.

Developing the framework and building a sculpture project that uses it are **two different workflows**.

---

## Repository Structure

```
clocksmith/
├── include/                ← Abstract interfaces (IMotor, IDisplay, IClockCore,
│                              IDigitMechanism, IPositionCurve, IDigitGroup)
│   └── HardwareRegistry.hpp← Named slot registry / factory
├── src/                    ← Application entry point and registry implementation
├── lib/ClockLogic/         ← Core time→position logic (hardware-free)
├── lib/mechanisms/         ← Digit mechanism handlers (multi-rev, linkage, groups)
├── lib/curves/             ← Non-linear position correction curves
├── hal/                    ← Concrete driver templates (hardware-specific stubs)
│   ├── motors/
│   └── displays/
└── docs/                   ← Developer documentation
```

See **[docs/directory-structure.md](docs/directory-structure.md)** for the full annotated tree.

---

## Documentation

| Document | Description |
|---|---|
| [docs/using-as-a-library.md](docs/using-as-a-library.md) | **Start here** — how to use clocksmith as a dependency, not a fork |
| [docs/using-copilot-to-scaffold-a-project.md](docs/using-copilot-to-scaffold-a-project.md) | How to use GitHub Copilot to create a sculpture project and work across both repos simultaneously |
| [docs/architecture.md](docs/architecture.md) | Why we decouple hardware from logic; system overview |
| [docs/directory-structure.md](docs/directory-structure.md) | Annotated folder tree explaining each file's role |
| [docs/mechanisms.md](docs/mechanisms.md) | Mechanism handlers: multi-revolution, linkage, digit groups, non-linear curves |
| [docs/adding-a-motor-driver.md](docs/adding-a-motor-driver.md) | Step-by-step guide to writing a new motor driver |
| [docs/adding-a-display-driver.md](docs/adding-a-display-driver.md) | Step-by-step guide to wrapping a display library |

## Learning Resources

If you are new to VS Code, GitHub Copilot, or the GitHub CLI, the [`learning/`](learning/) folder has step-by-step guides that are intentionally separate from the framework documentation:

| Guide | What it covers |
|---|---|
| [learning/01-vscode-and-copilot-setup.md](learning/01-vscode-and-copilot-setup.md) | Install VS Code, PlatformIO, and the Copilot extensions; sign in; verify everything works |
| [learning/02-copilot-in-vscode.md](learning/02-copilot-in-vscode.md) | Inline completions, Copilot Chat, `@workspace` context, slash commands, effective prompting |
| [learning/03-github-cli.md](learning/03-github-cli.md) | Install and authenticate the `gh` CLI; create repos; open pull requests from the terminal |

---

## Key Design Rules

1. **`ClockLogic` never `#include`s a hardware library.** It only calls interface methods.
2. **No `delay()` anywhere.** Every component exposes a non-blocking `update()` method.
3. **Normalised positions.** All motor positions are floats in `[0.0, 1.0]` — not steps, not degrees.
4. **`main.cpp` is the only seam.** Concrete hardware objects are created and registered here, nowhere else.