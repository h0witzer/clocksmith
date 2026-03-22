# GitHub Copilot Instructions for clocksmith

## What this repository is

clocksmith is a **modular, hardware-agnostic C++ framework** for building Kinetic Clock Sculptures on Arduino and other PlatformIO-supported microcontrollers.

It is published as a **library**, not a project template. Consumer sculpture projects declare it as a `lib_deps` dependency in their `platformio.ini` and never copy its source. See `docs/using-as-a-library.md` for the setup pattern.

---

## Architecture in one diagram

```
[IClockCore]──► ClockLogic ──► HardwareRegistry ──► [IMotor: hour_hand]
                                                 ──► [IMotor: minute_hand]
                                                 ──► [IMotor: second_hand]
                                                 ──► [IDisplay: main_ring]
                                                 ──► [IDigitMechanism: ...]
```

- `ClockLogic` contains **zero hardware calls**. It only calls interface methods.
- `HardwareRegistry` is the runtime glue. Concrete objects are registered under named slots (`Slots::Motor::HOUR_HAND`, etc.) in `main.cpp` and looked up by name inside `ClockLogic`.
- `main.cpp` is **the only file** that `#include`s concrete hardware headers. Everything else works through interfaces.

---

## Directory layout

```
clocksmith/
├── include/            ← Public interfaces and HardwareRegistry (header-only)
│   ├── IMotor.hpp
│   ├── IDisplay.hpp
│   ├── IClockCore.hpp
│   ├── IDigitMechanism.hpp
│   ├── IDigitGroup.hpp
│   ├── IPositionCurve.hpp
│   └── HardwareRegistry.hpp
├── src/
│   ├── main.cpp        ← FRAMEWORK DEV STUB only — not a consumer file
│   └── HardwareRegistry.cpp
├── lib/
│   ├── ClockLogic/     ← Pure time→position logic
│   ├── mechanisms/     ← SingleMotorDigit, MultiRevolutionDigit, LinkedMotorDigit, DigitGroup
│   └── curves/         ← LinearCurve, LookupTableCurve (IPositionCurve implementations)
├── hal/
│   ├── motors/         ← StepperMotorStub.hpp  (copy → rename → fill TODOs)
│   └── displays/       ← NeoPixelDisplayStub.hpp (copy → rename → fill TODOs)
├── examples/
│   └── basic-clock/
│       └── main.cpp    ← Annotated consumer reference; copy into your sculpture project
└── docs/
```

In a **consumer sculpture project**, the layout mirrors this but the framework code lives in `.pio/libdeps/` (never committed):

```
my-sculpture/
├── platformio.ini      ← lib_deps = https://github.com/h0witzer/clocksmith.git
├── lib/drivers/        ← concrete IMotor / IDisplay / IClockCore implementations
└── src/main.cpp        ← the only seam (see clocksmith/examples/basic-clock/main.cpp for the pattern)
```

---

## Interface contracts — what Copilot must know when generating implementations

### IMotor

```cpp
virtual void  setTarget(float position) = 0;   // non-blocking; record target only
virtual void  update()                  = 0;   // called every loop(); one tick of movement
virtual bool  isAtTarget()        const = 0;
virtual float getPosition()       const = 0;
```

- Positions are **normalised floats**. `0.0` = home/12 o'clock, `1.0` = one full revolution.
- Values **greater than 1.0 are valid** for multi-revolution mechanisms (Geneva drives, etc.). Never clamp the upper bound.
- **Clamp negative values to 0.0** inside `setTarget()`.
- `update()` must **never call `delay()`**. Use `millis()`-based timing.
- Concrete objects are declared `static` in `setup()` — never on the heap.

### IDisplay

```cpp
virtual void     setPixel(uint16_t index, uint8_t r, uint8_t g, uint8_t b) = 0;
virtual void     show()                    = 0;
virtual void     clear()                   = 0;
virtual void     update()                  = 0;
virtual uint16_t getPixelCount() const     = 0;
```

- `setPixel()` writes to a **software buffer only** — never to hardware.
- `show()` flushes the buffer to hardware (the "latch").
- Out-of-range `index` values in `setPixel()` must be **silently ignored** (bounds check, no assert, no crash).
- `update()` is for autonomous animation and must be **non-blocking**.

### IClockCore

```cpp
virtual void update()                                               = 0;
virtual void getTime(uint8_t& h, uint8_t& m, uint8_t& s) const     = 0;
virtual void setTime(uint8_t h, uint8_t m, uint8_t s)               = 0;
virtual bool isSynchronised() const                                  = 0;
```

- Hours are **0–23** (24-hour format). ClockLogic handles the 12-hour conversion.
- `update()` must be **non-blocking**; serve the last cached value if hardware is busy.

---

## Key design rules — enforce these in all generated code

