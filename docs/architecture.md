# Architecture Overview

## What Is clocksmith?

clocksmith is a modular C++ framework for building **Kinetic Clock Sculptures** — physical objects where motors move clock hands and LEDs (or other displays) bring the face to life. It is designed to run on microcontrollers using the Arduino framework, and it supports any MCU that PlatformIO can target: Arduino Uno, ESP32, Teensy, and beyond.

---

## The Problem With "Spaghetti Sketches"

When you first learn Arduino, it is natural to write everything in a single `.ino` file:

```cpp
// A typical beginner clock sketch
#include <AccelStepper.h>
#include <Adafruit_NeoPixel.h>
#include <RTClib.h>

AccelStepper stepper(AccelStepper::FULL4WIRE, 8, 10, 9, 11);
Adafruit_NeoPixel strip(60, 6, NEO_GRB + NEO_KHZ800);
RTC_DS3231 rtc;

void loop() {
    DateTime now = rtc.now();
    int targetStep = (now.second() * 4096) / 60;
    stepper.moveTo(targetStep);
    stepper.run();
    strip.setPixelColor(now.second(), strip.Color(255, 0, 0));
    strip.show();
}
```

This works — until it doesn't:

- You buy a different stepper motor with a different step count. You must hunt down every `4096` and change it.
- You want to replace the ring LED with an OLED display. You rewrite the whole loop.
- You want to test the time-to-position maths on your laptop without hardware. You cannot because `RTC_DS3231` requires I²C hardware.
- You add a second sculpture with a servo instead of a stepper. You copy-paste the whole sketch and maintain two diverging codebases.

This is known as **tight coupling**: your business logic (what time is it? where should the hand be?) is inseparably mixed with your hardware implementation (which library, which pins).

---

## The Solution: Separation of Concerns

clocksmith separates the system into four distinct concerns:

| Concern | Interface / Class | Question it answers |
|---|---|---|
| Time source | `IClockCore` | *What time is it right now?* |
| Motor control | `IMotor` | *Move this shaft to this normalised position.* |
| Visual display | `IDisplay` | *Light up this pixel with this colour.* |
| Coordination | `ClockLogic` | *Given the time, which targets should I send to which hardware?* |

These concerns communicate only through **interfaces** (C++ pure virtual base classes, also called Abstract Base Classes or ABCs). An interface is a contract: it defines *what* a component can do, without saying anything about *how* it does it.

---

## The Three Interfaces

### `IClockCore` — the time source

```
IClockCore
├── update()             — tick: refresh internal time registers
├── getTime(h, m, s)     — read current H/M/S
├── setTime(h, m, s)     — set / override the time
└── isSynchronised()     — is the time trustworthy?
```

Concrete implementations can be:
- `DS3231ClockCore` — reads from a DS3231 RTC chip over I²C
- `NTPClockCore` — syncs via WiFi (ESP32/ESP8266)
- `MillisClockCore` — counts seconds using `millis()` (great for prototyping)
- `MockClockCore` — returns a fixed time, ideal for unit tests

### `IMotor` — one clock hand

```
IMotor
├── setTarget(float)     — command a normalised target [0.0, 1.0]
├── update()             — non-blocking step toward target
├── isAtTarget()         — has the shaft reached the target?
└── getPosition()        — current normalised position [0.0, 1.0]
```

Normalised positions map to a 12-hour clock face:

| Position | Meaning |
|---|---|
| `0.0` | 12 o'clock |
| `0.25` | 3 o'clock |
| `0.5` | 6 o'clock |
| `0.75` | 9 o'clock |

Concrete implementations can be:
- `A4988StepperMotor` — NEMA 17 driven by an A4988 breakout
- `ULN2003StepperMotor` — 28BYJ-48 driven by the ULN2003 board
- `ServoMotor` — any standard RC servo

### `IDisplay` — the visual layer

```
IDisplay
├── setPixel(index, r, g, b) — write a colour to the pixel buffer
├── show()                   — push the buffer to the hardware
├── clear()                  — zero out the pixel buffer
├── update()                 — non-blocking animation tick
└── getPixelCount()          — how many pixels are in this display?
```

Concrete implementations can be:
- `NeoPixelRing` — Adafruit NeoPixel ring or strip
- `OLEDDisplay` — SSD1306-based I²C OLED
- `SevenSegmentDisplay` — TM1637 or similar digit display

---

## How They Connect: ClockLogic

`ClockLogic` is the only class that talks to both the time source and the hardware. Every `update()` call:

1. Calls `_clockCore.update()` to refresh time registers
2. Calls `_clockCore.getTime()` to read H/M/S
3. Converts H/M/S to normalised positions using simple arithmetic
4. Calls `setTarget()` on each registered motor
5. Calls `update()` on the registered display

ClockLogic never `#include`s a hardware library. It only includes the interface headers.

```
[IClockCore] ──────> ClockLogic ──────> [IMotor:  hour_hand]
                                    └──> [IMotor:  minute_hand]
                                    └──> [IMotor:  second_hand]
                                    └──> [IDisplay: main_ring]
```

---

## The Registry Pattern

`HardwareRegistry` is the glue between `main.cpp` (which knows about hardware) and `ClockLogic` (which does not). You register concrete objects under named "slots" at startup, and ClockLogic looks them up by name:

```cpp
// main.cpp (setup)
static ULN2003StepperMotor hourMotor(2048, 8, 9, 10, 11);
registry.registerMotor(Slots::Motor::HOUR_HAND, &hourMotor);

// ClockLogic.cpp (update) — no knowledge of ULN2003StepperMotor
IMotor* hand = registry.getMotor(Slots::Motor::HOUR_HAND);
if (hand) hand->setTarget(0.25f);  // 3 o'clock
```

Swapping from a stepper to a servo is a single change in `main.cpp`; nothing else in the project changes.

---

## Non-Blocking Architecture

`delay()` is banned in this project. Every driver implements a non-blocking `update()` method that performs a tiny amount of work each call (one step, one pixel write) and returns immediately. This is called **cooperative multitasking**:

```
loop()
  ├── clockLogic.update()    — read time, send targets
  ├── hourMotor.update()     — step one tick toward target
  ├── minuteMotor.update()   — step one tick toward target
  ├── secondMotor.update()   — step one tick toward target
  └── (future: display animation ticks, button reads, serial logging)
```

As long as every `update()` call returns in under ~1 ms, the whole system feels perfectly smooth.

---

## Further Reading

- [Directory Structure](directory-structure.md) — annotated folder tree
- [Adding a Motor Driver](adding-a-motor-driver.md) — step-by-step guide
- [Adding a Display Driver](adding-a-display-driver.md) — step-by-step guide
