/**
 * @file LinearCurve.hpp
 * @brief IPositionCurve implementation that performs no transformation.
 *
 * ============================================================================
 * WHEN TO USE THIS
 * ============================================================================
 *
 * Use LinearCurve (or pass nullptr where an IPositionCurve is optional) when:
 *   - Your mechanism's output is perfectly proportional to the motor input.
 *   - You are in early prototyping and want to verify movement before
 *     doing a full non-linearity calibration.
 *   - You are writing unit tests and need a predictable no-op curve.
 *
 * LinearCurve is a zero-overhead wrapper: map() is a trivial inline function
 * that the compiler will optimise away entirely.
 *
 * If you discover your mechanism IS non-linear, replace LinearCurve with a
 * LookupTableCurve (see lib/curves/LookupTableCurve.hpp) without changing
 * any other code.
 *
 * ============================================================================
 */

#pragma once

#include "IPositionCurve.hpp"

class LinearCurve : public IPositionCurve
{
public:
    /**
     * @brief Identity mapping — returns logicalPosition unchanged.
     *
     * @param logicalPosition  Any non-negative float.
     * @return The same value that was passed in.
     */
    float map(float logicalPosition) const override
    {
        return logicalPosition;
    }
};
