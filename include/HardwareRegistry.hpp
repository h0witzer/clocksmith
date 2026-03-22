/**
 * @file HardwareRegistry.hpp
 * @brief Registry / factory that maps named hardware "slots" to concrete
 *        interface implementations.
 *
 * ============================================================================
 * THE REGISTRY PATTERN — WHY WE NEED IT
 * ============================================================================
 *
 * At this point you might ask: "I have my StepperMotor class and I have
 * ClockLogic — why can't I just pass the StepperMotor directly to
 * ClockLogic?"
 *
 * You could — but it would force you to include StepperMotor.hpp inside
 * ClockLogic.hpp, creating a dependency.  The moment ClockLogic "knows"
 * about StepperMotor, you have coupled them again.
 *
 * The Registry pattern solves this with a three-part handshake:
 *
 *  1. REGISTRATION (main.cpp)
 *     You create your concrete hardware object and hand it to the registry
 *     under a human-readable name ("slot"):
 *
 *       StepperMotor hourMotor(2048, PINA, PINB, PINC, PIND);
 *       registry.registerMotor(Slots::Motor::HOUR_HAND, &hourMotor);
 *
 *  2. LOOKUP (ClockLogic.cpp)
 *     ClockLogic asks the registry for "the motor in slot hour_hand".
 *     It receives an IMotor* — it does NOT know the concrete type:
 *
 *       IMotor* hand = registry.getMotor(Slots::Motor::HOUR_HAND);
 *       if (hand) hand->setTarget(0.75f);   // 9 o'clock
 *
 *  3. ISOLATION
 *     ClockLogic never #includes StepperMotor.hpp, ServoMotor.hpp, or any
 *     other hardware file.  Swap the motor by changing ONE line in main.cpp.
 *
 * Think of the registry as a hotel reception desk:
 *   - main.cpp checks hardware in ("I have a StepperMotor for room
 *     'hour_hand'").
 *   - ClockLogic checks hardware out ("Give me whatever is in room
 *     'hour_hand'").
 *   - Reception (HardwareRegistry) doesn't care what the hardware is.
 *
 * ============================================================================
 * SLOT NAME CONSTANTS
 * ============================================================================
 *
 * Always use the constants in the Slots namespace (defined below) when
 * registering and looking up hardware.  Using raw string literals like
 * "hour_hand" in multiple files is error-prone — a single typo silently
 * returns nullptr at runtime.
 *
 * ============================================================================
 * MEMORY MODEL
 * ============================================================================
 *
 * The registry uses a simple fixed-capacity array rather than std::map or
 * std::unordered_map.  On microcontrollers, dynamic memory allocation and
 * STL containers can cause heap fragmentation and unpredictable behaviour.
 * Adjust MAX_MOTORS / MAX_DISPLAYS if your project needs more slots.
 *
 * The registry does NOT own the objects it holds — it stores raw pointers.
 * You are responsible for the lifetime of all registered objects.  Declaring
 * them as static locals in main.cpp (see src/main.cpp) is the standard
 * embedded pattern.
 *
 * ============================================================================
 */

#pragma once

#include <stdint.h>
#include "IMotor.hpp"
#include "IDisplay.hpp"
#include "IDigitMechanism.hpp"
#include "IDigitGroup.hpp"

// ----------------------------------------------------------------------------
// Slot name constants
// ----------------------------------------------------------------------------
// Use these everywhere instead of raw string literals.
// "Slots::Motor::HOUR_HAND" is safer than typing "hour_hand" twice.
// ----------------------------------------------------------------------------

namespace Slots
{
    namespace Motor
    {
        constexpr const char* HOUR_HAND   = "hour_hand";
        constexpr const char* MINUTE_HAND = "minute_hand";
        constexpr const char* SECOND_HAND = "second_hand";
    }

    namespace Display
    {
        constexpr const char* MAIN_RING = "main_ring";
    }

    // -------------------------------------------------------------------------
    // Digit mechanism slots — one slot per individual digit position.
    // Use these when you want to control each digit mechanism independently
    // (e.g. when the tens and ones use completely different motor types).
    //
    // ClockLogic will extract the tens/ones of each time unit and call
    // setDigit() on whichever mechanisms are registered here.
    // -------------------------------------------------------------------------
    namespace Digit
    {
        constexpr const char* HOURS_TENS    = "digit_h_tens";
        constexpr const char* HOURS_ONES    = "digit_h_ones";
        constexpr const char* MINUTES_TENS  = "digit_m_tens";
        constexpr const char* MINUTES_ONES  = "digit_m_ones";
        constexpr const char* SECONDS_TENS  = "digit_s_tens";
        constexpr const char* SECONDS_ONES  = "digit_s_ones";
    }

