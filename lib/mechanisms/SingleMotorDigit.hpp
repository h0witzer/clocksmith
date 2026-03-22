/**
 * @file SingleMotorDigit.hpp
 * @brief A digit mechanism driven by one motor, with optional non-linear
 *        position correction via IPositionCurve.
 *
 * ============================================================================
 * WHEN TO USE THIS
 * ============================================================================
 *
 * Use SingleMotorDigit when:
 *   - One motor directly drives one digit display.
 *   - The digit sweeps linearly from 0 to 9 within one motor revolution.
 *   - You optionally need to correct for a non-linear output mechanism
 *     (e.g. a cam, a non-circular gear, or mounting offset).
 *
 * SingleMotorDigit is the simplest possible digit mechanism.  If your
 * sculpture is more complex, see:
 *   - MultiRevolutionDigit — for mechanisms requiring more than one
 *     motor revolution to sweep through 0–9.
 *   - LinkedMotorDigit — for mechanisms where two motors together
 *     point to a digit.
 *
 * ============================================================================
 * POSITION MATHS
 * ============================================================================
 *
 * The logical digit-to-position mapping is:
 *
 *   logicalPosition = digit / digitRange          (e.g. 7/9 ≈ 0.778)
 *   physicalPosition = curve.map(logicalPosition) (≡ logicalPos if linear)
 *   motor.setTarget(physicalPosition)
 *
 * With the default LinearCurve (or nullptr), physicalPosition == logicalPosition.
 * With a LookupTableCurve you can correct for any mechanism non-linearity.
 *
 * ============================================================================
 */

#pragma once

#include <stdint.h>
#include "IDigitMechanism.hpp"
#include "IMotor.hpp"
#include "IPositionCurve.hpp"

class SingleMotorDigit : public IDigitMechanism
{
public:
    /**
     * @brief Construct a single-motor digit mechanism.
     *
     * @param motor       The IMotor that physically drives this digit.
     * @param digitRange  The maximum digit value (default 9, giving digits 0–9).
     *                    Use a lower value if your mechanism only shows a
     *                    partial range (e.g. 5 for the tens-of-minutes digit).
     * @param curve       Optional non-linear position curve.  Pass nullptr
     *                    (the default) for a linear response.
     *
     * @note All objects must outlive this SingleMotorDigit.  Declare them as
     *       static locals in setup() (see src/main.cpp).
     */
    explicit SingleMotorDigit(IMotor&         motor,
                               uint8_t         digitRange = 9,
                               IPositionCurve* curve      = nullptr)
        : _motor(motor)
        , _curve(curve)
        , _digitRange(digitRange)
        , _currentDigit(0)
    {}

    // -----------------------------------------------------------------------
    // IDigitMechanism interface
    // -----------------------------------------------------------------------

    /**
     * @brief Command the digit to display the given value.
     *
     * @param value  Target digit (0–digitRange).  Values above digitRange
     *               are clamped to digitRange.
     */
    void setDigit(uint8_t value) override
    {
        if (value > _digitRange) value = _digitRange;
        _currentDigit = value;

        // Map the digit to a logical position in [0.0, 1.0].
        const float logical = (_digitRange > 0)
            ? static_cast<float>(value) / static_cast<float>(_digitRange)
            : 0.0f;

        // Apply the position curve (if any) to correct for non-linearity.
        const float physical = (_curve != nullptr) ? _curve->map(logical) : logical;

        _motor.setTarget(physical);
    }

    void    update()      override { _motor.update(); }
    bool    isAtTarget()  const override { return _motor.isAtTarget(); }
    uint8_t getDigit()    const override { return _currentDigit; }

private:
    IMotor&         _motor;
    IPositionCurve* _curve;       // may be nullptr
    uint8_t         _digitRange;
    uint8_t         _currentDigit;
};
