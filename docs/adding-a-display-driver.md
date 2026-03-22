# How to Add a Display Driver

This guide walks you through wrapping the Adafruit NeoPixel library — a popular ring of addressable RGB LEDs — into the `IDisplay` interface. The same pattern applies to any other display technology (OLED, e-ink, 7-segment, etc.).

---

## Prerequisites

- You have read [architecture.md](architecture.md) and understand the interface pattern.
- You have a NeoPixel ring or strip physically wired to your microcontroller.
- PlatformIO is installed and the project builds (even with the stubs).

---

## Step 1 — Understand What You Are Building

The `IDisplay` interface defines a contract with five methods:

```cpp
virtual void     setPixel(uint16_t index, uint8_t r, uint8_t g, uint8_t b) = 0;
virtual void     show()                                                     = 0;
virtual void     clear()                                                    = 0;
virtual void     update()                                                   = 0;
virtual uint16_t getPixelCount() const                                      = 0;
```

Your driver must inherit `IDisplay` and implement all five. The framework writes pixel data via `setPixel()`, commits frames via `show()`, and calls `update()` every loop for any animation state.

---

## Step 2 — Install the Library

In your sculpture project's `platformio.ini`, declare both clocksmith and the Adafruit NeoPixel library under `lib_deps`:

```ini
[env:arduino_uno]
platform    = atmelavr
board       = uno
framework   = arduino
lib_deps    =
    https://github.com/h0witzer/clocksmith.git
    adafruit/Adafruit NeoPixel
```

Run `pio pkg install` (or let the IDE sync) to download both. The NeoPixel library provides `Adafruit_NeoPixel.h`.

---

## Step 3 — Copy the Stub File

In your **sculpture project** (not the clocksmith repo), create a `lib/drivers/` folder if it doesn't exist. Copy `hal/displays/NeoPixelDisplayStub.hpp` from the clocksmith library — available at `.pio/libdeps/<env>/clocksmith/hal/displays/NeoPixelDisplayStub.hpp` after `pio pkg install`, or [directly on GitHub](https://github.com/h0witzer/clocksmith/blob/main/hal/displays/NeoPixelDisplayStub.hpp) — and save it as your new driver:

```
lib/drivers/NeoPixelRing60.hpp   (a 60-LED ring for the seconds track)
lib/drivers/NeoPixelRing12.hpp   (a 12-LED ring for the hours track)
```

Open your new file, change the class name at the top, and work through the TODOs.

---

## Step 4 — Add the Library Include and Member

At the top of your file, include the library and add a member variable:

```cpp
#include <Adafruit_NeoPixel.h>
#include "IDisplay.hpp"

class NeoPixelRing60 : public IDisplay
{
public:
    // ...
private:
    uint16_t        _pixelCount;
    Adafruit_NeoPixel _strip;  // ← the real NeoPixel object
};
```

---

## Step 5 — Implement the Constructor

The constructor initialises the hardware. It runs once in `setup()`.

```cpp
NeoPixelRing60(uint16_t pixelCount, uint8_t dataPin)
    : _pixelCount(pixelCount)
    , _strip(pixelCount, dataPin, NEO_GRB + NEO_KHZ800)
{
    _strip.begin();
    _strip.setBrightness(50);  // 0–255; 50 ≈ 20% brightness (safe on USB power)
    _strip.clear();
    _strip.show();             // initialise all pixels to "off"
}
```

**NEO_GRB vs NEO_RGB:** Most NeoPixel rings expect data in GRB order (Green, Red, Blue), not RGB. Use `NEO_GRB` unless your strip's datasheet says otherwise. Adafruit's website lists the correct constant for every product they sell.

**Brightness:** Setting `setBrightness(50)` limits power draw and protects your MCU's 5 V rail when the strip is powered from USB. For a 60-LED ring at full white (255, 255, 255), uncapped brightness draws roughly 3.6 A — more than most USB ports can supply. A brightness of 50/255 brings this to about 700 mA, which is safe.

---

## Step 6 — Implement `setPixel()`

