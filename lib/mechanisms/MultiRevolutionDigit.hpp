/**
 * @file MultiRevolutionDigit.hpp
 * @brief A digit mechanism where the motor rotates more than one full
 *        revolution to sweep through the complete digit range.
 *
 * ============================================================================
 * WHEN TO USE THIS
 * ============================================================================
 *
 * Use MultiRevolutionDigit when your sculpture uses a mechanism such as:
 *   - A Geneva drive: the output shaft indexes one position per input
 *     revolution, so showing digits 0–9 takes 9 full motor turns.
 *   - An odometer-style digit drum: one complete rotation of the motor
 *     gear train advances the drum by a fraction of a digit, so the full
 *     0–9 sweep spans several revolutions.
 *   - A worm-geared display: very high gear ratio means many motor
 *     revolutions equal one digit step.
 *
 * In all these cases the motor's setTarget() receives a value GREATER than
 * 1.0 — for example, setTarget(4.5) means "go to 4.5 full revolutions from
 * home."  IMotor and the concrete driver (e.g. StepperMotorStub) are designed
 * to support this extended range.
 *
 * ============================================================================
 * POSITION MATHS
 * ============================================================================
 *
 * The mapping from digit value to absolute motor position is:
 *
 *   motorPosition = (digit / digitRange) * revolutionsPerSweep
 *
 * Example: digitRange=9, revolutionsPerSweep=3.0
 *   digit 0  →  (0/9) * 3.0  = 0.000 (home)
 *   digit 3  →  (3/9) * 3.0  = 1.000 (one full revolution)
 *   digit 6  →  (6/9) * 3.0  = 2.000 (two full revolutions)
 *   digit 9  →  (9/9) * 3.0  = 3.000 (three full revolutions)
 *
 * The motor driver converts this absolute float to step counts:
 *   steps = motorPosition * stepsPerRevolution
 *
 * ============================================================================
 * IMPORTANT: THE MOTOR MUST SUPPORT ABSOLUTE POSITIONING FROM HOME
 * ============================================================================
 *
 * The motor must know its "home" position (position 0.0).  On first power-up
 * you typically need a homing routine (move to a limit switch or optical
 * sensor) before calling setDigit().  Without homing, the absolute position
 * is unknown and digit accuracy cannot be guaranteed.
 *
 * ============================================================================
 */

#pragma once

#include <stdint.h>
#include "IDigitMechanism.hpp"
#include "IMotor.hpp"

class MultiRevolutionDigit : public IDigitMechanism
{
public:
    /**
     * @brief Construct a multi-revolution digit mechanism.
     *
     * @param motor                The IMotor driving this digit.
     * @param revolutionsPerSweep  Number of full motor revolutions required
     *                             to sweep from digit 0 to digit digitRange.
     *                             Must be > 0.0.  Example: 3.0 for a mechanism
     *                             that takes three revolutions to show 0–9.
     * @param digitRange           The maximum digit value (default 9).
     */
    explicit MultiRevolutionDigit(IMotor&  motor,
                                   float    revolutionsPerSweep,
                                   uint8_t  digitRange = 9)
        : _motor(motor)
        , _revolutionsPerSweep(revolutionsPerSweep)
        , _digitRange(digitRange)
        , _currentDigit(0)
    {}

    // -----------------------------------------------------------------------
    // IDigitMechanism interface
    // -----------------------------------------------------------------------

    /**
     * @brief Command the mechanism to display the given digit.
     *
     * @param value  Target digit (0–digitRange).  Clamped if out of range.
     *
     * Computes an absolute motor position that may be well above 1.0 and
     * forwards it to the underlying IMotor::setTarget().
     */
    void setDigit(uint8_t value) override
    {
        if (value > _digitRange) value = _digitRange;
        _currentDigit = value;

        // Map digit to an absolute motor position (may be > 1.0).
        // This is the key difference from SingleMotorDigit.
        const float motorPosition = (_digitRange > 0)
            ? (static_cast<float>(value) / static_cast<float>(_digitRange))
              * _revolutionsPerSweep
            : 0.0f;

        _motor.setTarget(motorPosition);
    }

    void    update()      override { _motor.update(); }
    bool    isAtTarget()  const override { return _motor.isAtTarget(); }
    uint8_t getDigit()    const override { return _currentDigit; }

private:
    IMotor& _motor;
    float   _revolutionsPerSweep;
    uint8_t _digitRange;
    uint8_t _currentDigit;
};
