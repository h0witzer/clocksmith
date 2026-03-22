/**
 * @file main.cpp
 * @brief Application entry point — wire the hardware to the logic here.
 *
 * ============================================================================
 * THIS FILE IS THE ONLY "SEAM"
 * ============================================================================
 *
 * This is the ONLY file in the project that is allowed to #include concrete
 * hardware libraries (e.g. AccelStepper, Adafruit_NeoPixel, RTClib).
 * Everything else in the project only includes the abstract interfaces
 * (IMotor, IDisplay, IClockCore).
 *
 * Think of this file as the electrical panel in a building: all the wires
 * from the walls (hardware) meet here and get connected to the circuits
 * (logic).  Nobody inside the building needs to know what's in the panel.
 *
 * ============================================================================
 * HOW TO ADD A REAL MOTOR
 * ============================================================================
 *
 *  1. Create a driver class in hal/motors/ that inherits IMotor.
 *     Use hal/motors/StepperMotorStub.hpp as your starting template.
 *     Read docs/adding-a-motor-driver.md for a step-by-step walkthrough.
 *
 *  2. #include that file below (in the "HAL includes" section).
 *
 *  3. Instantiate it as a static local in setup() and register it:
 *       static MyStepperMotor hourMotor(2048, PIN_A, PIN_B, PIN_C, PIN_D);
 *       registry.registerMotor(Slots::Motor::HOUR_HAND, &hourMotor);
 *
 * ============================================================================
 * HOW TO ADD A REAL DISPLAY
 * ============================================================================
 *
 *  1. Create a driver class in hal/displays/ that inherits IDisplay.
 *     Use hal/displays/NeoPixelDisplayStub.hpp as your starting template.
 *     Read docs/adding-a-display-driver.md for a step-by-step walkthrough.
 *
 *  2. #include that file below and register it:
 *       static MyNeoPixelDisplay ring(60, PIN_DATA);
 *       registry.registerDisplay(Slots::Display::MAIN_RING, &ring);
 *
 * ============================================================================
 */

// Project-wide interfaces and registry
#include "IClockCore.hpp"
#include "IMotor.hpp"
#include "IDisplay.hpp"
#include "HardwareRegistry.hpp"

// ClockLogic library
#include "ClockLogic.hpp"

// ---------------------------------------------------------------------------
// HAL includes — uncomment and add your real driver headers here
// ---------------------------------------------------------------------------
// #include "../hal/motors/StepperMotorStub.hpp"
// #include "../hal/displays/NeoPixelDisplayStub.hpp"
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Global objects
// ---------------------------------------------------------------------------
// Objects declared at file scope live for the entire program lifetime.
// This is standard practice in embedded C++ — it avoids heap allocation and
// makes lifetimes explicit.

HardwareRegistry registry;

// Pointers set during setup().  We use pointers (not references) here so
// that setup() can assign concrete types without a forward declaration.
IClockCore* clockCore  = nullptr;
ClockLogic* clockLogic = nullptr;

// ---------------------------------------------------------------------------
// setup() — runs once at power-on / reset
// ---------------------------------------------------------------------------

void setup()
{
    Serial.begin(115200);
    Serial.println(F("[clocksmith] booting..."));

    // ------------------------------------------------------------------
    // 1. Instantiate and register motor drivers.
    //    Replace the StepperMotorStub with your real implementation.
    // ------------------------------------------------------------------
    //
    // static StepperMotorStub hourMotor(2048);
    // registry.registerMotor(Slots::Motor::HOUR_HAND,   &hourMotor);
    //
    // static StepperMotorStub minuteMotor(2048);
    // registry.registerMotor(Slots::Motor::MINUTE_HAND, &minuteMotor);
    //
    // static StepperMotorStub secondMotor(4096);
    // registry.registerMotor(Slots::Motor::SECOND_HAND, &secondMotor);

    // ------------------------------------------------------------------
    // 2. Instantiate and register the display driver.
    // ------------------------------------------------------------------
    //
    // static NeoPixelDisplayStub ring(60, /* dataPin = */ 6);
    // registry.registerDisplay(Slots::Display::MAIN_RING, &ring);

    // ------------------------------------------------------------------
    // 3. Instantiate the time source.
    // ------------------------------------------------------------------
    //
    // static MillisClockCore rtc;
    // rtc.setTime(12, 0, 0);   // optional: set initial time
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

    Serial.println(F("[clocksmith] setup complete."));
    Serial.println(F("[clocksmith] Uncomment hardware blocks in main.cpp to activate."));
}

// ---------------------------------------------------------------------------
// loop() — runs repeatedly
// ---------------------------------------------------------------------------

void loop()
{
    // The golden rule of non-blocking embedded design:
    //   Call update() on EVERY object EVERY loop iteration.
    //   NEVER call delay() anywhere in production code.
    //
    // ClockLogic reads the time and sends targets to the motors:
    // if (clockLogic) clockLogic->update();
    //
    // Each motor must also be ticked so it can step toward its target.
    // (ClockLogic only commands targets; the motor drives itself.)
    //
    // IMotor* h = registry.getMotor(Slots::Motor::HOUR_HAND);
    // if (h) h->update();
    //
    // IMotor* m = registry.getMotor(Slots::Motor::MINUTE_HAND);
    // if (m) m->update();
    //
    // IMotor* s = registry.getMotor(Slots::Motor::SECOND_HAND);
    // if (s) s->update();
}
