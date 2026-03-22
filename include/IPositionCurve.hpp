/**
 * @file IPositionCurve.hpp
 * @brief Abstract interface for mapping a logical position to a physical
 *        motor position, allowing for non-linear mechanism correction.
 *
 * ============================================================================
 * WHY THIS EXISTS — A NOTE FOR ARCHITECTURE BEGINNERS
 * ============================================================================
 *
 * In an ideal world, if you want a hand to point at the "3" on a clock face,
 * you command the motor to 25% of a revolution (0.25) and it arrives exactly
 * there.  In the real world, mechanisms introduce non-linearity:
 *
 *   - A cam-driven display arm moves quickly near the centre and slowly
 *     near the ends, so the same number of motor steps doesn't produce the
 *     same visible movement everywhere.
 *   - A gearbox with slight backlash produces a dead zone near reversal.
 *   - A hand linked through a bell-crank produces a sinusoidal position
 *     error that varies through the rotation.
 *
 * IPositionCurve defines a simple contract: given a logical position (the
 * value ClockLogic WANTS to display), return the physical motor position (the
 * value the motor NEEDS to reach) that produces the correct visible result.
 *
 * ============================================================================
 * CALIBRATION WORKFLOW
 * ============================================================================
 *
 *  1. Drive the motor to several known positions and record what the
 *     mechanism actually displays (a few points spread across the range).
 *  2. Build a LookupTableCurve from those calibration pairs.
 *  3. Pass the curve into your digit mechanism (e.g. SingleMotorDigit).
 *  4. The mechanism will now hit every digit accurately despite the
 *     non-linearity.
 *
 * If your mechanism IS linear, use LinearCurve — it is a zero-cost identity
 * mapping and the compiler will inline it away.
 *
 * ============================================================================
 */

#pragma once

class IPositionCurve
{
public:
    virtual ~IPositionCurve() = default;

    /**
     * @brief Map a logical position to a physical motor position.
     *
     * @param logicalPosition  The desired displayed position, in [0.0, 1.0].
     *                         0.0 = display start, 1.0 = display end.
     *
     * @return The physical motor position needed to produce the desired
     *         displayed position.  For linear mechanisms this equals
     *         logicalPosition.  For non-linear mechanisms it may be higher
     *         or lower.  The return value MAY exceed 1.0 for multi-revolution
     *         mechanisms — the motor driver must handle this.
     */
    virtual float map(float logicalPosition) const = 0;
};
