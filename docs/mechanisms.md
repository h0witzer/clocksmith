# Mechanism Handlers

This document explains the four specialised mechanism handlers that clocksmith provides for complex kinetic displays, and shows you how and when to use each one.

---

## Background: Why Mechanisms Exist

The base `IMotor` interface is ideal for smooth, analogue clock hands — it speaks in normalised shaft positions `[0.0, 1.0]` and moves continuously. Many kinetic sculptures, however, display **digits** — discrete numerals 0 through 9 driven by physical mechanisms that have properties the basic motor interface cannot express:

| Complexity | Mechanism Handler |
|---|---|
| One motor, one digit, linear scale | `SingleMotorDigit` |
| One motor, multiple revolutions per digit sweep | `MultiRevolutionDigit` |
| Two motors working together to point at a digit | `LinkedMotorDigit` |
| Separate tens and ones display units | `DigitGroup` |
| Non-linear mechanical output | `IPositionCurve` + `LookupTableCurve` |

All four handlers implement the `IDigitMechanism` interface (`include/IDigitMechanism.hpp`), which means ClockLogic can drive any of them with a single `setDigit()` call — the mechanical complexity is entirely hidden inside the handler.

---

## The Digit Interface Stack

```
ClockLogic
  │
  ├── IMotor*            (analogue hand path — unchanged)
  │
  └── IDigitMechanism*   (digit path — new)
        │
        ├── SingleMotorDigit       — 1 motor, linear or curved
        ├── MultiRevolutionDigit   — 1 motor, N revolutions per sweep
        ├── LinkedMotorDigit       — 2 motors + ILinkageSolver
        └── DigitGroup             — groups N IDigitMechanisms for one time unit
              └── IDigitGroup* (registered in HardwareRegistry)
```

---

## 1. SingleMotorDigit

**File:** `lib/mechanisms/SingleMotorDigit.hpp`

### What it does

The simplest digit handler. One motor, one digit, one full revolution (or less) to sweep from digit 0 to digit 9.

### When to use it

- Your digit mechanism sweeps fully through 0–9 within one motor revolution.
- The output may or may not be perfectly linear (use a `LookupTableCurve` if not).

### Minimal example

```cpp
// In main.cpp, setup():
#include "../hal/motors/StepperMotorStub.hpp"
#include "../lib/mechanisms/SingleMotorDigit.hpp"

static StepperMotorStub motor(2048);
static SingleMotorDigit minutesOnes(motor);        // linear, full 0–9 range
registry.registerDigitMechanism(Slots::Digit::MINUTES_ONES, &minutesOnes);
```

### With a position curve

```cpp
#include "../lib/curves/LookupTableCurve.hpp"

// Calibration table: at each logical position, what physical position
// does the motor need to reach?  Measured by hand against the sculpture.
static const LookupTableCurve::Point kCurvePoints[] = {
    {0.00f, 0.00f},   // digit 0 → motor at 0.0
    {0.25f, 0.28f},   // digit 2–3 region is slightly compressed
    {0.50f, 0.54f},
    {0.75f, 0.71f},
    {1.00f, 1.00f},   // digit 9 → motor at 1.0
};
static LookupTableCurve myCurve(kCurvePoints, 5);
static SingleMotorDigit minutesOnes(motor, 9, &myCurve);
```

---

## 2. MultiRevolutionDigit

**File:** `lib/mechanisms/MultiRevolutionDigit.hpp`

### What it does

Wraps one motor and scales the position command so that the motor turns **more than one full revolution** to sweep through the complete digit range. The motor's `setTarget()` receives values greater than 1.0.

### When to use it

- Your digit display uses a **Geneva drive**, **worm gear**, or other mechanism where the output advances one digit position per full (or multiple) motor revolution(s).
- An **odometer-style digit drum** where the gear ratio means several motor turns equal one digit step.

### How many revolutions?

Measure (or calculate from your gear ratio) how many motor revolutions it takes to advance from digit 0 to digit 9. That is `revolutionsPerSweep`.

Example: if your gearbox has a 3:1 ratio and it takes 3 input revolutions to show all 10 digits, then `revolutionsPerSweep = 3.0`.

### Minimal example

