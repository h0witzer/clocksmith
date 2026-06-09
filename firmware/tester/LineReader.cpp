#include "LineReader.h"
#include "Config.h"

namespace LineReader {

static char s_buf[96];
static uint8_t s_len = 0;
static bool s_ready = false;

bool poll() {
  if (s_ready) {        // clear the previous line before reading the next
    s_ready = false;
    s_len = 0;
  }
  while (HOST_SERIAL.available()) {
    char c = (char)HOST_SERIAL.read();
    if (c == '\r') continue;
    if (c == '\n') {
      s_buf[s_len] = '\0';
      s_ready = true;
      return true;
    }
    if (s_len < sizeof(s_buf) - 1) {
      s_buf[s_len++] = c;
    }
  }
  return false;
}

const char* line() { return s_buf; }

}  // namespace LineReader
