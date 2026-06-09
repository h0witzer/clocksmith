#pragma once
#include <Arduino.h>

// Non-blocking line reader for the USB host link. Accumulates characters until
// a newline, then exposes the completed line once.
namespace LineReader {

// Pull any available characters from the host serial port. Returns true once a
// full line is ready; read it with line() and it is cleared on the next poll.
bool poll();

// The most recently completed line (without the trailing newline).
const char* line();

}  // namespace LineReader
