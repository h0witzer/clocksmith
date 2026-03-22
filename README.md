# clocksmith 🕰️

A modular, hardware-agnostic C++ framework for building **Kinetic Clock Sculptures** on Arduino and other embedded platforms.

---

## Design Philosophy

A typical Arduino clock sketch tightly couples the time logic, the motor driver, and the display library into a single file. clocksmith separates those concerns so that every piece of hardware is independently swappable:

| Concern | Interface | Example implementations |
|---|---|---|
| *What time is it?* | `IClockCore` | DS3231 RTC, NTP (ESP32), `millis()` counter |
| *Move the hands* | `IMotor` | Stepper motor, servo, DC + encoder |
| *Show the time* | `IDisplay` | NeoPixel ring, OLED, e-ink |
| *Coordinate all three* | `ClockLogic` | Pure logic — zero hardware calls |

Swapping a stepper motor for a servo is a single-line change in `main.cpp`. Nothing else in the project needs to change.

---

## Quick Start

1. Clone or fork this repository.
2. Open the folder in PlatformIO (VS Code extension or CLI).
3. Read **[docs/architecture.md](docs/architecture.md)** to understand the design.
4. Follow **[docs/adding-a-motor-driver.md](docs/adding-a-motor-driver.md)** to write your first motor driver.
5. Follow **[docs/adding-a-display-driver.md](docs/adding-a-display-driver.md)** to connect a display.
6. Wire everything together in `src/main.cpp` and upload.

---

## Repository Structure

```
clocksmith/
├── include/                ← Abstract interfaces (IMotor, IDisplay, IClockCore)
│   └── HardwareRegistry.hpp← Named slot registry / factory
├── src/                    ← Application entry point and registry implementation
├── lib/ClockLogic/         ← Core time→position logic (hardware-free)
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
| [docs/architecture.md](docs/architecture.md) | Why we decouple hardware from logic; system overview |
| [docs/directory-structure.md](docs/directory-structure.md) | Annotated folder tree explaining each file's role |
| [docs/adding-a-motor-driver.md](docs/adding-a-motor-driver.md) | Step-by-step guide to writing a new motor driver |
| [docs/adding-a-display-driver.md](docs/adding-a-display-driver.md) | Step-by-step guide to wrapping a display library |

---

## Key Design Rules

1. **`ClockLogic` never `#include`s a hardware library.** It only calls interface methods.
2. **No `delay()` anywhere.** Every component exposes a non-blocking `update()` method.
3. **Normalised positions.** All motor positions are floats in `[0.0, 1.0]` — not steps, not degrees.
4. **`main.cpp` is the only seam.** Concrete hardware objects are created and registered here, nowhere else.