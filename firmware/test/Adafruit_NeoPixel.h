#pragma once
#include "Arduino.h"
#define NEO_GRB 0
#define NEO_KHZ800 0
struct Adafruit_NeoPixel {
  int n; uint32_t px[64] = {0};
  Adafruit_NeoPixel(int n, int, int) : n(n) {}
  void begin() {} void setBrightness(int) {} void show() {}
  void clear() { for (int i = 0; i < n; i++) px[i] = 0; }
  void setPixelColor(int i, uint32_t c) { px[i] = c; }
  static uint32_t Color(int r, int g, int b) { return (r << 16) | (g << 8) | b; }
  int lit() { int c = 0; for (int i = 0; i < n; i++) c += px[i] != 0; return c; }
};
