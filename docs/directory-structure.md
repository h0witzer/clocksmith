# Directory Structure

Below is the complete directory tree for the clocksmith project, with an explanation of each folder's purpose.

```
clocksmith/
│
├── platformio.ini          ← PlatformIO build configuration (environments, lib paths)
├── .gitignore              ← Tells Git to ignore build output (.pio/, etc.)
├── README.md               ← Project overview and quick-start instructions
│
├── include/                ← Public interface headers (on the compiler path for all files)
│   ├── IMotor.hpp          ← Abstract interface for a single clock-hand motor
│   ├── IDisplay.hpp        ← Abstract interface for any visual display peripheral
│   ├── IClockCore.hpp      ← Abstract interface for the time-keeping subsystem
│   ├── IDigitMechanism.hpp ← Abstract interface for a single-digit display mechanism
│   ├── IPositionCurve.hpp  ← Abstract interface for non-linear position correction
│   ├── IDigitGroup.hpp     ← Abstract interface for a grouped (tens/ones) display
│   └── HardwareRegistry.hpp← Registry that maps named "slots" to implementations
│
├── src/                    ← Framework source — library implementation code only
│   ├── main.cpp            ← FRAMEWORK DEV STUB (not a consumer file — see examples/)
│   └── HardwareRegistry.cpp← Implementation of the HardwareRegistry class
│
├── lib/                    ← Local project libraries (PlatformIO auto-discovers these)
│   ├── ClockLogic/         ← Core logic library: converts time → motor/digit targets
│   │   ├── ClockLogic.hpp  ← ClockLogic class declaration
│   │   └── ClockLogic.cpp  ← ClockLogic implementation
│   │
│   ├── mechanisms/         ← Digit mechanism handlers
│   │   ├── SingleMotorDigit.hpp     ← One motor, one digit, optional curve
│   │   ├── MultiRevolutionDigit.hpp ← One motor, N revolutions per digit sweep
│   │   ├── LinkedMotorDigit.hpp     ← Two motors + ILinkageSolver (linkages)
│   │   └── DigitGroup.hpp           ← Groups tens/ones for one time unit
│   │
│   └── curves/             ← Non-linear position curve implementations
│       ├── LinearCurve.hpp          ← Identity (no correction)
│       ├── LookupTableCurve.hpp     ← Piecewise-linear calibration table
│       └── LookupTableCurve.cpp     ← Interpolation implementation
│
├── hal/                    ← Hardware Abstraction Layer: concrete driver templates
│   ├── motors/             ← One file per motor type
│   │   └── StepperMotorStub.hpp   ← Template for wrapping a stepper motor
│   └── displays/           ← One file per display type
│       └── NeoPixelDisplayStub.hpp← Template for wrapping a NeoPixel ring
│
├── examples/               ← Consumer entry point examples (copy into your sculpture project)
│   └── basic-clock/
│       └── main.cpp        ← Annotated reference: how to wire drivers to ClockLogic
│
├── docs/                   ← Framework documentation
│   ├── architecture.md              ← Why we decouple hardware from logic
│   ├── directory-structure.md       ← This file
│   ├── mechanisms.md                ← Mechanism handlers: multi-rev, linkage, digit groups, curves
│   ├── using-as-a-library.md        ← How to consume clocksmith as a PlatformIO dependency
│   ├── using-copilot-to-scaffold-a-project.md ← Using Copilot to create a sculpture project
│   ├── adding-a-motor-driver.md     ← Step-by-step: write a new motor driver
│   └── adding-a-display-driver.md   ← Step-by-step: wrap a display library
│
└── learning/               ← AI workflow guides (separate from framework docs)
    ├── README.md                    ← Index and reading order
    ├── 01-vscode-and-copilot-setup.md ← VS Code and Copilot extension setup
    ├── 02-copilot-in-vscode.md      ← Using Copilot Chat, @workspace, prompting
    └── 03-github-cli.md             ← GitHub CLI (gh) installation and usage
```

---

## Folder Roles in Detail

### `include/`

This folder holds **header-only interface definitions** and the HardwareRegistry declaration. PlatformIO automatically adds this directory to the compiler include path for all source files and all libraries in `lib/`. That means any file in the project can write:

```cpp
#include "IMotor.hpp"
```

without needing a relative path.