```cpp
void setPixel(uint16_t index, uint8_t r, uint8_t g, uint8_t b) override
{
    if (index >= _pixelCount) return;        // silent bounds check
    _strip.setPixelColor(index, r, g, b);   // writes to the internal buffer
}
```

**Why the bounds check?**
If ClockLogic accidentally passes an index of 60 to a 60-pixel ring, the Adafruit library silently ignores it — but not all LED libraries do. The explicit check here makes the behaviour predictable regardless of the underlying library.

---

## Step 7 — Implement `show()`

```cpp
void show() override
{
    _strip.show();
}
```

`Adafruit_NeoPixel::show()` transmits the internal pixel buffer to the LEDs using a precisely timed data burst. For a 60-LED ring this takes roughly 1.8 ms. During the burst, interrupts are disabled on AVR MCUs — a known limitation of the NeoPixel protocol. For most clock sculptures this is acceptable; if you need interrupt-safe LED control, consider the FastLED library with a DMA-capable MCU.

---

## Step 8 — Implement `clear()` and `update()`

```cpp
void clear() override
{
    _strip.clear();
    // Note: does NOT call show() — the caller decides when to commit.
}

void update() override
{
    // A static NeoPixel ring has no autonomous animation, so this is a
    // no-op for now.
    //
    // Future ideas:
    //   - Brightness pulse on each second tick
    //   - Rainbow sweep background
    //   - Fade-in / fade-out on hand positions
    // All of these belong here, using millis()-based timing (no delay()).
}
```

---

## Step 9 — Implement `getPixelCount()`

```cpp
uint16_t getPixelCount() const override
{
    return _pixelCount;
}
```

---

## Step 10 — Register the Display in `main.cpp`

In your sculpture project's `src/main.cpp`, add the include and the registration. PlatformIO automatically discovers files in `lib/drivers/`, so you can include by filename only:

```cpp
// Add near the top of main.cpp (the only file allowed to include concrete drivers):
#include "NeoPixelRing60.hpp"

// Inside setup():
static NeoPixelRing60 ring(60, /* dataPin = */ 6);
registry.registerDisplay(Slots::Display::MAIN_RING, &ring);
```

That is it. Call `ring.update()` in `loop()` so the display can process any autonomous animation state:

```cpp
// Inside loop():
IDisplay* d = registry.getDisplay(Slots::Display::MAIN_RING);
if (d) d->update();
```

---

## Adapting for Other Display Types

### OLED (SSD1306 via I²C)

The key difference with an OLED is that "pixels" map to individual screen pixels on a bitmap canvas, not discrete LEDs. Your driver would:

- Hold an `Adafruit_SSD1306` (or `U8g2`) instance.
- In `setPixel()`, call `display.drawPixel(x, y, color)`.
- In `show()`, call `display.display()`.
- Optionally translate the 1D pixel index to 2D coordinates inside `setPixel()`.

### 7-Segment Digits (TM1637)

A 7-segment display has no pixel concept — it shows digits. Your driver would:

- Ignore `r`, `g`, `b` in `setPixel()`.
- Treat `index` as a digit position (0 = leftmost digit).
- Treat the `r` value as the digit to display (0–9).
- In `show()`, push the digit buffer to the TM1637 driver.

The point is that `IDisplay` is a convention, not a rigid pixel renderer. The driver decides how to translate the generic calls into its specific hardware API.

---

## Checklist

Before you mark the driver as done, verify:

- [ ] The class inherits `IDisplay` and all five pure virtual methods are implemented.
- [ ] `setPixel()` silently ignores out-of-range indices.
- [ ] `clear()` does NOT call `show()` automatically.
- [ ] `update()` contains no `delay()` call.
- [ ] The driver file is in `lib/drivers/` of your sculpture project.
- [ ] The display is registered in `main.cpp` under `Slots::Display::MAIN_RING`.
- [ ] `update()` is called in `loop()`.

---

## Further Reading

- [IDisplay interface](../include/IDisplay.hpp)
- [NeoPixelDisplayStub template](../hal/displays/NeoPixelDisplayStub.hpp)
- [Architecture overview](architecture.md)
- [Adafruit NeoPixel Überguide](https://learn.adafruit.com/adafruit-neopixel-uberguide)