    // -------------------------------------------------------------------------
    // Digit group slots — one slot per time unit.
    // Use these when your tens and ones digit mechanisms are managed together
    // by a DigitGroup object.  ClockLogic will call setValue(hours),
    // setValue(minutes), or setValue(seconds) on the registered group, and the
    // group handles the tens/ones split internally.
    //
    // Prefer digit groups when both digits use the same mechanism type and
    // you want to keep the registration code compact.
    // -------------------------------------------------------------------------
    namespace DigitGroup
    {
        constexpr const char* HOURS   = "group_hours";
        constexpr const char* MINUTES = "group_minutes";
        constexpr const char* SECONDS = "group_seconds";
    }
}

// ----------------------------------------------------------------------------
// HardwareRegistry declaration
// (Implementation is in src/HardwareRegistry.cpp)
// ----------------------------------------------------------------------------

class HardwareRegistry
{
public:
    // -----------------------------------------------------------------------
    // Motor slots
    // -----------------------------------------------------------------------

    /**
     * @brief Register a motor implementation under a named slot.
     *
     * @param slot   Slot name — use the Slots::Motor constants.
     * @param motor  Pointer to a concrete IMotor object.
     *
     * @note The registry does NOT take ownership of the pointer.
     *       If _motorCount has reached MAX_MOTORS the call is silently
     *       dropped; consider increasing MAX_MOTORS if you hit this limit.
     */
    void registerMotor(const char* slot, IMotor* motor);

    /**
     * @brief Look up a motor by slot name.
     *
     * @param slot  The slot name used during registerMotor().
     * @return      Pointer to the registered IMotor, or nullptr if not found.
     *
     * @note Always null-check the return value before dereferencing.
     *       A missing registration is a common setup mistake and nullptr
     *       checks make it safe rather than causing a hard fault.
     */
    IMotor* getMotor(const char* slot) const;

    // -----------------------------------------------------------------------
    // Display slots
    // -----------------------------------------------------------------------

    /**
     * @brief Register a display implementation under a named slot.
     *
     * @param slot     Slot name — use the Slots::Display constants.
     * @param display  Pointer to a concrete IDisplay object.
     */
    void registerDisplay(const char* slot, IDisplay* display);

    /**
     * @brief Look up a display by slot name.
     *
     * @param slot  The slot name used during registerDisplay().
     * @return      Pointer to the registered IDisplay, or nullptr if not found.
     */
    IDisplay* getDisplay(const char* slot) const;

    // -----------------------------------------------------------------------
    // Digit mechanism slots — individual digit positions
    // -----------------------------------------------------------------------

    /**
     * @brief Register a digit mechanism under a named slot.
     *
     * @param slot       Slot name — use the Slots::Digit constants.
     * @param mechanism  Pointer to a concrete IDigitMechanism object.
     *
     * @note The registry does NOT own the object.  Declare the mechanism
     *       as a static local in setup() to ensure correct lifetime.
     */
    void registerDigitMechanism(const char* slot, IDigitMechanism* mechanism);

    /**
     * @brief Look up a digit mechanism by slot name.
     *
     * @param slot  The slot name used during registerDigitMechanism().
     * @return      Pointer to the registered IDigitMechanism, or nullptr.
     */
    IDigitMechanism* getDigitMechanism(const char* slot) const;

    // -----------------------------------------------------------------------
    // Digit group slots — tens/ones groups per time unit
    // -----------------------------------------------------------------------

    /**
     * @brief Register a digit group under a named slot.
     *
     * @param slot   Slot name — use the Slots::DigitGroup constants.
     * @param group  Pointer to a concrete IDigitGroup object.
     */
    void registerDigitGroup(const char* slot, IDigitGroup* group);

    /**
     * @brief Look up a digit group by slot name.
     *
     * @param slot  The slot name used during registerDigitGroup().
     * @return      Pointer to the registered IDigitGroup, or nullptr.
     */
    IDigitGroup* getDigitGroup(const char* slot) const;

private:
    // Fixed-capacity slot tables — no heap allocation needed.
    static constexpr uint8_t MAX_MOTORS   = 8;
    static constexpr uint8_t MAX_DISPLAYS = 4;

    struct MotorEntry
    {
        const char* slot;
        IMotor*     motor;
    };

    struct DisplayEntry
    {
        const char* slot;
        IDisplay*   display;
    };

    MotorEntry   _motors[MAX_MOTORS]     = {};
    DisplayEntry _displays[MAX_DISPLAYS] = {};
    uint8_t      _motorCount             = 0;
    uint8_t      _displayCount           = 0;

    // Digit mechanism slots
    static constexpr uint8_t MAX_DIGIT_MECHANISMS = 8;
    static constexpr uint8_t MAX_DIGIT_GROUPS     = 4;

    struct DigitMechanismEntry
    {
        const char*      slot;
        IDigitMechanism* mechanism;
    };

    struct DigitGroupEntry
    {
        const char* slot;
        IDigitGroup* group;
    };

    DigitMechanismEntry _digitMechanisms[MAX_DIGIT_MECHANISMS] = {};
    DigitGroupEntry     _digitGroups[MAX_DIGIT_GROUPS]         = {};
    uint8_t             _digitMechanismCount                   = 0;
    uint8_t             _digitGroupCount                       = 0;
};
