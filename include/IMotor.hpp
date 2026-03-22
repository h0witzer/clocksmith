/**
 * @file IMotor.hpp
 * @brief Abstract interface for a single motor that controls one clock hand.
 *
 * ============================================================================
 * WHY THIS EXISTS — A NOTE FOR ARCHITECTURE BEGINNERS
 * ============================================================================
 *
 * In a traditional Arduino sketch you might write something like this directly
 * inside loop():
 *
 *   stepper.moveTo(512);   // 512 steps = 12 o'clock position
 *   stepper.run();
 *
 * That tightly "couples" (glues together) your hardware and your logic.
 * If you later want to swap the stepper for a servo motor, you must rewrite
 * every line of code that mentions steps or AccelStepper.
 *
 * An INTERFACE (or Abstract Base Class) is a *contract*: it defines what
 * operations a class must support, without saying anything about how those
 * operations are actually performed.
 *
 * IMotor says:
 *   "I don't care whether you are a stepper motor, a servo, a DC motor with
 *    an encoder, or something we haven't invented yet — as long as you can do
 *    these four things, ClockLogic can drive you."
 *
 * ============================================================================
 * NORMALIZED POSITIONS
 * ============================================================================
 *
 * Positions are expressed as a non-negative float where 1.0 represents one
 * full revolution of the output shaft:
 *
 *   Standard analogue clock hand:
 *     0.0  = 12 o'clock  (zero / home reference)
 *     0.25 = 3 o'clock
 *     0.5  = 6 o'clock
 *     0.75 = 9 o'clock
 *
 *   Multi-revolution mechanisms (e.g. geneva drives, odometer digits):
 *     Values greater than 1.0 are explicitly supported and represent
 *     additional full revolutions.  For example, 3.5 means "three and a
 *     half full revolutions from the home position."  Concrete motor drivers
 *     must NOT clamp the upper bound to 1.0; they should convert the full
 *     float to an absolute step or encoder count.
 *
 * This convention frees ClockLogic and the mechanism handlers from ever
 * knowing about steps, degrees, microseconds of PWM pulse width, or encoder
 * ticks. The concrete driver is responsible for translating the float into
 * whatever its hardware understands.
 *
 * ============================================================================
 */

#pragma once

class IMotor
{
public:
    virtual ~IMotor() = default;

    /**
     * @brief Command the motor to move to a normalised position.
     *
     * @param position  Target position as a non-negative float.
     *                  For standard analogue hands this is in [0.0, 1.0].
     *                  For multi-revolution mechanisms (e.g. MultiRevolutionDigit)
     *                  values greater than 1.0 are valid and represent additional
     *                  full revolutions beyond the home position.
     *                  Negative values must be clamped to 0.0 by the
     *                  concrete implementation.
     *
     * @note This call is NON-BLOCKING.  It records the target; the actual
     *       movement happens incrementally inside update().  Never block
     *       inside this method.
     */
    virtual void setTarget(float position) = 0;

    /**
     * @brief Advance the motor one "tick" toward its target position.
     *
     * Call this as often as possible — ideally every iteration of loop().
     *
     * @warning Never call delay() inside an implementation of this method.
     *          Doing so would freeze the entire system (display updates,
     *          RTC reads, serial comms) for the duration of the delay.
     *          Use millis()-based timing instead.
     */
    virtual void update() = 0;

    /**
     * @brief Query whether the motor has reached its commanded target.
     *
     * @return true  if the current position equals the target within a
     *               small tolerance defined by the implementation.
     * @return false if the motor is still moving.
     */
    virtual bool isAtTarget() const = 0;

    /**
     * @brief Return the current normalised position of the motor shaft.
     *
     * @return Current position as a non-negative float.  For single-revolution
     *         use this is in [0.0, 1.0].  For multi-revolution use it may
     *         be greater than 1.0.
     */
    virtual float getPosition() const = 0;
};
