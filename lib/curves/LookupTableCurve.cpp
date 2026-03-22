/**
 * @file LookupTableCurve.cpp
 * @brief Implementation of LookupTableCurve.
 *
 * Performs piecewise-linear interpolation between adjacent calibration points.
 * The algorithm is deliberately simple and uses no dynamic memory or STL,
 * making it safe on all Arduino-family microcontrollers.
 */

#include "LookupTableCurve.hpp"

LookupTableCurve::LookupTableCurve(const Point* points, uint8_t count)
    : _points(points)
    , _count(count)
{}

float LookupTableCurve::map(float logicalPosition) const
{
    // Guard against an empty or null table.
    if (_points == nullptr || _count == 0) return logicalPosition;

    // Clamp to the table bounds so out-of-range inputs hit the endpoints.
    if (logicalPosition <= _points[0].logical)          return _points[0].physical;
    if (logicalPosition >= _points[_count - 1].logical) return _points[_count - 1].physical;

    // Linear search for the enclosing segment.
    // For small calibration tables (typically 5–20 points) this is fast enough
    // on any MCU.  Replace with binary search if you use very large tables.
    for (uint8_t i = 0; i < static_cast<uint8_t>(_count - 1); ++i)
    {
        const Point& lo = _points[i];
        const Point& hi = _points[i + 1];

        if (logicalPosition >= lo.logical && logicalPosition <= hi.logical)
        {
            // Compute how far we are between the two points (0.0–1.0).
            const float span = hi.logical - lo.logical;
            if (span <= 0.0f) return lo.physical;  // degenerate segment

            const float t = (logicalPosition - lo.logical) / span;

            // Linear interpolation between the two physical positions.
            return lo.physical + t * (hi.physical - lo.physical);
        }
    }

    // Should never reach here if table is well-formed.
    return logicalPosition;
}
