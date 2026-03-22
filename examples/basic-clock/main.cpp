/**
 * @file examples/basic-clock/main.cpp
 * @brief Consumer entry point reference — copy this into YOUR sculpture project.
 *
 * ============================================================================
 * THIS IS AN EXAMPLE FOR YOUR SCULPTURE PROJECT
 * ============================================================================
 *
 * This file does NOT belong in the clocksmith repository.  Copy it into your
 * own sculpture project's src/main.cpp and adapt it to your hardware.
 *
 * Refer to docs/using-as-a-library.md for the step-by-step setup guide.
 *
 * ============================================================================
 * THE ONLY "SEAM"
 * ============================================================================
 *
 * This is the ONLY file in your sculpture project that is allowed to
 * #include concrete hardware drivers (AccelStepper, Adafruit_NeoPixel, RTClib,
 * etc.).  Everything else in your project works through the abstract interfaces
 * (IMotor, IDisplay, IClockCore).
 *
 * Think of this file as the electrical panel in a building: all the wires
 * from the walls (hardware) meet here and connect to the circuits (logic).
 * Nobody inside the building needs to know what is in the panel.
 *
 * ============================================================================
 * HOW TO ADD A MOTOR DRIVER
 * ============================================================================
 *
 *  1. Create a driver class in your project's lib/drivers/ that inherits IMotor.
 *     Copy hal/motors/StepperMotorStub.hpp as your starting template.
 *     Read docs/adding-a-motor-driver.md for a step-by-step walkthrough.
 *
 *  2. #include that file here (in the "Concrete drivers" section below).
 *
 *  3. Instantiate it as a static local in setup() and register it:
 *       static MyStepperMotor hourMotor(2048, PIN_A, PIN_B, PIN_C, PIN_D);
 *       registry.registerMotor(Slots::Motor::HOUR_HAND, &hourMotor);
 *
 *  4. Call update() on it every loop() iteration (see loop() below).
 *
 * ============================================================================
 * HOW TO ADD A DISPLAY DRIVER
 * ============================================================================
 *
 *  1. Create a driver class in your project's lib/drivers/ that inherits IDisplay.
 *     Copy hal/displays/NeoPixelDisplayStub.hpp as your starting template.
 *     Read docs/adding-a-display-driver.md for a step-by-step walkthrough.
 *
 *  2. #include that file here and register it:
 *       static MyNeoPixelRing ring(60, PIN_DATA);
 *       registry.registerDisplay(Slots::Display::MAIN_RING, &ring);
 *
 *  3. Call update() on it every loop() iteration (see loop() below).
 *
 * ============================================================================
 */

// clocksmith interfaces and registry (provided by the library via lib_deps)
#include "IClockCore.hpp"
#include "IMotor.hpp"
#include "IDisplay.hpp"
#include "HardwareRegistry.hpp"
#include "ClockLogic.hpp"

// ---------------------------------------------------------------------------
// Concrete drivers — these live in YOUR project's lib/drivers/
// ---------------------------------------------------------------------------
// Replace these includes with your actual driver files:
//
// #include "ULN2003StepperMotor.hpp"   // implements IMotor
// #include "NeoPixelRing.hpp"          // implements IDisplay
// #include "MillisClockCore.hpp"       // implements IClockCore
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Global objects
// ---------------------------------------------------------------------------
// Objects declared at file scope live for the entire program lifetime without
// heap allocation — correct practice for embedded C++ on memory-constrained
// MCUs (e.g. Arduino Uno with 2 KB of RAM).

HardwareRegistry registry;

// Pointers initialised during setup().
IClockCore* clockCore  = nullptr;
ClockLogic* clockLogic = nullptr;

// ---------------------------------------------------------------------------
// setup() — runs once at power-on / reset
// ---------------------------------------------------------------------------

void setup()
{
    Serial.begin(115200);

    // ------------------------------------------------------------------
    // 1. Instantiate and register motor drivers as static locals.
    //    Static locals live for the whole program lifetime without using
    //    the heap.  Never use `new` for driver objects.
    // ------------------------------------------------------------------
    //
    // static ULN2003StepperMotor hourMotor(2048, 8, 9, 10, 11);
    // registry.registerMotor(Slots::Motor::HOUR_HAND, &hourMotor);
    //
    // static ULN2003StepperMotor minuteMotor(2048, 4, 5, 6, 7);
    // registry.registerMotor(Slots::Motor::MINUTE_HAND, &minuteMotor);
    //
    // static ULN2003StepperMotor secondMotor(4096, 0, 1, 2, 3);
    // registry.registerMotor(Slots::Motor::SECOND_HAND, &secondMotor);

    // ------------------------------------------------------------------
    // 2. Instantiate and register the display driver.
    // ------------------------------------------------------------------
    //
    // static NeoPixelRing ring(60, /* dataPin = */ 6);
    // registry.registerDisplay(Slots::Display::MAIN_RING, &ring);

    // ------------------------------------------------------------------
    // 3. Instantiate the time source.
    // ------------------------------------------------------------------
    //
    // static MillisClockCore rtc;
    // rtc.setTime(12, 0, 0);   // optional: set an initial time
    // clockCore = &rtc;

    // ------------------------------------------------------------------
    // 4. Wire ClockLogic to the time source and registry.
    // ------------------------------------------------------------------
    //
    // if (clockCore)
    // {
    //     static ClockLogic logic(*clockCore, registry);
    //     clockLogic = &logic;
    // }
}

// ---------------------------------------------------------------------------
// loop() — runs repeatedly after setup() returns
// ---------------------------------------------------------------------------

void loop()
{
    // The golden rule of non-blocking embedded design:
    //   Call update() on EVERY object EVERY loop iteration.
    //   NEVER call delay() anywhere in your driver or logic code.

    // ClockLogic reads the current time and sends normalised targets to each
    // registered motor / digit mechanism / display:
    if (clockLogic) clockLogic->update();

    // Each motor must also be ticked so it steps toward its target.
    // ClockLogic only commands targets — it does not drive motors itself.
    //
    // IMotor* h = registry.getMotor(Slots::Motor::HOUR_HAND);
    // if (h) h->update();
    //
    // IMotor* m = registry.getMotor(Slots::Motor::MINUTE_HAND);
    // if (m) m->update();
    //
    // IMotor* s = registry.getMotor(Slots::Motor::SECOND_HAND);
    // if (s) s->update();

    // Tick the display so it can process any animation state:
    //
    // IDisplay* d = registry.getDisplay(Slots::Display::MAIN_RING);
    // if (d) d->update();
}
