/**
 * @file HardwareRegistry.cpp
 * @brief Implementation of HardwareRegistry.
 *
 * Kept intentionally small.  String comparison with strcmp() is used instead
 * of operator== to remain compatible with plain C-strings on all platforms,
 * including AVR builds where std::string can be costly.
 */

#include "HardwareRegistry.hpp"

#include <string.h>  // strcmp

// ----------------------------------------------------------------------------
// Motor slots
// ----------------------------------------------------------------------------

void HardwareRegistry::registerMotor(const char* slot, IMotor* motor)
{
    if (_motorCount >= MAX_MOTORS) return;
    _motors[_motorCount].slot  = slot;
    _motors[_motorCount].motor = motor;
    ++_motorCount;
}

IMotor* HardwareRegistry::getMotor(const char* slot) const
{
    for (uint8_t i = 0; i < _motorCount; ++i)
    {
        if (strcmp(_motors[i].slot, slot) == 0)
        {
            return _motors[i].motor;
        }
    }
    return nullptr;
}

// ----------------------------------------------------------------------------
// Display slots
// ----------------------------------------------------------------------------

void HardwareRegistry::registerDisplay(const char* slot, IDisplay* display)
{
    if (_displayCount >= MAX_DISPLAYS) return;
    _displays[_displayCount].slot    = slot;
    _displays[_displayCount].display = display;
    ++_displayCount;
}

IDisplay* HardwareRegistry::getDisplay(const char* slot) const
{
    for (uint8_t i = 0; i < _displayCount; ++i)
    {
        if (strcmp(_displays[i].slot, slot) == 0)
        {
            return _displays[i].display;
        }
    }
    return nullptr;
}
