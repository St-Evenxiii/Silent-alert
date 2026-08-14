#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

// --- Configuration ---

// NeoPixel Configuration
#define LED_PIN 4        // Data pin connected to NeoPixels
#define NUM_LEDS 30      // Number of LEDs in the strip
#define BRIGHTNESS 150   // 0-255

// LD2410C Distance Configuration
#define MAX_DISTANCE_CM 600.0 // Distance at which 1 LED is blue
#define MIN_DISTANCE_CM 50.0  // Distance at which all LEDs are red

// Hardware Serial for RS-485 Communication with LD2410C
#define RXD2 16
#define TXD2 17

// --- Globals ---
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);
HardwareSerial RadarSerial(2);

// LD2410C parsing variables
const byte FRAME_HEADER[] = {0xF4, 0xF3, 0xF2, 0xF1};
const byte FRAME_TAIL[]   = {0xF8, 0xF7, 0xF6, 0xF5};

#define MAX_BUFFER_SIZE 256
byte buffer[MAX_BUFFER_SIZE];
int bufferIndex = 0;

// Current state
float currentDistance = -1; // -1 means no presence
bool isPresent = false;
unsigned long lastDisplayUpdate = 0;

// --- Function Prototypes ---
void processRadarData(byte* frameBuffer, int length);
void updateDisplay();
uint32_t colorWheel(byte WheelPos);
void shiftBufferDown(int shiftAmount);

void setup() {
  Serial.begin(115200); // Debug serial

  // Standard baud rate for LD2410C is 256000
  RadarSerial.begin(256000, SERIAL_8N1, RXD2, TXD2);

  // Initialize NeoPixels
  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.show();

  Serial.println("Silent Alarm Starting...");
}

void loop() {
  // Read incoming data from RS-485 (LD2410C)
  while (RadarSerial.available()) {
    byte b = RadarSerial.read();

    // Prevent buffer overflow
    if (bufferIndex >= MAX_BUFFER_SIZE) {
      shiftBufferDown(1);
    }

    buffer[bufferIndex++] = b;

    // We need at least the length of a header to start checking
    if (bufferIndex >= 4) {
        // If the start of our buffer doesn't match the header, we are out of sync.
        while (bufferIndex > 0 &&
               !(buffer[0] == FRAME_HEADER[0] &&
                 (bufferIndex < 2 || buffer[1] == FRAME_HEADER[1]) &&
                 (bufferIndex < 3 || buffer[2] == FRAME_HEADER[2]) &&
                 (bufferIndex < 4 || buffer[3] == FRAME_HEADER[3]))) {

            shiftBufferDown(1);
        }
    }

    // Now, if our buffer starts with a valid header and we have enough bytes for a tail
    if (bufferIndex >= 8 &&
        buffer[0] == FRAME_HEADER[0] && buffer[1] == FRAME_HEADER[1] &&
        buffer[2] == FRAME_HEADER[2] && buffer[3] == FRAME_HEADER[3]) {

        // Look for the tail at the end of our current buffer
        if (buffer[bufferIndex-4] == FRAME_TAIL[0] &&
            buffer[bufferIndex-3] == FRAME_TAIL[1] &&
            buffer[bufferIndex-2] == FRAME_TAIL[2] &&
            buffer[bufferIndex-1] == FRAME_TAIL[3]) {

            // We found a complete frame!
            processRadarData(buffer, bufferIndex);

            // Reset buffer to start accumulating the next frame
            bufferIndex = 0;
        }
    }
  }

  // Non-blocking update display
  if (millis() - lastDisplayUpdate >= 20) {
    updateDisplay();
    lastDisplayUpdate = millis();
  }
}

void shiftBufferDown(int shiftAmount) {
    if (shiftAmount >= bufferIndex) {
        bufferIndex = 0;
        return;
    }
    for (int i = 0; i < bufferIndex - shiftAmount; i++) {
        buffer[i] = buffer[i + shiftAmount];
    }
    bufferIndex -= shiftAmount;
}

void processRadarData(byte* frameBuffer, int length) {
  // LD2410C Engineering Mode or Basic Mode parsing.

  // Ensure we have enough data for a basic mode frame which is exactly 23 bytes.
  if (length < 23) {
      return;
  }

  // Data starts at index 6 (Header(4) + Length(2))
  // Index 6 is intra-frame head (0xAA or 0xAB)
  // Index 7 is data type
  if (frameBuffer[7] == 0x00 || frameBuffer[7] == 0x01 || frameBuffer[7] == 0x02) {

    // Target state is at index 8
    // 0x00: No target, 0x01: Moving target, 0x02: Static target, 0x03: Moving and static
    byte targetState = frameBuffer[8];

    if (targetState == 0x00) {
      isPresent = false;
      currentDistance = -1;
    } else {
      isPresent = true;

      // Moving distance is at index 9, 10
      int moveDist = frameBuffer[9] | (frameBuffer[10] << 8); // in cm
      // Static distance is at index 12, 13
      int staticDist = frameBuffer[12] | (frameBuffer[13] << 8); // in cm

      if (targetState == 0x01 || targetState == 0x03) {
        currentDistance = moveDist;
      } else if (targetState == 0x02) {
        currentDistance = staticDist;
      }

      // Sanity check, ignore crazy values
      if(currentDistance > 1000) currentDistance = 1000;
    }
  }
}

void updateDisplay() {
  strip.clear();

  if (!isPresent || currentDistance < 0) {
    strip.show();
    return;
  }

  float dist = constrain(currentDistance, MIN_DISTANCE_CM, MAX_DISTANCE_CM);

  // Closer = more LEDs. Farther = fewer LEDs.
  int ledsToLight = map(dist, MAX_DISTANCE_CM, MIN_DISTANCE_CM, 1, NUM_LEDS);
  ledsToLight = constrain(ledsToLight, 1, NUM_LEDS);

  // We want Blue for far, Red for close.
  int colorVal = map(dist, MIN_DISTANCE_CM, MAX_DISTANCE_CM, 0, 170);
  colorVal = constrain(colorVal, 0, 170);

  uint32_t activeColor = colorWheel(colorVal);

  for (int i = 0; i < ledsToLight; i++) {
    strip.setPixelColor(i, activeColor);
  }

  strip.show();
}

uint32_t colorWheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}
