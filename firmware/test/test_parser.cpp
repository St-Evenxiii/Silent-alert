// Host test: g++ -std=c++17 -I. test_parser.cpp -o t && ./t
#include "Arduino.h"
unsigned long g_millis = 0;
MockSerial Serial;
#include "../silent_alarm.ino"
#include <cassert>
#include <vector>

static void sendFrame(byte state, int moveCm, int staticCm, bool junkFirst = false) {
  std::vector<byte> f = {0xF4,0xF3,0xF2,0xF1, 0x0D,0x00, 0x02,0xAA, state,
    (byte)moveCm,(byte)(moveCm>>8), 50, (byte)staticCm,(byte)(staticCm>>8), 40,
    0x00,0x00, 0x55,0x00, 0xF8,0xF7,0xF6,0xF5};
  if (junkFirst) for (byte b : {0x12, 0xF4, 0xF3, 0x00}) RadarSerial.rx.push_back(b);
  for (byte b : f) RadarSerial.rx.push_back(b);
}
static void tick(unsigned long ms = 25) { g_millis += ms; loop(); }

int main() {
  setup();
  sendFrame(0x01, 60, 0, true); tick();
  assert(isPresent && currentDistance == 60);
  printf("close moving target: %d LEDs, color %06X\n", strip.lit(), (unsigned)strip.px[0]);
  fflush(stdout);
  assert(strip.lit() > 25 && (strip.px[0] >> 16) > 0xE0 && (strip.px[0] & 0xFF) < 0x20);

  sendFrame(0x02, 0, 590); tick();
  printf("far static target:   %d LEDs, color %06X\n", strip.lit(), (unsigned)strip.px[0]);
  assert(isPresent && strip.lit() <= 2 && (strip.px[0] & 0xFF) > 0xE0 && (strip.px[0] >> 16) < 0x20);

  sendFrame(0x03, 300, 200); tick();
  assert(currentDistance == 200);

  sendFrame(0x00, 0, 0); tick();
  assert(!isPresent && strip.lit() == 0);

  sendFrame(0x01, 100, 0); tick();
  assert(isPresent);
  for (int i = 0; i < 60; i++) tick(); // no frames for >1s
  assert(!isPresent && strip.lit() == 0);
  printf("all parser tests passed\n");
}
