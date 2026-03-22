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
 *   2. For analogue hands: converts H/M/S to normalised [0.0, 1.0] positions
 *      and commands the registered IMotor instances via setTarget().
 *   3. For digit displays: extracts individual decimal digits and commands
 *      registered IDigitMechanism or IDigitGroup instances via setDigit() /
 *      setValue().
 *   4. Passes display update calls through to any registered IDisplay.
 *
 * Crucially, this class deliberately knows NOTHING about stepper motors,
 * NeoPixels, DS3231 RTCs, or any other hardware.  It only speaks the
 * language of the interfaces:
 *
 *   IClockCore         →  "what time is it?"
 *   IMotor             →  "move to this normalised position"
 *   IDigitMechanism    →  "show this digit"
 *   IDigitGroup        →  "show this full number (group handles tens/ones)"
 *   IDisplay           →  "update your visual state"
 *
 * ============================================================================
 * ANALOGUE vs. DIGIT MODE
 * ============================================================================
 *
 * Both modes are active simultaneously.  You can mix and match:
 *   - Register only IMotors for a pure analogue sculpture.
 *   - Register only IDigitGroups for a pure digit sculpture.
 *   - Register both for a sculpture that has analogue hands AND digit displays.
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
     * Reads the current time, then:
     *   - Commands analogue motors (if any IMotors are registered).
     *   - Commands digit mechanisms and groups (if any are registered).
     *   - Ticks any registered IDisplay.
     *
     * @note Each motor's own update() method must also be called every loop
     *       iteration for movement to actually happen (see src/main.cpp).
     *       ClockLogic only sends targets; the motor drives itself.
     */
    void update();

private:
    IClockCore&       _clockCore;
    HardwareRegistry& _registry;

    // -----------------------------------------------------------------------
    // Analogue position calculations
    // -----------------------------------------------------------------------

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

    // -----------------------------------------------------------------------
    // Digit dispatch helpers
    // -----------------------------------------------------------------------

    /**
     * @brief Drive all registered digit groups and individual digit mechanisms
     *        for one time unit.
     *
     * @param groupSlot  Slot name for the IDigitGroup (e.g. Slots::DigitGroup::HOURS).
     * @param tensSlot   Slot name for the tens IDigitMechanism.
     * @param onesSlot   Slot name for the ones IDigitMechanism.
     * @param value      The full integer time value (0–59 or 0–23).
     *
     * If a group is registered under groupSlot, it receives setValue(value).
     * Otherwise, individual mechanisms under tensSlot / onesSlot receive
     * setDigit() with the extracted tens and ones digits respectively.
     */
    void updateDigits(const char* groupSlot,
                      const char* tensSlot,
                      const char* onesSlot,
                      uint8_t     value);
};
