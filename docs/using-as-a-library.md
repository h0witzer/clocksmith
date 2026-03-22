# Using clocksmith as a Library Dependency

## TL;DR

**Do not fork or clone clocksmith to build your sculpture project.** Create a separate project and declare clocksmith as a dependency in `platformio.ini`. Your sculpture project only needs to contain the code that is unique to that sculpture.

---

## Why Not Fork or Clone?

Forking or cloning a framework to build a project based on it creates several problems:

- **Duplication:** Your copy of the framework immediately diverges from the upstream. Bug fixes and new features from clocksmith must be merged manually.
- **Scope confusion:** Your sculpture's specific code (motor pin assignments, calibration tables, your custom drivers) is mixed with the framework code. It is hard to tell what is "yours" and what is "the framework."
- **No versioning:** You cannot pin your project to a known-good version of the framework and upgrade on your own schedule.

The correct pattern — used everywhere from Arduino libraries to embedded C SDKs to enterprise software — is to keep the framework in its own repository, version it, and let every project declare it as a named dependency. Your project contains only what is unique to that project.

---

## How to Reference clocksmith as a PlatformIO Dependency

### 1. Create a new PlatformIO project

Use the PlatformIO CLI or IDE to scaffold a new project for your board:

```sh
pio project init --board uno --ide vscode
```

This gives you a folder with `platformio.ini` and an empty `src/main.cpp`.

### 2. Declare clocksmith in `platformio.ini`

Add a `lib_deps` entry pointing at the clocksmith git repository:

```ini
[env:arduino_uno]
platform    = atmelavr
board       = uno
framework   = arduino
lib_deps    =
    https://github.com/h0witzer/clocksmith.git
```

To pin to a specific release tag (recommended for production):

```ini
lib_deps =
    https://github.com/h0witzer/clocksmith.git#v1.0.0
```

PlatformIO will fetch the library automatically the first time you run `pio run` or `pio pkg install`. The library is stored in `.pio/libdeps/` — you never touch it directly, and it is automatically excluded from your project's git history.

### 3. Include clocksmith headers in your project

Once installed, all clocksmith public headers are on the compiler search path:

```cpp
// src/main.cpp — your sculpture project
#include "IMotor.hpp"
#include "IDisplay.hpp"
#include "IClockCore.hpp"
#include "HardwareRegistry.hpp"
#include "ClockLogic.hpp"

// mechanism handlers (needed only for digit displays)
#include "SingleMotorDigit.hpp"
#include "MultiRevolutionDigit.hpp"
#include "DigitGroup.hpp"
```

### 4. Write your hardware drivers in your project

Create a `lib/` folder inside your sculpture project for your concrete drivers:

```
my-sculpture/
├── platformio.ini
├── lib/
│   └── drivers/
│       ├── ULN2003StepperMotor.hpp   ← your motor driver
│       └── NeoPixelRing.hpp          ← your display driver
└── src/
    └── main.cpp                      ← wires everything together
```

Use the stubs in clocksmith's `hal/` directory as starting templates. To access them, look inside `.pio/libdeps/<env>/clocksmith/hal/` after the first `pio pkg install`, or refer to them directly on GitHub:

