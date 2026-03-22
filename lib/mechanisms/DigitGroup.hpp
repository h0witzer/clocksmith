/**
 * @file DigitGroup.hpp
 * @brief Groups independent digit mechanisms (e.g. tens and ones) for a
 *        single time unit so ClockLogic can drive them with one call.
 *
 * ============================================================================
 * WHEN TO USE THIS
 * ============================================================================
 *
 * Many kinetic clock sculptures display each time unit as two independent
 * physical digit mechanisms — a tens digit and a ones digit:
 *
 *   Hours   display: [H_tens][H_ones]   e.g. "1" | "4"  for 14:xx
 *   Minutes display: [M_tens][M_ones]   e.g. "4" | "7"  for xx:47
 *   Seconds display: [S_tens][S_ones]   e.g. "0" | "9"  for xx:xx:09
 *
 * The tens and ones mechanisms are physically separate (different motors,
 * different positions on the sculpture) but logically they represent one
 * time value.
 *
 * DigitGroup takes a full integer value (e.g. 47) and automatically:
 *   - Extracts the tens digit (4) and forwards it to the tens mechanism.
 *   - Extracts the ones digit (7) and forwards it to the ones mechanism.
 *
 * This means ClockLogic only needs ONE call per time unit, and each
 * IDigitMechanism inside the group can be any type (SingleMotorDigit,
 * MultiRevolutionDigit, LinkedMotorDigit, or a future custom type).
 *
 * ============================================================================
 * DIGIT POSITIONS EXPLAINED
 * ============================================================================
 *
 * Digit positions are numbered from the RIGHT (least significant first):
 *   position 0 = ones digit  (rightmost)
 *   position 1 = tens digit
 *   position 2 = hundreds digit (rarely needed for time display)
 *
 * Example: setValue(47) with two registered mechanisms:
 *   mechanism[0] (ones): setDigit(7)
 *   mechanism[1] (tens): setDigit(4)
 *
 * ============================================================================
 * MEMORY MODEL
 * ============================================================================
 *
 * Like HardwareRegistry, DigitGroup uses a fixed-capacity array.
 * MAX_DIGITS is 4 by default (supports up to 4-digit groups).
 * Increase this if your sculpture needs more.
 *
 * ============================================================================
 */

#pragma once

#include <stdint.h>
#include "IDigitGroup.hpp"
#include "IDigitMechanism.hpp"

class DigitGroup : public IDigitGroup
{
public:
    static constexpr uint8_t MAX_DIGITS = 4;

    DigitGroup() = default;

    /**
     * @brief Register an IDigitMechanism for a digit position.
     *
     * @param position   Digit position: 0 = ones, 1 = tens, 2 = hundreds.
     *                   Values >= MAX_DIGITS are silently ignored.
     * @param mechanism  Pointer to the IDigitMechanism for this position.
     *                   May be any concrete type (SingleMotorDigit,
     *                   MultiRevolutionDigit, LinkedMotorDigit, etc.).
     *
     * @note The group does NOT own the pointer.  The mechanism must outlive
     *       this DigitGroup.  Declare it as a static local in setup().
     */
    void setMechanism(uint8_t position, IDigitMechanism* mechanism)
    {
        if (position >= MAX_DIGITS) return;
        _mechanisms[position] = mechanism;
        if (position >= _count) _count = static_cast<uint8_t>(position + 1);
    }

    // -----------------------------------------------------------------------
    // IDigitGroup interface
    // -----------------------------------------------------------------------

    /**
     * @brief Set the full integer value to be displayed.
     *
     * Extracts individual decimal digits and forwards each to the
     * corresponding registered IDigitMechanism.
     *
     * @param value  The integer to display (e.g. 47 for 47 minutes).
     *               - Ones digit:     value % 10
     *               - Tens digit:     (value / 10) % 10
     *               - Hundreds digit: (value / 100) % 10
     *
     * Digit positions with no registered mechanism are silently skipped.
     */
    void setValue(uint8_t value) override
    {
        // Extract digits from least significant to most significant.
        uint8_t remaining = value;
        for (uint8_t pos = 0; pos < MAX_DIGITS; ++pos)
        {
            if (_mechanisms[pos] != nullptr)
            {
                _mechanisms[pos]->setDigit(remaining % 10);
            }
            remaining /= 10;
        }
    }

    /**
     * @brief Tick every registered digit mechanism.
     *
     * Call every loop() iteration for all digits to animate toward their
     * targets simultaneously.
     */
    void update() override
    {
        for (uint8_t pos = 0; pos < MAX_DIGITS; ++pos)
        {
            if (_mechanisms[pos] != nullptr)
            {
                _mechanisms[pos]->update();
            }
        }
    }

    /**
     * @brief True only when ALL registered digit mechanisms are at their
     *        commanded targets.
     */
    bool isAtTarget() const override
    {
        for (uint8_t pos = 0; pos < MAX_DIGITS; ++pos)
        {
            if (_mechanisms[pos] != nullptr && !_mechanisms[pos]->isAtTarget())
            {
                return false;
            }
        }
        return true;
    }

private:
    IDigitMechanism* _mechanisms[MAX_DIGITS] = {};
    uint8_t          _count                  = 0;
};