```cpp
#include "../lib/mechanisms/MultiRevolutionDigit.hpp"

static StepperMotorStub genevaDriveMotor(2048);
// This mechanism needs 4 full revolutions to sweep digits 0–9.
static MultiRevolutionDigit secondsOnes(genevaDriveMotor, /* revolutionsPerSweep = */ 4.0f);
registry.registerDigitMechanism(Slots::Digit::SECONDS_ONES, &secondsOnes);
```

### Position maths

```
motorPosition = (digit / digitRange) * revolutionsPerSweep

digit 0 → 0.0   (home)
digit 4 → (4/9) * 4.0 ≈ 1.78   (almost 2 full revolutions)
digit 9 → (9/9) * 4.0 = 4.0    (four full revolutions from home)
```

### Important: homing

The motor must know where "home" (position 0.0) is before `setDigit()` is called. Add a homing routine to your `setup()` function that moves the motor to a limit switch or optical sensor, then calls `_stepper.setCurrentPosition(0)`.

---

## 3. LinkedMotorDigit

**File:** `lib/mechanisms/LinkedMotorDigit.hpp`

### What it does

Drives **two motors simultaneously** using a user-provided `ILinkageSolver`. The solver contains the kinematic equations that convert a single logical digit position into separate positions for each motor.

### When to use it

- Two arms of a **mechanical linkage** together indicate a digit (like a pantograph or a two-jointed arm pointing to a number on a dial).
- A **differential mechanism** where the sum or difference of two shaft angles encodes the displayed value.
- Any mechanism where one logical value requires two motors to work in concert.

### The ILinkageSolver contract

You subclass `ILinkageSolver` and implement `solve()`:

```cpp
class ILinkageSolver {
public:
    virtual void solve(float  logicalPosition,   // [0.0, 1.0]
                       float& posA,              // output for motor A
                       float& posB) const = 0;   // output for motor B
};
```

`logicalPosition` is `digit / digitRange` (0.0 for digit 0, 1.0 for digit 9).  Your `solve()` fills `posA` and `posB` with the corresponding physical positions for each motor.

### Writing a solver — worked example

Imagine a two-arm display where:
- Arm A rotates between 0.1 and 0.9 of a revolution.
- Arm B mirrors arm A in reverse (rotates in the opposite direction).
- Together they form a "V" shape that indicates a digit on a curved scale.

```cpp
class TwoArmSolver : public ILinkageSolver {
public:
    void solve(float logicalPos, float& posA, float& posB) const override {
        // Arm A moves forward from 10% to 90% of a revolution.
        posA = 0.1f + logicalPos * 0.8f;

        // Arm B mirrors arm A (rotates in reverse).
        posB = 0.9f - logicalPos * 0.8f;
    }
};
```

For a real physical linkage with known arm lengths L1 and L2 and pivot geometry, you would use the inverse kinematics equations for a two-link planar arm.  See the field of **robot kinematics** for the maths — the solve() method is the ideal place to put those equations because it keeps them isolated and testable.

### Minimal example

```cpp
#include "../lib/mechanisms/LinkedMotorDigit.hpp"

static StepperMotorStub shoulderMotor(2048);
static StepperMotorStub elbowMotor(2048);
static TwoArmSolver     mySolver;
static LinkedMotorDigit hoursOnes(shoulderMotor, elbowMotor, mySolver);

registry.registerDigitMechanism(Slots::Digit::HOURS_ONES, &hoursOnes);
```

---

## 4. DigitGroup

**File:** `lib/mechanisms/DigitGroup.hpp`

### What it does

Groups independent digit mechanisms for a single time unit (e.g. the tens digit and the ones digit of the minutes display) so ClockLogic can command them with one integer value.

### When to use it

- Your display has separate physical mechanisms for tens and ones digits.
- You want to keep your `setup()` code organised by time unit rather than by individual digit.
- The tens and ones digits may be different mechanism types (e.g. tens = `SingleMotorDigit`, ones = `MultiRevolutionDigit`).

### Digit positions

```
position 0 = ones digit  (least significant, rightmost)
position 1 = tens digit
position 2 = hundreds digit (rare for time displays)
```

### How setValue() splits the number

```
setValue(47)
  → mechanism[0].setDigit(47 % 10) = setDigit(7)   (ones)
  → mechanism[1].setDigit(47 / 10) = setDigit(4)   (tens)
```

### Minimal example

