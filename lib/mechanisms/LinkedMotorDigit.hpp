/**
 * @file LinkedMotorDigit.hpp
 * @brief A digit mechanism where TWO motors work in concert to display a
 *        single digit — like two arms of a mechanical linkage pointing to
 *        a location on a dial.
 *
 * ============================================================================
 * WHEN TO USE THIS
 * ============================================================================
 *
 * Use LinkedMotorDigit when your sculpture has a physical mechanism where
 * two independently-driven arms combine to indicate a digit or position.
 * Examples:
 *
 *   - A two-arm clock where both arms must rotate to specific angles so
 *     that a pointer at their intersection indicates a digit.
 *   - A pantograph linkage where the output position is a function of
 *     both input arm angles.
 *   - A differential display where the difference (or sum) of two shaft
 *     positions encodes the displayed value.
 *
 * ============================================================================
 * THE LINKAGE SOLVER PATTERN
 * ============================================================================
 *
 * The relationship between "logical digit position" and "motor A / motor B
 * positions" is unique to each sculpture — LinkedMotorDigit cannot hardcode it.
 *
 * Instead, you provide the maths by subclassing ILinkageSolver and implementing
 * the solve() method.  LinkedMotorDigit calls your solver every time a new
 * digit is commanded, then forwards the results to each motor.
 *
 * This pattern is called the STRATEGY pattern: the algorithm (solve) is
 * separated from the mechanism that uses it (LinkedMotorDigit).  You can
 * swap solvers without changing any other code.
 *
 * ============================================================================
 * EXAMPLE SOLVER — SUM LINKAGE
 * ============================================================================
 *
 *   // A simple linkage where arm A = half the logical position, and
 *   // arm B = the other half (both together span the full range).
 *   class SumLinkageSolver : public ILinkageSolver {
 *   public:
 *       void solve(float logicalPos,
 *                  float& posA, float& posB) const override {
 *           posA = logicalPos * 0.5f;
 *           posB = logicalPos * 0.5f;
 *       }
 *   };
 *
 *   // Real linkage solvers involve trigonometry based on the arm lengths
 *   // and pivot geometry of your specific mechanism.
 *   // See docs/mechanisms.md for a detailed worked example.
 *
 * ============================================================================
 */

#pragma once

#include <stdint.h>
#include "IDigitMechanism.hpp"
#include "IMotor.hpp"

// ----------------------------------------------------------------------------
// ILinkageSolver — strategy interface for two-motor position computation
// ----------------------------------------------------------------------------

/**
 * @brief Abstract strategy that computes per-motor positions for a given
 *        logical digit position.
 *
 * Subclass this and implement solve() with the kinematic equations for your
 * specific linkage geometry.
 */
class ILinkageSolver
{
public:
    virtual ~ILinkageSolver() = default;

    /**
     * @brief Compute the two motor positions required to reach a logical
     *        position.
     *
     * @param logicalPosition  Normalised logical position in [0.0, 1.0] for
     *                         the full digit range (digit / digitRange).
     * @param[out] posA        Target position for motor A (non-negative float).
     * @param[out] posB        Target position for motor B (non-negative float).
     *
     * @note Values > 1.0 are valid for multi-revolution motor positions.
     */
    virtual void solve(float  logicalPosition,
                       float& posA,
                       float& posB) const = 0;
};

// ----------------------------------------------------------------------------
// LinkedMotorDigit
// ----------------------------------------------------------------------------

class LinkedMotorDigit : public IDigitMechanism
{
public:
    /**
     * @brief Construct a linked two-motor digit mechanism.
     *
     * @param motorA      First motor (e.g. the "shoulder" arm).
     * @param motorB      Second motor (e.g. the "elbow" arm).
     * @param solver      ILinkageSolver implementing the kinematic equations
     *                    for your specific linkage.
     * @param digitRange  Maximum digit value (default 9).
     *
     * @note All objects must outlive this LinkedMotorDigit.
     */
    explicit LinkedMotorDigit(IMotor&          motorA,
                               IMotor&          motorB,
                               ILinkageSolver&  solver,
                               uint8_t          digitRange = 9)
        : _motorA(motorA)
        , _motorB(motorB)
        , _solver(solver)
        , _digitRange(digitRange)
        , _currentDigit(0)
    {}

    // -----------------------------------------------------------------------
    // IDigitMechanism interface
    // -----------------------------------------------------------------------

    /**
     * @brief Command both motors to the positions that display the given digit.
     *
     * @param value  Target digit (0–digitRange).  Clamped if out of range.
     *
     * Computes the logical position, calls ILinkageSolver::solve() to obtain
     * per-motor positions, and forwards each to the respective IMotor.
     */
    void setDigit(uint8_t value) override
    {
        if (value > _digitRange) value = _digitRange;
        _currentDigit = value;

        const float logicalPos = (_digitRange > 0)
            ? static_cast<float>(value) / static_cast<float>(_digitRange)
            : 0.0f;

        float posA = 0.0f;
        float posB = 0.0f;
        _solver.solve(logicalPos, posA, posB);

        _motorA.setTarget(posA);
        _motorB.setTarget(posB);
    }

    /**
     * @brief Tick both motors toward their targets.
     */
    void update() override
    {
        _motorA.update();
        _motorB.update();
    }

    /**
     * @brief True only when BOTH motors have reached their targets.
     */
    bool isAtTarget() const override
    {
        return _motorA.isAtTarget() && _motorB.isAtTarget();
    }

    uint8_t getDigit() const override { return _currentDigit; }

private:
    IMotor&         _motorA;
    IMotor&         _motorB;
    ILinkageSolver& _solver;
    uint8_t         _digitRange;
    uint8_t         _currentDigit;
};
