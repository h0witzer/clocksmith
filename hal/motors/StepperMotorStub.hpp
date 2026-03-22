/**
 * @file StepperMotorStub.hpp
 * @brief STUB / TEMPLATE for a stepper-motor IMotor implementation.
 *
 * ============================================================================
 * THIS FILE IS A STARTING TEMPLATE — IT IS NOT FUNCTIONAL
 * ============================================================================
 *
 * It exists to show you exactly which methods you need to fill in and why.
 * Every method body that needs real code is marked with a prominent TODO.
 *
 * HOW TO USE THIS FILE
 * --------------------
 *  1. Copy this file and rename it to match your driver
 *     (e.g. hal/motors/A4988StepperMotor.hpp).
 *  2. Add the library for your stepper driver to platformio.ini:
 *       lib_deps = waspinator/AccelStepper
 *  3. Fill in each TODO section.
 *  4. #include your new file in src/main.cpp.
 *  5. Instantiate and register it (see src/main.cpp for the pattern).
 *
 * For a full walkthrough, read docs/adding-a-motor-driver.md.
 *
 * ============================================================================
 * NORMALISED POSITIONS
 * ============================================================================
 *
 * The IMotor interface works in normalised floats [0.0, 1.0].
 * This driver converts them to an absolute step count using:
 *
 *   targetStep = round(position * totalSteps)
 *
 * totalSteps is the number of steps in one full revolution of the OUTPUT
 * shaft (not the motor shaft).  For a 28BYJ-48 in half-step mode this is
 * 4096.  For a NEMA 17 at 1/16 microstepping with a 1:1 ratio this is
 * 3200 (200 full steps × 16).
 *
 * ============================================================================
 */

#pragma once

#include <stdint.h>
#include "IMotor.hpp"

// TODO: Include the header for your chosen stepper library, e.g.:
// #include <AccelStepper.h>

class StepperMotorStub : public IMotor
{
public:
    /**
     * @brief Constructor — hardware pin setup and library initialisation.
     *
     * @param totalSteps  Full-revolution step count for your motor/gearbox
     *                    combination (e.g. 2048 for a 28BYJ-48 in half-step
     *                    mode, or 4096 in full-step mode with a 64:1 gearbox).
     *
     * TODO: Add pin parameters appropriate for your driver board, e.g.:
     *         StepperMotorStub(int totalSteps,
     *                          uint8_t pin1, uint8_t pin2,
     *                          uint8_t pin3, uint8_t pin4)
     */
    explicit StepperMotorStub(int totalSteps)
        : _totalSteps(totalSteps)
        , _targetStep(0)
        , _currentStep(0)
    {
        // TODO: Configure the hardware and initialise your motor library.
        //
        // AccelStepper example:
        //   _stepper = AccelStepper(AccelStepper::FULL4WIRE,
        //                           pin1, pin3, pin2, pin4);
        //   _stepper.setMaxSpeed(1000.0f);
        //   _stepper.setAcceleration(500.0f);
    }

    // -----------------------------------------------------------------------
    // IMotor interface implementation
    // -----------------------------------------------------------------------

    /**
     * @brief Command the motor to move to a normalised position [0.0, 1.0].
     *
     * Converts the float to an absolute step count and passes it to the
     * motor library.  The actual movement happens inside update().
     */
    void setTarget(float position) override
    {
        // Clamp negative values — a motor cannot go "before" its home position.
        // We do NOT clamp the upper limit: values > 1.0 represent multi-revolution
        // positions used by MultiRevolutionDigit and similar handlers.
        // For example, setTarget(3.5) means "3.5 full revolutions from home."
        if (position < 0.0f) position = 0.0f;

        // Convert the normalised position to an absolute step index.
        // For position > 1.0 this produces a step count beyond one revolution,
        // which AccelStepper handles correctly as an absolute step target.
        _targetStep = static_cast<int>(
            position * static_cast<float>(_totalSteps));

        // TODO: Pass the target to your motor library, e.g.:
        //   _stepper.moveTo(static_cast<long>(_targetStep));
    }

    /**
     * @brief Advance the motor one micro-step toward the target.
     *
     * This is the non-blocking heartbeat of the driver.  Call it every
     * loop() iteration.  AccelStepper::run() is already non-blocking;
     * other libraries may require you to compute a step manually here.
     */
    void update() override
    {
        // TODO: Advance one step toward the target, e.g.:
        //   _stepper.run();

        // TODO: Sync the internal position counter with the library, e.g.:
        //   _currentStep = static_cast<int>(_stepper.currentPosition());
    }

    /**
     * @brief Return true when the motor shaft is at the commanded target.
     */
    bool isAtTarget() const override
    {
        return _currentStep == _targetStep;

        // TODO (AccelStepper): return (_stepper.distanceToGo() == 0);
    }

    /**
     * @brief Return the current normalised position [0.0, 1.0].
     */
    float getPosition() const override
    {
        if (_totalSteps == 0) return 0.0f;
        return static_cast<float>(_currentStep)
             / static_cast<float>(_totalSteps);
    }

private:
    int _totalSteps;
    int _targetStep;
    int _currentStep;

    // TODO: Declare your motor library instance here, e.g.:
    // AccelStepper _stepper;
};