- [hal/motors/StepperMotorStub.hpp](https://github.com/h0witzer/clocksmith/blob/main/hal/motors/StepperMotorStub.hpp)
- [hal/displays/NeoPixelDisplayStub.hpp](https://github.com/h0witzer/clocksmith/blob/main/hal/displays/NeoPixelDisplayStub.hpp)

Your driver must inherit the relevant interface (`IMotor`, `IDisplay`, etc.) and implement its abstract methods. See [adding-a-motor-driver.md](adding-a-motor-driver.md) for a step-by-step walkthrough.

### 5. Wire everything together in `src/main.cpp`

```cpp
// src/main.cpp — the ONLY file that knows about concrete hardware
#include "HardwareRegistry.hpp"
#include "ClockLogic.hpp"
#include "IClockCore.hpp"

// Your sculpture-specific drivers (in lib/drivers/):
#include "ULN2003StepperMotor.hpp"
#include "NeoPixelRing.hpp"
#include "MillisClockCore.hpp"

HardwareRegistry registry;
IClockCore*      clockCore  = nullptr;
ClockLogic*      clockLogic = nullptr;

void setup()
{
    Serial.begin(115200);

    static ULN2003StepperMotor hourMotor(2048, 8, 9, 10, 11);
    registry.registerMotor(Slots::Motor::HOUR_HAND, &hourMotor);

    static ULN2003StepperMotor minuteMotor(2048, 4, 5, 6, 7);
    registry.registerMotor(Slots::Motor::MINUTE_HAND, &minuteMotor);

    static NeoPixelRing ring(60, /*dataPin=*/6);
    registry.registerDisplay(Slots::Display::MAIN_RING, &ring);

    static MillisClockCore rtc;
    rtc.setTime(12, 0, 0);
    clockCore = &rtc;

    static ClockLogic logic(*clockCore, registry);
    clockLogic = &logic;
}

void loop()
{
    if (clockLogic) clockLogic->update();

    IMotor* h = registry.getMotor(Slots::Motor::HOUR_HAND);
    if (h) h->update();

    IMotor* m = registry.getMotor(Slots::Motor::MINUTE_HAND);
    if (m) m->update();

    IDisplay* d = registry.getDisplay(Slots::Display::MAIN_RING);
    if (d) d->update();
}
```

---

## Minimal Consumer Project Structure

```
my-sculpture/
├── .gitignore          ← add .pio/ to this — never commit the installed library
├── platformio.ini      ← declares lib_deps = https://github.com/h0witzer/clocksmith.git
├── lib/
│   └── drivers/
│       ├── ULN2003StepperMotor.hpp   ← implements IMotor
│       ├── DS3231ClockCore.hpp       ← implements IClockCore
│       └── NeoPixelRing.hpp          ← implements IDisplay
└── src/
    └── main.cpp        ← wires drivers to ClockLogic; the only seam
```

Everything inside `lib/drivers/` is unique to your sculpture. Everything from clocksmith is fetched automatically and never checked in.

---

## Arduino IDE (Without PlatformIO)

If you are using the Arduino IDE rather than PlatformIO:

1. Download the clocksmith release ZIP from [GitHub Releases](https://github.com/h0witzer/clocksmith/releases).
2. In the Arduino IDE: **Sketch → Include Library → Add .ZIP Library…** and select the downloaded file.
3. clocksmith will appear in **Sketch → Include Library** and its headers will be available to your sketch.

> **Tip:** For a more professional setup, switch to PlatformIO. It gives you proper version pinning, dependency management, and support for multiple boards in one project.

---

## Version Pinning and Updating

| Goal | `lib_deps` syntax |
|---|---|
| Always latest | `https://github.com/h0witzer/clocksmith.git` |
| Pinned to a release | `https://github.com/h0witzer/clocksmith.git#v1.0.0` |
| Pinned to a commit | `https://github.com/h0witzer/clocksmith.git#abc1234` |

To update a pinned dependency: change the tag in `platformio.ini` and run `pio pkg install`. The rest of your project is unaffected.

---

## When IS Forking Appropriate?

Forking clocksmith is appropriate only when you want to **contribute to the framework itself** — adding a new interface, fixing a bug in ClockLogic, or improving the documentation. In that case:

1. Fork the repo on GitHub.
2. Clone your fork locally.
3. Open the cloned folder in PlatformIO.
4. Make your changes against the framework code.
5. Open a pull request back to `h0witzer/clocksmith`.

Your sculpture project is still a separate repository that depends on clocksmith — it is not the same repo as your fork.

---

## Further Reading

- [architecture.md](architecture.md) — why we decouple hardware from logic
- [using-copilot-to-scaffold-a-project.md](using-copilot-to-scaffold-a-project.md) — how to use GitHub Copilot to generate drivers and work across both repos
- [adding-a-motor-driver.md](adding-a-motor-driver.md) — step-by-step guide to writing a new motor driver in your consumer project
- [adding-a-display-driver.md](adding-a-display-driver.md) — step-by-step guide to wrapping a display library
- [mechanisms.md](mechanisms.md) — multi-revolution drives, linkages, digit groups, non-linear curves
