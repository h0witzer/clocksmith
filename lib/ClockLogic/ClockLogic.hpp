/**
 * @file ClockLogic.hpp
 * @brief Translates the current time into motor positions and display states.
 *
 * ============================================================================
 * WHAT THIS CLASS DOES
 * ============================================================================
 *
 * ClockLogic is the "brain" of the system.  Every time update() is called it:
 *
 *   1. Asks IClockCore for the current hours / minutes / seconds.
 *   2. Converts those integers to normalised float positions (0.0–1.0).
 *   3. Commands the registered IMotor instances via setTarget().
 *   4. Passes display update calls through to any registered IDisplay.
 *
 * Crucially, this class deliberately knows NOTHING about stepper motors,
 * NeoPixels, DS3231 RTCs, or any other hardware.  It only speaks the
 * language of the three interfaces:
 *
 *   IClockCore  →  "what time is it?"
 *   IMotor      →  "move to this normalised position"
 *   IDisplay    →  "update your visual state"
 *
 * ============================================================================
 * ADDING NEW BEHAVIOUR
 * ============================================================================
 *
 * If you want new motion styles (e.g. "sweep" vs. "snap"), add a
 * configuration enum/struct and keep all hardware calls behind the interfaces.
 * Do NOT #include any hardware library here.
 *
 * ============================================================================
 */

#pragma once

#include <stdint.h>
#include "IClockCore.hpp"
#include "HardwareRegistry.hpp"

class ClockLogic
{
public:
    /**
     * @brief Construct with a time source and a populated hardware registry.
     *
     * @param clockCore  Reference to the active IClockCore implementation.
     * @param registry   Reference to the HardwareRegistry that holds the
     *                   motors and displays for this build.
     *
     * @note Both objects must outlive ClockLogic.  Declare them at file scope
     *       or as static locals in setup() (see src/main.cpp).
     */
    explicit ClockLogic(IClockCore& clockCore, HardwareRegistry& registry);

    /**
     * @brief Main update tick — call from loop() as fast as possible.
     *
     * Reads the current time, computes normalised target positions, and
     * commands the registered motors.
     *
     * @note Each motor's own update() method must also be called every loop
     *       iteration for movement to actually happen (see src/main.cpp).
     *       ClockLogic only sends targets; the motor drives itself.
     */
    void update();

private:
    IClockCore&       _clockCore;
    HardwareRegistry& _registry;

    /**
     * @brief Convert hours and minutes to a normalised hour-hand position.
     *
     * @param hours    Current hour   (0–23).
     * @param minutes  Current minute (0–59).
     * @return         Position in [0.0, 1.0] on a 12-hour face.
     *
     * The hour hand moves continuously: at 1:30 it sits halfway between the
     * 1 and the 2, not snapped to the 1.
     */
    float toHourPosition(uint8_t hours, uint8_t minutes) const;

    /**
     * @brief Convert minutes and seconds to a normalised minute-hand position.
     *
     * @param minutes  Current minute (0–59).
     * @param seconds  Current second (0–59).
     * @return         Position in [0.0, 1.0].
     *
     * Like the hour hand, the minute hand moves continuously with the seconds.
     */
    float toMinutePosition(uint8_t minutes, uint8_t seconds) const;

    /**
     * @brief Convert seconds to a normalised second-hand position.
     *
     * @param seconds  Current second (0–59).
     * @return         Position in [0.0, 1.0].
     */
    float toSecondPosition(uint8_t seconds) const;
};
