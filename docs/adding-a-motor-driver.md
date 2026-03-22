# How to Write a Motor Driver

This guide walks you through wrapping a real stepper motor library into the `IMotor` interface so that ClockLogic can drive it. By the end, you will have a working motor driver that slots into the clocksmith framework without touching any other file except `main.cpp`.

---

## Prerequisites

- You have read [architecture.md](architecture.md) and understand why we use interfaces.
- You have a motor connected to your microcontroller.
- You know which library you will use (AccelStepper, Servo, etc.).
- PlatformIO is installed and the project builds (even with the stubs).

---

## Step 1 — Understand What You Are Building

The `IMotor` interface defines a contract with four methods:

```cpp
virtual void  setTarget(float position) = 0;  // normalised position; clamp negative to 0.0
virtual void  update()                  = 0;  // non-blocking tick
virtual bool  isAtTarget()        const = 0;  // has it arrived?
virtual float getPosition()       const = 0;  // current normalised position
```

Your driver class must inherit `IMotor` and implement all four. The framework will call `setTarget()` when the time changes and `update()` every loop iteration. Your driver translates these generic calls into AccelStepper (or your library's) API.

> **Position range:** For standard analogue clock hands the position is in `[0.0, 1.0]`. For multi-revolution mechanisms (Geneva drives, odometers) values **greater than 1.0 are explicitly valid** — e.g. `3.5` means "3.5 full revolutions from home". Only **negative** values must be clamped; never clamp the upper bound.

---

## Step 2 — Copy the Stub File

In your **sculpture project** (not the clocksmith repo), create a `lib/drivers/` folder if it doesn't exist. Copy `hal/motors/StepperMotorStub.hpp` from the clocksmith library — available at `.pio/libdeps/<env>/clocksmith/hal/motors/StepperMotorStub.hpp` after `pio pkg install`, or [directly on GitHub](https://github.com/h0witzer/clocksmith/blob/main/hal/motors/StepperMotorStub.hpp) — and save it as your new driver:

```
lib/drivers/ULN2003StepperMotor.hpp   (28BYJ-48 + ULN2003 driver board)
lib/drivers/A4988StepperMotor.hpp     (NEMA 17 + A4988 breakout)
lib/drivers/ServoMotor.hpp            (any standard RC servo)
```

Open your new file and change the class name at the top to match the filename.

---

## Step 3 — Add the Library Dependency

In your sculpture project's `platformio.ini`, declare both clocksmith and your motor library under `lib_deps`:

```ini
[env:arduino_uno]
platform    = atmelavr
board       = uno
framework   = arduino
lib_deps    =
    https://github.com/h0witzer/clocksmith.git
    waspinator/AccelStepper   ; for stepper motors
    ; or: arduino-libraries/Servo  ; for servo motors
```

Run `pio pkg install` (or let the IDE sync) to download both.

---

## Step 4 — Implement the Constructor

The constructor is where you configure your hardware. It runs once, during `setup()` in `main.cpp`.

```cpp
#include <AccelStepper.h>
#include "IMotor.hpp"

class ULN2003StepperMotor : public IMotor
{
public:
    ULN2003StepperMotor(int totalSteps,
                        uint8_t pin1, uint8_t pin2,
                        uint8_t pin3, uint8_t pin4)
        : _totalSteps(totalSteps)
        , _stepper(AccelStepper::HALF3WIRE, pin1, pin3, pin2, pin4)
    {
        _stepper.setMaxSpeed(1000.0f);
        _stepper.setAcceleration(500.0f);
        _stepper.setCurrentPosition(0);
    }
    // ...
};
```

**Why static locals in main.cpp?**
When you instantiate the motor in `setup()` with `static`, the object lives for the entire program lifetime without using the heap. Heap fragmentation is a common source of mysterious crashes on AVR MCUs with only 2 KB of RAM.

---

## Step 5 — Implement `setTarget(float position)`

This method must:
1. Clamp negative values to `0.0` (a motor cannot go "before" its home position).
2. **Do NOT clamp the upper bound.** Values greater than `1.0` are valid and represent multi-revolution positions used by `MultiRevolutionDigit` and similar handlers.
3. Convert the normalised float to an absolute step count.
4. Tell the library where to go.

```cpp
void setTarget(float position) override
{
    if (position < 0.0f) position = 0.0f;
    // Do NOT clamp position > 1.0 — values above 1.0 are valid for
    // multi-revolution mechanisms (Geneva drives, odometers, etc.).

    const long target = static_cast<long>(
        position * static_cast<float>(_totalSteps));

    _stepper.moveTo(target);
}
```

The `_totalSteps` value for common motors:

| Motor | Mode | `totalSteps` |
|---|---|---|
| 28BYJ-48 + ULN2003 | Full-step | 2048 |
| 28BYJ-48 + ULN2003 | Half-step | 4096 |
| NEMA 17 (200 step) | Full | 200 |
| NEMA 17 + A4988 @ 1/16 | Micro | 3200 |

---

## Step 6 — Implement `update()`

This is called every `loop()` iteration. It must be **non-blocking** — do one small unit of work and return.

```cpp
void update() override
{
    _stepper.run();  // AccelStepper::run() is already non-blocking
}
```

**What does "non-blocking" mean?**
`AccelStepper::run()` checks whether it is time to fire the next step pulse (using a microsecond timer), fires it if so, and returns. It never waits. This is correct behaviour — your `update()` should mimic this pattern.

If you are writing a driver without AccelStepper, the pattern is:

```cpp
void update() override
{
    const unsigned long now = micros();
    if (now - _lastStepTime >= _stepIntervalUs)
    {
        // fire one step
        _lastStepTime = now;
    }
}
```

---

## Step 7 — Implement `isAtTarget()` and `getPosition()`

```cpp
bool isAtTarget() const override
{
    return _stepper.distanceToGo() == 0;
}

float getPosition() const override
{
    if (_totalSteps == 0) return 0.0f;
    return static_cast<float>(_stepper.currentPosition())
         / static_cast<float>(_totalSteps);
}
```

---

## Step 8 — Register the Motor in `main.cpp`

In your sculpture project's `src/main.cpp`, add the `#include` and instantiation. PlatformIO automatically discovers files in `lib/drivers/`, so you can include by filename only — no path prefix needed:

```cpp
// Add near the top of main.cpp (the only file allowed to include concrete drivers):
#include "ULN2003StepperMotor.hpp"

// Inside setup():
static ULN2003StepperMotor hourMotor(2048, 8, 9, 10, 11);
registry.registerMotor(Slots::Motor::HOUR_HAND, &hourMotor);

static ULN2003StepperMotor minuteMotor(2048, 4, 5, 6, 7);
registry.registerMotor(Slots::Motor::MINUTE_HAND, &minuteMotor);

// Inside loop(), call update() for every motor:
IMotor* h = registry.getMotor(Slots::Motor::HOUR_HAND);
if (h) h->update();

IMotor* m = registry.getMotor(Slots::Motor::MINUTE_HAND);
if (m) m->update();
```

---

## Step 9 — Test Without Hardware (Optional)

You can write a minimal `MockMotor` in a test file that records the last position set and always returns `isAtTarget() == true`. This lets you run ClockLogic on a desktop build and verify the position maths before plugging in any hardware.

---

## Checklist

Before you mark the driver as done, verify:

- [ ] The class inherits `IMotor` and all four pure virtual methods are implemented.
- [ ] `setTarget()` clamps **only negative** values to `0.0`; values above `1.0` are **not** clamped.
- [ ] `update()` contains no `delay()` call.
- [ ] `getPosition()` returns a non-negative float (may exceed `1.0` for multi-revolution use).
- [ ] The driver file is in `lib/drivers/` of your sculpture project.
- [ ] The motor is registered in `main.cpp` under a `Slots::Motor` constant.
- [ ] `update()` is called in `loop()`.

---

## Further Reading

- [IMotor interface](../include/IMotor.hpp)
- [StepperMotorStub template](../hal/motors/StepperMotorStub.hpp)
- [Architecture overview](architecture.md)
