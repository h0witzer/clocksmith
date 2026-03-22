/**
 * @file NeoPixelDisplayStub.hpp
 * @brief STUB / TEMPLATE for a NeoPixel ring IDisplay implementation.
 *
 * ============================================================================
 * THIS FILE IS A STARTING TEMPLATE — IT IS NOT FUNCTIONAL
 * ============================================================================
 *
 * It exists as a guided template showing exactly how to wrap the Adafruit
 * NeoPixel library into the IDisplay interface.  Every section that needs
 * real code is marked with a prominent TODO.
 *
 * HOW TO USE THIS FILE
 * --------------------
 *  1. Add the Adafruit NeoPixel library to platformio.ini:
 *       lib_deps = adafruit/Adafruit NeoPixel
 *  2. Copy this file and rename it
 *     (e.g. hal/displays/NeoPixelRing60.hpp).
 *  3. Uncomment the #include line below.
 *  4. Fill in each TODO section.
 *  5. #include your new file in src/main.cpp.
 *  6. Instantiate and register it (see src/main.cpp for the pattern).
 *
 * For a full walkthrough, read docs/adding-a-display-driver.md.
 *
 * ============================================================================
 * THE PIXEL MODEL
 * ============================================================================
 *
 * A NeoPixel ring maps perfectly to the IDisplay pixel model:
 *   - Each NeoPixel LED is one "pixel".
 *   - Pixels are indexed 0 to (numPixels - 1) in clockwise order.
 *   - ClockLogic can mark the second-hand position by lighting one LED,
 *     or draw a sweep by lighting a range of LEDs — all through setPixel().
 *
 * ============================================================================
 */

#pragma once

#include <stdint.h>
#include "IDisplay.hpp"

// TODO: Uncomment when the Adafruit NeoPixel library is installed:
// #include <Adafruit_NeoPixel.h>

class NeoPixelDisplayStub : public IDisplay
{
public:
    /**
     * @brief Constructor.
     *
     * @param pixelCount  Number of LEDs in the ring / strip.
     * @param dataPin     Arduino pin connected to the NeoPixel data line.
     */
    NeoPixelDisplayStub(uint16_t pixelCount, uint8_t dataPin)
        : _pixelCount(pixelCount)
    {
        // TODO: Initialise the Adafruit_NeoPixel object, e.g.:
        //
        //   _strip = Adafruit_NeoPixel(pixelCount, dataPin,
        //                              NEO_GRB + NEO_KHZ800);
        //   _strip.begin();
        //   _strip.setBrightness(50);   // 0–255; 50 ≈ 20% — safe for USB power
        //   _strip.clear();
        //   _strip.show();              // push the cleared state to the LEDs

        (void)dataPin;  // suppress "unused parameter" warning until TODO is done
    }

    // -----------------------------------------------------------------------
    // IDisplay interface implementation
    // -----------------------------------------------------------------------

    /**
     * @brief Write a colour to one pixel in the software buffer.
     *
     * Bounds-checks the index before forwarding to the library so that
     * ClockLogic can never cause an out-of-bounds write.
     */
    void setPixel(uint16_t index, uint8_t r, uint8_t g, uint8_t b) override
    {
        if (index >= _pixelCount) return;  // silent bounds check

        // TODO: _strip.setPixelColor(index, r, g, b);

        (void)r; (void)g; (void)b;  // suppress warnings until TODO is done
    }

    /**
     * @brief Push the software buffer to the physical LEDs (the "latch").
     *
     * NeoPixels require this explicit call after every frame.
     * The data burst takes roughly 30 µs per LED — for a 60-LED ring that
     * is about 1.8 ms.  This is still non-blocking in the sense that it
     * returns as soon as the burst completes; do not call it inside a
     * tight loop where 1.8 ms matters.
     */
    void show() override
    {
        // TODO: _strip.show();
    }

    /**
     * @brief Set all pixels in the buffer to off (black).
     *
     * Remember to call show() afterwards if you want the LEDs to go dark
     * immediately.
     */
    void clear() override
    {
        // TODO: _strip.clear();
    }

    /**
     * @brief Update hook for time-dependent animation state.
     *
     * A basic NeoPixel ring has no autonomous animation, so this is a
     * no-op for now.  You could add a brightness fade or blink state
     * machine here using millis()-based timing.
     */
    void update() override
    {
        // No autonomous animation state yet.
        // Example future use: brightness pulse on the second tick.
    }

    /**
     * @brief Return the total number of pixels.
     */
    uint16_t getPixelCount() const override
    {
        return _pixelCount;
    }

private:
    uint16_t _pixelCount;

    // TODO: Declare the Adafruit_NeoPixel instance, e.g.:
    // Adafruit_NeoPixel _strip;
};
