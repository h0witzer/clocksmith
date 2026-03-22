/**
 * @file IClockCore.hpp
 * @brief Abstract interface for the time-keeping subsystem.
 *
 * ============================================================================
 * WHY THIS EXISTS — A NOTE FOR ARCHITECTURE BEGINNERS
 * ============================================================================
 *
 * "Where does the time come from?" is a surprisingly hardware-specific
 * question.  A real kinetic clock project might use any of these:
 *   - A DS3231 or DS1307 hardware RTC module (I2C, battery-backed)
 *   - An NTP client over WiFi (on an ESP8266/ESP32)
 *   - A GPS module for precision time
 *   - A software timer based on millis() — great for prototyping
 *   - A unit-test mock that counts fake seconds
 *
 * Rather than hard-coding "use the DS3231 library" inside ClockLogic,
 * we declare this interface.  ClockLogic only ever calls
 * IClockCore::getTime() and IClockCore::update().
 *
 * Swapping from a DS3231 RTC to an NTP client is then a single one-line
 * change in main.cpp — everything else compiles and runs unchanged.
 *
 * ============================================================================
 * TIME REPRESENTATION
 * ============================================================================
 *
 * Time is expressed as raw integer components:
 *   hours   — 0 to 23 (24-hour / military time)
 *   minutes — 0 to 59
 *   seconds — 0 to 59
 *
 * ClockLogic converts these integers to normalized motor positions internally
 * using simple arithmetic.  The time source does not need to know about
 * clock hands.
 *
 * ============================================================================
 */

#pragma once

#include <stdint.h>

class IClockCore
{
public:
    virtual ~IClockCore() = default;

    /**
     * @brief Refresh the internal time registers from the hardware source.
     *
     * For an RTC this might trigger an I2C read.
     * For a millis()-based implementation this might recalculate counters.
     * For an NTP client this might check whether a resync is due.
     *
     * @note Must be non-blocking.  If the hardware is busy (e.g. I2C bus
     *       contention) the implementation should return immediately and
     *       serve the last cached value until the next successful read.
     */
    virtual void update() = 0;

    /**
     * @brief Retrieve the current time.
     *
     * @param[out] hours    Current hour   (0–23).
     * @param[out] minutes  Current minute (0–59).
     * @param[out] seconds  Current second (0–59).
     *
     * @note Returns the most recent cached value.  Call update() first to
     *       ensure the value is fresh.
     */
    virtual void getTime(uint8_t& hours,
                         uint8_t& minutes,
                         uint8_t& seconds) const = 0;

    /**
     * @brief Set or override the current time.
     *
     * @param hours    Hour   (0–23).
     * @param minutes  Minute (0–59).
     * @param seconds  Second (0–59).
     *
     * Useful for initial synchronisation or for setting the clock manually.
     * An RTC implementation would write these values to the chip's registers.
     * A millis()-based implementation would recalculate its epoch offset.
     */
    virtual void setTime(uint8_t hours,
                         uint8_t minutes,
                         uint8_t seconds) = 0;

    /**
     * @brief Query whether the time source is considered reliable / synced.
     *
     * @return true   if the time can be trusted (e.g. the RTC battery is
     *                good, or the NTP sync succeeded recently).
     * @return false  if the time is unknown or stale — the caller should
     *                display an error or "not synced" state.
     */
    virtual bool isSynchronised() const = 0;
};