```cpp
#include "../lib/mechanisms/SingleMotorDigit.hpp"
#include "../lib/mechanisms/MultiRevolutionDigit.hpp"
#include "../lib/mechanisms/DigitGroup.hpp"

static StepperMotorStub motorMinuteOnes(2048);
static StepperMotorStub motorMinuteTens(2048);

// Ones digit: single revolution (digits 0–9)
static SingleMotorDigit minutesOnesDigit(motorMinuteOnes);

// Tens digit: multi-revolution Geneva drive (digits 0–5 for tens-of-minutes)
static MultiRevolutionDigit minutesTensDigit(motorMinuteTens, /* revs = */ 2.0f, /* range = */ 5);

static DigitGroup minutesGroup;
// setup():
minutesGroup.setMechanism(0, &minutesOnesDigit);  // position 0 = ones
minutesGroup.setMechanism(1, &minutesTensDigit);  // position 1 = tens
registry.registerDigitGroup(Slots::DigitGroup::MINUTES, &minutesGroup);
```

ClockLogic will then call `minutesGroup.setValue(47)` automatically.

### loop() — don't forget to tick the group

Even when using a DigitGroup, you must call `group.update()` in `loop()` because ClockLogic only sends targets — it doesn't drive the individual motors itself:

```cpp
// loop():
IDigitGroup* mg = registry.getDigitGroup(Slots::DigitGroup::MINUTES);
if (mg) mg->update();
```

---

## 5. Non-Linear Correction with IPositionCurve

**Files:** `include/IPositionCurve.hpp`, `lib/curves/LinearCurve.hpp`, `lib/curves/LookupTableCurve.hpp`

### The problem

In an ideal mechanism, motor position and displayed value are proportional.  Real mechanisms aren't ideal: cams, non-circular gears, and off-centre pivots all introduce non-linearity.  The result is that digit positions look evenly spaced when measured in steps but unevenly spaced when you look at the display.

### The solution

An `IPositionCurve` maps a **logical** position (the evenly-spaced value you want to display) to a **physical** motor position (the uneven position the motor must reach to produce the correct visual result).

```
logical = digit / digitRange         (what you want to display)
physical = curve.map(logical)        (where the motor must go)
motor.setTarget(physical)
```

### LinearCurve — the default

Use `LinearCurve` (or pass `nullptr`) when your mechanism is linear.  It is a zero-overhead identity function that the compiler will optimise away:

```cpp
static LinearCurve flat;
static SingleMotorDigit d(motor, 9, &flat);
// equivalent:
static SingleMotorDigit d(motor);  // nullptr defaults to linear
```

### LookupTableCurve — calibrated correction

1. Build a calibration table by commanding the motor to known positions and recording what the display actually shows.
2. Express each measurement as a `{logical, physical}` point.
3. Sort the table by ascending `logical` value (required).

```cpp
static const LookupTableCurve::Point kCalibration[] = {
    {0.00f, 0.00f},   // digit 0: no correction needed
    {0.11f, 0.13f},   // digit 1: motor needs to go slightly further
    {0.22f, 0.26f},   // digit 2
    {0.33f, 0.37f},   // digit 3
    {0.44f, 0.47f},   // digit 4
    {0.56f, 0.58f},   // digit 5
    {0.67f, 0.68f},   // digit 6
    {0.78f, 0.78f},   // digit 7: linear from here
    {0.89f, 0.89f},   // digit 8
    {1.00f, 1.00f},   // digit 9: no correction needed
};
static LookupTableCurve myCurve(kCalibration, 10);
static SingleMotorDigit correctedDigit(motor, 9, &myCurve);
```

Between calibration points, the curve interpolates linearly, producing a smooth correction across the full digit range.

---

## Complete Registration Checklist

For each digit display in your sculpture:

1. Choose the appropriate mechanism type based on the table at the top of this document.
2. Instantiate the required motor(s) as static locals in `setup()`.
3. Instantiate any position curve as a static local (with its calibration table as `static const`).
4. Instantiate the mechanism handler as a static local.
5. Register it in the HardwareRegistry under the appropriate slot.
6. Call `update()` on it in `loop()` (or on its DigitGroup).

---

## Further Reading

- [IDigitMechanism interface](../include/IDigitMechanism.hpp)
- [IPositionCurve interface](../include/IPositionCurve.hpp)
- [IDigitGroup interface](../include/IDigitGroup.hpp)
- [Architecture overview](architecture.md)
- [Adding a motor driver](adding-a-motor-driver.md)
