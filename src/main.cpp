/**
 * @file src/main.cpp
 * @brief Framework development stub — NOT a consumer entry point.
 *
 * ============================================================================
 * THIS FILE IS FOR CLOCKSMITH FRAMEWORK DEVELOPMENT ONLY
 * ============================================================================
 *
 * This file exists solely so that `pio run` compiles successfully when working
 * on the clocksmith framework itself.  It contains the bare minimum needed to
 * satisfy the Arduino framework's requirement for setup() and loop().
 *
 * If you are building a kinetic clock sculpture project that USES clocksmith:
 *
 *   → See examples/basic-clock/main.cpp for a fully-annotated consumer
 *     entry point that you can copy into your own sculpture project's
 *     src/main.cpp.
 *
 *   → See docs/using-as-a-library.md for the step-by-step setup guide.
 *
 * DO NOT model your sculpture project's main.cpp on this file.
 *
 * ============================================================================
 */

#include "IClockCore.hpp"
#include "IMotor.hpp"
#include "IDisplay.hpp"
#include "HardwareRegistry.hpp"
#include "ClockLogic.hpp"

HardwareRegistry registry;
IClockCore*      clockCore  = nullptr;
ClockLogic*      clockLogic = nullptr;

void setup()
{
    // Framework dev stub — uncomment driver blocks from examples/basic-clock/main.cpp
    // to activate hardware for development testing.
}

void loop()
{
    if (clockLogic) clockLogic->update();
}

