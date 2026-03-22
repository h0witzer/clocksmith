/**
 * @file IDigitMechanism.hpp
 * @brief Abstract interface for a single-digit kinetic display mechanism.
 *
 * ============================================================================
 * WHY THIS EXISTS — A NOTE FOR ARCHITECTURE BEGINNERS
 * ============================================================================
 *
 * The existing IMotor interface is perfect for analogue clock hands: it thinks
 * in normalised shaft positions (0.0–1.0) and doesn't care what the hand is
 * displaying.
 *
 * But many kinetic sculptures show DIGITS — numerals 0 through 9 driven by
 * physical mechanisms.  Each digit mechanism has its own quirks:
 *
 *   - A simple single motor that linearly sweeps through 0–9
 *   - A geneva-drive mechanism that needs the motor to rotate multiple
 *     full revolutions to advance one digit
 *   - Two arms of a mechanical linkage that together point to a digit
 *
 * Rather than forcing ClockLogic to know which type is attached, IDigitMechanism
 * defines a uniform language: "show digit N".  Each concrete mechanism
 * translates that command into whatever motor movements are required.
 *
 * ============================================================================
 * DIGIT RANGE
 * ============================================================================
 *
 * The standard range is 0–9 (decimal digits), but mechanisms can support any
 * range from 0 to 9.  The caller (ClockLogic) extracts individual decimal
 * digits from H/M/S values before calling setDigit():
 *
 *   minutes = 47  →  tens digit = 4,  ones digit = 7
 *   setDigit(4) on the tens mechanism
 *   setDigit(7) on the ones mechanism
 *
 * ============================================================================
 */

#pragma once

#include <stdint.h>

class IDigitMechanism
{
public:
    virtual ~IDigitMechanism() = default;

    /**
     * @brief Command the mechanism to move to the given digit position.
     *
     * @param value  Target digit value.  The valid range is 0 to 9 for
     *               standard decimal display.  Out-of-range values must be
     *               clamped silently by the concrete implementation.
     *
     * @note This call is NON-BLOCKING.  It records the target; actual
     *       movement happens inside update().
     */
    virtual void setDigit(uint8_t value) = 0;

    /**
     * @brief Advance the mechanism one tick toward its commanded digit.
     *
     * Call this every iteration of loop().  Must be completely non-blocking.
     */
    virtual void update() = 0;

    /**
     * @brief Query whether the mechanism has reached its commanded digit.
     *
     * @return true  when all underlying motors are at their target positions.
     * @return false if the mechanism is still moving.
     */
    virtual bool isAtTarget() const = 0;

    /**
     * @brief Return the digit value most recently commanded via setDigit().
     *
     * @return The digit value last passed to setDigit().
     */
    virtual uint8_t getDigit() const = 0;
};