1. **No `delay()` anywhere.** Every driver uses `millis()`-based or library-managed non-blocking timing.
2. **`main.cpp` is the only seam.** Never `#include` a concrete hardware header outside `main.cpp`.
3. **Normalised positions.** Motors deal in floats `[0.0, 1.0]` (or above for multi-revolution). Never generate code that passes raw step counts to `ClockLogic` or the mechanism handlers.
4. **Static locals in `setup()`.** Concrete driver objects are declared `static` inside `setup()`, not as global variables and not with `new`. This avoids heap fragmentation on AVR MCUs.
5. **Interface pointers everywhere outside `main.cpp`.** Store `IMotor*`, not `ULN2003StepperMotor*`, except in `main.cpp`.

---

## How to implement a new motor driver

1. Inherit `IMotor`.
2. Constructor takes `totalSteps` (full-revolution step count) plus any pin numbers.
3. `setTarget()`: clamp negative values, multiply `position * _totalSteps` to get an absolute step target, pass it to the library.
4. `update()`: call the library's non-blocking run/step method.
5. `isAtTarget()`: delegate to the library's distance-to-go check.
6. `getPosition()`: return `currentStep / (float)_totalSteps`.
7. Place the file in `lib/drivers/` inside the consumer project (not in clocksmith itself).
8. Add the motor library to `lib_deps` in `platformio.ini`.
9. In `main.cpp`: `#include` the driver, declare `static <ClassName> motor(...)` inside `setup()`, call `registry.registerMotor(Slots::Motor::HOUR_HAND, &motor)`.

---

## How to implement a new display driver

1. Inherit `IDisplay`.
2. Constructor takes `pixelCount` and `dataPin` (plus any library-specific config).
3. `setPixel()`: bounds-check `index`; call the library's pixel-set method on the internal buffer.
4. `show()`: call the library's flush/latch method.
5. `clear()`: call the library's clear method on the buffer.
6. `update()`: advance any animation state using `millis()`; do not block.
7. `getPixelCount()`: return the configured count.
8. Place the file in `lib/drivers/` inside the consumer project.
9. Add the display library to `lib_deps`.
10. In `main.cpp`: `#include`, declare `static`, register under `Slots::Display::MAIN_RING`.

---

## How to wire the consumer's src/main.cpp

This pattern lives in the **consumer sculpture project**, not in the clocksmith repo.
A fully-annotated reference is in `examples/basic-clock/main.cpp` in the clocksmith repo.

```cpp
// my-sculpture/src/main.cpp  ← THE CONSUMER'S FILE, not clocksmith's src/main.cpp
#include "HardwareRegistry.hpp"
#include "ClockLogic.hpp"
// concrete drivers from lib/drivers/ in the consumer project:
#include "MyStepperMotor.hpp"
#include "MyNeoPixelRing.hpp"
#include "MyClockCore.hpp"

HardwareRegistry registry;
IClockCore*      clockCore  = nullptr;
ClockLogic*      clockLogic = nullptr;

void setup()
{
    static MyClockCore      rtc;
    static MyStepperMotor   hourMotor(2048, 8, 9, 10, 11);
    static MyStepperMotor   minuteMotor(2048, 4, 5, 6, 7);
    static MyNeoPixelRing   ring(60, 6);

    registry.registerMotor(Slots::Motor::HOUR_HAND,   &hourMotor);
    registry.registerMotor(Slots::Motor::MINUTE_HAND, &minuteMotor);
    registry.registerDisplay(Slots::Display::MAIN_RING, &ring);

    clockCore = &rtc;
    static ClockLogic logic(*clockCore, registry);
    clockLogic = &logic;
}

void loop()
{
    if (clockLogic) clockLogic->update();

    // call update() on every registered motor/display:
    if (auto* m = registry.getMotor(Slots::Motor::HOUR_HAND))     m->update();
    if (auto* m = registry.getMotor(Slots::Motor::MINUTE_HAND))   m->update();
    if (auto* d = registry.getDisplay(Slots::Display::MAIN_RING)) d->update();
}
```

---

## Stub files to copy when generating new drivers

- Motor stub:   `hal/motors/StepperMotorStub.hpp` — copy, rename, fill the `TODO` sections
- Display stub: `hal/displays/NeoPixelDisplayStub.hpp` — copy, rename, fill the `TODO` sections

---

## What NOT to generate

- Do not generate code that calls `delay()` inside any driver method.
- Do not generate code that uses `new` / `malloc` for driver objects.
- Do not add `#include <AccelStepper.h>` (or any hardware library) outside a concrete driver file or `main.cpp`.
- Do not add clocksmith interface headers to `lib_deps` — they are already provided by the clocksmith library itself.
- Do not generate a `main.cpp` inside the clocksmith repository — that file is only for local development of the framework. Consumer projects have their own `main.cpp`.
