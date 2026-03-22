/**
 * @file IDigitGroup.hpp
 * @brief Abstract interface for a group of independently-addressable digit
 *        mechanisms (e.g. tens digit + ones digit of the same display unit).
 *
 * ============================================================================
 * WHY THIS EXISTS — A NOTE FOR ARCHITECTURE BEGINNERS
 * ============================================================================
 *
 * Many kinetic clock sculptures present time as individual decimal digits —
 * an hours display, a minutes display, a seconds display — where each display
 * is physically split into a TENS digit and a ONES digit.  For example:
 *
 *   Minutes = 47  →  tens mechanism shows "4",  ones mechanism shows "7"
 *
 * Each mechanism is independent: they might be different sizes, positioned
 * at different locations on the sculpture, and may even use different motor
 * types.
 *
 * Without IDigitGroup, ClockLogic would need to:
 *   1. Extract tens = minutes / 10
 *   2. Extract ones = minutes % 10
 *   3. Look up the tens mechanism in the registry
 *   4. Call setDigit(tens) on it
 *   5. Look up the ones mechanism in the registry
 *   6. Call setDigit(ones) on it
 *
 * With IDigitGroup, the split logic moves INTO the group object, so ClockLogic
 * simply calls:
 *   group.setValue(47)
 *
 * and the group handles the rest.  If your sculpture has three digits instead
 * of two (e.g. seconds display 0–59 with an extra hundreds digit for artistic
 * reasons), you just register more mechanisms — ClockLogic doesn't change.
 *
 * ============================================================================
 */

#pragma once

#include <stdint.h>

class IDigitGroup
{
public:
    virtual ~IDigitGroup() = default;

    /**
     * @brief Set the full integer value to be displayed.
     *
     * The group splits @p value into individual decimal digits and forwards
     * each to the appropriate registered mechanism.
     *
     * @param value  Integer value to display (e.g. 47 for 47 minutes).
     *               - For a 2-digit group: valid range is 0–99.
     *               - For hours in 24-hour mode: valid range is 0–23.
     *               - Out-of-range values are handled by each digit mechanism
     *                 clamping to its own maximum.
     *
     * @note This call is NON-BLOCKING.  Actual movement happens in update().
     */
    virtual void setValue(uint8_t value) = 0;

    /**
     * @brief Advance all digit mechanisms in the group by one tick.
     *
     * Call every iteration of loop().  Must be completely non-blocking.
     */
    virtual void update() = 0;

    /**
     * @brief Query whether every digit mechanism in the group has reached
     *        its target.
     *
     * @return true  when ALL digits are at their commanded positions.
     * @return false if ANY digit is still moving.
     */
    virtual bool isAtTarget() const = 0;
};
