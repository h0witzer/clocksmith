/**
 * @file IDisplay.hpp
 * @brief Abstract interface for any visual display peripheral.
 *
 * ============================================================================
 * WHY THIS EXISTS — A NOTE FOR ARCHITECTURE BEGINNERS
 * ============================================================================
 *
 * A "display" on a kinetic clock sculpture can be many things:
 *   - A ring of 60 NeoPixel LEDs around the clock face
 *   - A small OLED module showing a digital time readout
 *   - A string of 7-segment digits
 *   - A colour-changing e-ink panel
 *
 * Each of those requires a completely different library and a completely
 * different API.  Yet ClockLogic wants to say "light up pixel 15" without
 * knowing what hardware is underneath.
 *
 * IDisplay provides that uniform language.  The concrete driver
 * (e.g. NeoPixelDisplay) translates these generic calls into the real
 * hardware library calls.  ClockLogic stays clean.
 *
 * ============================================================================
 * THE PIXEL MODEL
 * ============================================================================
 *
 * We model the display as an ordered strip of addressable colour pixels:
 *   - Pixels are indexed from 0 to (getPixelCount() - 1).
 *   - Each pixel has an RGB colour (three uint8_t values, 0–255).
 *
 * For a NeoPixel ring this maps one-to-one to individual LEDs.
 * For a 7-segment display the driver would translate a pixel colour to
 * a segment on/off state — but ClockLogic never needs to know that.
 *
 * ============================================================================
 * DOUBLE BUFFERING
 * ============================================================================
 *
 * Writing pixels is split into two phases:
 *   1. setPixel() — writes to an internal software buffer (fast, no I/O).
 *   2. show()     — pushes the buffer to the hardware (triggers a data burst).
 *
 * This prevents partial-frame flicker: the display only updates when all
 * pixel data for the current frame is ready.
 *
 * ============================================================================
 */

#pragma once

#include <stdint.h>

class IDisplay
{
public:
    virtual ~IDisplay() = default;

    /**
     * @brief Set the colour of a single pixel in the internal buffer.
     *
     * @param index  Zero-based pixel index.  Out-of-range values must be
     *               silently ignored by the concrete implementation.
     * @param r      Red   channel (0–255).
     * @param g      Green channel (0–255).
     * @param b      Blue  channel (0–255).
     *
     * @note This writes to an internal buffer only.  Call show() to push
     *       the buffer to the physical hardware.
     */
    virtual void setPixel(uint16_t index, uint8_t r, uint8_t g, uint8_t b) = 0;

    /**
     * @brief Push the internal pixel buffer to the physical display hardware.
     *
     * For NeoPixels this triggers the timed data burst.
     * For I2C OLEDs this may call display() or similar.
     * Implementations that update continuously may treat this as a no-op.
     */
    virtual void show() = 0;

    /**
     * @brief Set all pixels to off (black) in the internal buffer.
     *
     * @note Does NOT automatically call show().  Call show() afterwards if
     *       you want the hardware to reflect the cleared state immediately.
     */
    virtual void clear() = 0;

    /**
     * @brief Advance any time-dependent display logic by one tick.
     *
     * Use this for animations, brightness fades, or blink effects that
     * should progress smoothly over time.  Like IMotor::update(), this
     * method must be completely non-blocking.
     */
    virtual void update() = 0;

    /**
     * @brief Report the total number of addressable pixels.
     *
     * @return Pixel count as configured in the concrete implementation.
     */
    virtual uint16_t getPixelCount() const = 0;
};
