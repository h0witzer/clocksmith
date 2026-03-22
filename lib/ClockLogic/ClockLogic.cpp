/**
 * @file ClockLogic.cpp
 * @brief Implementation of ClockLogic.
 *
 * Each method body is deliberately minimal and commented so you can follow
 * the data flow without needing any hardware attached.  The arithmetic below
 * is the only non-trivial logic in this file — everything else is a
 * straight pass-through to an interface.
 */

#include "ClockLogic.hpp"

// ----------------------------------------------------------------------------
// Constructor
// ----------------------------------------------------------------------------

ClockLogic::ClockLogic(IClockCore& clockCore, HardwareRegistry& registry)
    : _clockCore(clockCore)
    , _registry(registry)
{
    // Nothing to initialise here.
    // Hardware initialisation belongs in the concrete driver constructors
    // and in setup() (main.cpp), NOT in this constructor.
}

// ----------------------------------------------------------------------------
// Public: update
// ----------------------------------------------------------------------------

void ClockLogic::update()
{
    // Step 1 — Refresh the time source.
    // This may perform an I2C read, recalculate a millis() counter, or
    // issue an NTP poll.  We don't care which; we just call the interface.
    _clockCore.update();

    // Step 2 — Fetch the current H/M/S.
    uint8_t hours   = 0;
    uint8_t minutes = 0;
    uint8_t seconds = 0;
    _clockCore.getTime(hours, minutes, seconds);

    // -----------------------------------------------------------------------
    // Step 3a — ANALOGUE PATH: compute normalised positions and command motors.
    //
    // getMotor() returns nullptr when no motor was registered for a slot.
    // The null-checks below make ClockLogic safe even when running without
    // hardware (e.g. in a host-side unit test or on a board that only has
    // two hands, not three).
    // -----------------------------------------------------------------------
    if (IMotor* hour = _registry.getMotor(Slots::Motor::HOUR_HAND))
    {
        hour->setTarget(toHourPosition(hours, minutes));
    }
    if (IMotor* minute = _registry.getMotor(Slots::Motor::MINUTE_HAND))
    {
        minute->setTarget(toMinutePosition(minutes, seconds));
    }
    if (IMotor* second = _registry.getMotor(Slots::Motor::SECOND_HAND))
    {
        second->setTarget(toSecondPosition(seconds));
    }

    // -----------------------------------------------------------------------
    // Step 3b — DIGIT PATH: drive digit mechanisms and groups.
    //
    // updateDigits() checks for a DigitGroup first (which handles tens/ones
    // splitting internally), then falls back to individually registered
    // digit mechanisms.  Both can be registered simultaneously if desired.
    //
    // Hours are passed as raw 24-hour values (0–23).  If your display shows
    // 12-hour time, apply the conversion inside your digit mechanism or group.
    // -----------------------------------------------------------------------
    updateDigits(Slots::DigitGroup::HOURS,
                 Slots::Digit::HOURS_TENS,
                 Slots::Digit::HOURS_ONES,
                 hours);

    updateDigits(Slots::DigitGroup::MINUTES,
                 Slots::Digit::MINUTES_TENS,
                 Slots::Digit::MINUTES_ONES,
                 minutes);

    updateDigits(Slots::DigitGroup::SECONDS,
                 Slots::Digit::SECONDS_TENS,
                 Slots::Digit::SECONDS_ONES,
                 seconds);

    // -----------------------------------------------------------------------
    // Step 4 — Update the display (if one is registered).
    // -----------------------------------------------------------------------
    if (IDisplay* display = _registry.getDisplay(Slots::Display::MAIN_RING))
    {
        display->update();
    }
}

// ----------------------------------------------------------------------------
// Private: position calculations
// ----------------------------------------------------------------------------

float ClockLogic::toHourPosition(uint8_t hours, uint8_t minutes) const
{
    // A 12-hour clock face: the hand completes one revolution every 12 hours.
    // We interpolate with minutes so the hand moves continuously rather than
    // snapping once per hour.
    //
    // Example: 3:30  →  (3 + 30/60) / 12  =  3.5 / 12  ≈  0.292
    const float h12 = static_cast<float>(hours % 12);
    const float m   = static_cast<float>(minutes);
    return (h12 + m / 60.0f) / 12.0f;
}

float ClockLogic::toMinutePosition(uint8_t minutes, uint8_t seconds) const
{
    // One full revolution every 60 minutes; interpolate seconds for smooth
    // continuous motion.
    //
    // Example: 45:30  →  (45 + 30/60) / 60  =  45.5 / 60  ≈  0.758
    const float m = static_cast<float>(minutes);
    const float s = static_cast<float>(seconds);
    return (m + s / 60.0f) / 60.0f;
}

float ClockLogic::toSecondPosition(uint8_t seconds) const
{
    // One full revolution every 60 seconds.
    //
    // Example: 30s  →  30 / 60  =  0.5  (6 o'clock)
    return static_cast<float>(seconds) / 60.0f;
}

// ----------------------------------------------------------------------------
// Private: digit dispatch
// ----------------------------------------------------------------------------

void ClockLogic::updateDigits(const char* groupSlot,
                               const char* tensSlot,
                               const char* onesSlot,
                               uint8_t     value)
{
    // Check for a digit group first.  A DigitGroup manages the tens/ones
    // split internally, so one call does everything.
    if (IDigitGroup* group = _registry.getDigitGroup(groupSlot))
    {
        group->setValue(value);
    }

    // Independently check for individual digit mechanisms.  This allows a
    // project to register BOTH a group AND individual mechanisms for the same
    // time unit (e.g. the group drives the physical display, and an individual
    // mechanism drives a separate indicator LED ring).
    if (IDigitMechanism* tens = _registry.getDigitMechanism(tensSlot))
    {
        tens->setDigit(value / 10);
    }
    if (IDigitMechanism* ones = _registry.getDigitMechanism(onesSlot))
    {
        ones->setDigit(value % 10);
    }
}
