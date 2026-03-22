/**
 * @file LookupTableCurve.hpp
 * @brief IPositionCurve implementation that corrects non-linear mechanisms
 *        using a piecewise-linear calibration table.
 *
 * ============================================================================
 * WHEN TO USE THIS
 * ============================================================================
 *
 * Use LookupTableCurve when your mechanism has a non-linear relationship
 * between the motor position and the displayed value.  Common causes:
 *
 *   - A cam-driven hand: moves fast in the middle, slow at the extremes.
 *   - A non-circular gear or elliptical pulley: sinusoidal speed variation.
 *   - A linkage with a non-centred pivot: one-sided compression/expansion.
 *   - Any mechanism where you have measured the real outputs at a set of
 *     known input positions.
 *
 * ============================================================================
 * HOW TO CALIBRATE — STEP BY STEP
 * ============================================================================
 *
 *  1. Command the motor to a known position (e.g. 0.0, 0.1, 0.2, … 1.0).
 *  2. Measure or observe what the mechanism ACTUALLY displays at each step.
 *     Convert that to a normalised "logical position" (e.g. if your display
 *     shows the 3rd digit out of 9, logical = 3/9 ≈ 0.333).
 *  3. Fill in a Point array with pairs of {logical, physical} coordinates.
 *  4. Construct a LookupTableCurve from the array.
 *
 * The curve then interpolates linearly between adjacent calibration points
 * at runtime to produce smooth, accurate corrections across the full range.
 *
 * ============================================================================
 * MEMORY MODEL
 * ============================================================================
 *
 * LookupTableCurve stores a POINTER to the Point array — it does NOT copy
 * the data.  This avoids RAM duplication on memory-constrained MCUs.
 * The Point array must outlive the LookupTableCurve instance.
 *
 * Typical usage (in main.cpp or a driver file):
 *
 *   static const LookupTableCurve::Point kMyCurve[] = {
 *       {0.00f, 0.00f},
 *       {0.25f, 0.31f},   // mechanism reaches "25%" point at motor 31%
 *       {0.50f, 0.55f},
 *       {0.75f, 0.72f},
 *       {1.00f, 1.00f},
 *   };
 *   static LookupTableCurve myCurve(kMyCurve, 5);
 *
 * ============================================================================
 */

#pragma once

#include <stdint.h>
#include "IPositionCurve.hpp"

class LookupTableCurve : public IPositionCurve
{
public:
    /**
     * @brief A single calibration point mapping logical → physical position.
     */
    struct Point
    {
        float logical;   ///< Desired displayed position in [0.0, 1.0].
        float physical;  ///< Motor position required to produce that display.
    };

    /**
     * @brief Construct from a user-supplied calibration table.
     *
     * @param points  Pointer to an array of calibration points.
     *                Points MUST be sorted in ascending order of `logical`.
     *                The array must start at logical=0.0 and end at logical=1.0
     *                for full-range coverage.
     * @param count   Number of points in the array.  Minimum 2 (start + end).
     *
     * @note The array must outlive this LookupTableCurve.  Declare it as
     *       `static const` to ensure correct lifetime (see header comment).
     */
    LookupTableCurve(const Point* points, uint8_t count);

    /**
     * @brief Map a logical position to a physical motor position using
     *        piecewise-linear interpolation.
     *
     * @param logicalPosition  Desired displayed position in [0.0, 1.0].
     *                         Values outside [0.0, 1.0] are clamped to the
     *                         nearest endpoint of the table.
     *
     * @return The physical motor position that produces the desired display.
     */
    float map(float logicalPosition) const override;

private:
    const Point* _points;
    uint8_t      _count;
};