**Rule:** Only interface headers (`.hpp` files with no implementation) and the HardwareRegistry declaration belong here. No `#include <AccelStepper.h>` or any other hardware library ever appears in this folder.

---

### `src/`

This folder contains two files that serve different purposes:

- **`HardwareRegistry.cpp`** — the implementation of the `HardwareRegistry` class. This is genuine library code, included in every consumer build via `library.json`'s `srcFilter`.
- **`main.cpp`** — a **framework development stub**. It exists solely so that `pio run` inside the clocksmith repository compiles successfully. It contains the bare minimum `setup()` and `loop()` required by the Arduino framework. **This is not a consumer file.** Do not use it as a template for your sculpture project.

For the consumer entry point reference, see **`examples/basic-clock/main.cpp`**.

---

### `examples/`

Each subdirectory of `examples/` is a self-contained consumer entry point that you can copy as-is into your own sculpture project's `src/main.cpp`. PlatformIO and the Arduino IDE both recognise the `examples/` convention.

- **`examples/basic-clock/main.cpp`** — shows how to instantiate and register motor, display, and clock-core drivers, and how to wire them into ClockLogic.

**What lives in `examples/` belongs in your project, not in clocksmith.** These files are annotated reference patterns — copy them, adapt the driver class names and pin numbers to your hardware, and they become your project's real entry point.

---

### `lib/`

PlatformIO treats each subdirectory of `lib/` as an independent library. Libraries in `lib/` are:

1. Compiled separately from `src/`.
2. Automatically discovered — no path configuration needed.
3. Able to include headers from `include/` (PlatformIO adds the project root `include/` to their search path).

`ClockLogic` lives here because it is a self-contained module with no hardware dependencies. If you later want to reuse ClockLogic in a different project, you can copy the `lib/ClockLogic/` folder straight across.

`lib/mechanisms/` contains the four digit mechanism handlers — `SingleMotorDigit`, `MultiRevolutionDigit`, `LinkedMotorDigit`, and `DigitGroup`. These are header-only and depend only on the interfaces in `include/`.

`lib/curves/` contains `LinearCurve` (header-only) and `LookupTableCurve` (header + implementation) for non-linear position correction.

---

### `hal/`

**HAL** stands for Hardware Abstraction Layer. This folder contains the concrete implementations (or stub templates) of the `IMotor` and `IDisplay` interfaces.

These files **do** `#include` real hardware libraries like `AccelStepper` or `Adafruit_NeoPixel`. That is expected and correct — the whole point of the HAL is to isolate library-specific code in one place.

When your HAL drivers are ready to compile, uncomment the following line in `platformio.ini`:

```ini
[platformio]
lib_extra_dirs = hal
```

This tells PlatformIO to treat `hal/motors/` and `hal/displays/` as library source directories.

---

### `docs/`

Human-readable documentation written in Markdown. GitHub renders these files automatically. Every significant decision in the codebase should have a corresponding note in this folder explaining the *why*, not just the *what*.

---

### `learning/`

Tooling and workflow guides for developers who are new to VS Code, GitHub Copilot, or the GitHub CLI. These guides are intentionally separate from the framework documentation — they teach the *tools*, not the clocksmith API. See [`learning/README.md`](../learning/README.md) for the recommended reading order.

---

## Where to Put a New Driver

This table covers two different contexts. Read the context column carefully.

| What you are adding | Where it goes | Context |
|---|---|---|
| A concrete motor driver for your sculpture | `lib/drivers/YourMotor.hpp` | **Consumer project** |
| A concrete display driver for your sculpture | `lib/drivers/YourDisplay.hpp` | **Consumer project** |
| A concrete clock core (RTC/NTP) for your sculpture | `lib/drivers/YourClockCore.hpp` | **Consumer project** |
| A stub/template for a motor type | `hal/motors/StepperMotorStub.hpp` | clocksmith framework |
| A stub/template for a display type | `hal/displays/NeoPixelDisplayStub.hpp` | clocksmith framework |
| A custom digit mechanism (e.g. `BinaryDigit`) | `lib/mechanisms/BinaryDigit.hpp` | clocksmith framework |
| A custom position curve | `lib/curves/MyCurve.hpp` | clocksmith framework |
| New shared logic (e.g. `AlarmLogic`) | `lib/AlarmLogic/AlarmLogic.hpp` + `.cpp` | clocksmith framework |
| A new project-wide interface | `include/IAlarm.hpp` | clocksmith framework |
