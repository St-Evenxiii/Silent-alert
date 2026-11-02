// Minimal host-side Arduino mock used only by test_parser.cpp
#pragma once
#include <cstdint>
#include <cstdio>
#include <deque>
#include <algorithm>
typedef uint8_t byte;
using std::min; using std::max;
#define SERIAL_8N1 0
extern unsigned long g_millis;
inline unsigned long millis() { return g_millis; }
inline long map(long x, long a, long b, long c, long d) { return (x - a) * (d - c) / (b - a) + c; }
template <class T, class L, class H> T constrain(T x, L lo, H hi) { return x < lo ? lo : (x > hi ? hi : x); }
struct MockSerial {
  std::deque<byte> rx;
  MockSerial(int = 0) {}
  void begin(long, int = 0, int = 0, int = 0) {}
  int available() { return rx.size(); }
  int read() { byte b = rx.front(); rx.pop_front(); return b; }
  void println(const char*) {}
};
typedef MockSerial HardwareSerial;
extern MockSerial Serial;
