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

// Clear the display if no valid radar frame arrives for this long
#define RADAR_TIMEOUT_MS 1000

// Current state
float currentDistance = -1; // -1 means no presence
bool isPresent = false;
unsigned long lastDisplayUpdate = 0;
unsigned long lastFrameTime = 0;

// --- Function Prototypes ---
void feedRadarByte(byte b);
void processRadarData(byte* frameBuffer, int length);
void updateDisplay();
uint32_t colorWheel(byte WheelPos);

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
  // Read incoming bytes from the LD2410C (via the RS-485 link) and feed the
  // frame parser one byte at a time.
  while (RadarSerial.available()) {
    feedRadarByte((byte)RadarSerial.read());
  }

  // If the radar stops talking (cable unplugged, sensor reset...) don't leave
  // the LEDs frozen on the last reading.
  if (isPresent && millis() - lastFrameTime > RADAR_TIMEOUT_MS) {
    isPresent = false;
    currentDistance = -1;
  }

  // Non-blocking display update
  if (millis() - lastDisplayUpdate >= 20) {
    updateDisplay();
    lastDisplayUpdate = millis();
  }
}

// LD2410C report frame layout:
//   F4 F3 F2 F1 | len_lo len_hi | <len bytes of payload> | F8 F7 F6 F5
// The length field tells us exactly where the frame ends, which is much more
// robust than searching for the tail bytes (they can appear inside the payload).
void feedRadarByte(byte b) {
  if (bufferIndex < 4) {
    // Still looking for the 4-byte header.
    if (b == FRAME_HEADER[bufferIndex]) {
      buffer[bufferIndex++] = b;
    } else {
      // Restart; the byte may itself be the first header byte.
      bufferIndex = (b == FRAME_HEADER[0]) ? 1 : 0;
      if (bufferIndex) buffer[0] = b;
    }
    return;
  }

  buffer[bufferIndex++] = b;

  if (bufferIndex < 6) return; // waiting for the length field

  int payloadLen = buffer[4] | (buffer[5] << 8);
  int frameLen = 4 + 2 + payloadLen + 4;

  if (payloadLen <= 0 || frameLen > MAX_BUFFER_SIZE) {
    bufferIndex = 0; // corrupt length, resync
    return;
  }

  if (bufferIndex < frameLen) return; // keep accumulating

  // Full frame received: validate the tail before using it.
  if (buffer[frameLen - 4] == FRAME_TAIL[0] &&
      buffer[frameLen - 3] == FRAME_TAIL[1] &&
      buffer[frameLen - 2] == FRAME_TAIL[2] &&
      buffer[frameLen - 1] == FRAME_TAIL[3]) {
    processRadarData(buffer, frameLen);
  }
  bufferIndex = 0;
}

void processRadarData(byte* frameBuffer, int length) {
  // Payload (starting at index 6):
  //   [6]  data type   0x02 = basic mode, 0x01 = engineering mode
  //   [7]  0xAA        intra-frame head
  //   [8]  target state 0x00 none, 0x01 moving, 0x02 static, 0x03 both
  //   [9..10]  moving target distance (cm, little endian)
  //   [11]     moving target energy
  //   [12..13] stationary target distance (cm)
  //   [14]     stationary target energy
  //   [15..16] detection distance (cm)
  // A basic-mode frame is 23 bytes; engineering mode frames are longer but
  // start with the same fields, so both are handled here.
  if (length < 23) return;
  if (frameBuffer[6] != 0x01 && frameBuffer[6] != 0x02) return;
  if (frameBuffer[7] != 0xAA) return;

  lastFrameTime = millis();

  byte targetState = frameBuffer[8];
  int moveDist   = frameBuffer[9]  | (frameBuffer[10] << 8);
  int staticDist = frameBuffer[12] | (frameBuffer[13] << 8);

  switch (targetState) {
    case 0x01: // moving
      currentDistance = moveDist;
      break;
    case 0x02: // stationary
      currentDistance = staticDist;
      break;
    case 0x03: // both: use whichever is closer
      currentDistance = min(moveDist, staticDist);
      if (currentDistance <= 0) currentDistance = max(moveDist, staticDist);
      break;
    default:   // 0x00 no target (or unknown)
      isPresent = false;
      currentDistance = -1;
      return;
  }

  isPresent = currentDistance > 0;
  if (currentDistance > 1000) currentDistance = 1000; // sanity clamp
}

void updateDisplay() {
  strip.clear();

  if (!isPresent || currentDistance < 0) {
    strip.show();
    return;
  }

  float dist = constrain(currentDistance, MIN_DISTANCE_CM, MAX_DISTANCE_CM);

  // Closer = more LEDs. Farther = fewer LEDs.
  int ledsToLight = map((long)dist, (long)MAX_DISTANCE_CM, (long)MIN_DISTANCE_CM, 1, NUM_LEDS);
  ledsToLight = constrain(ledsToLight, 1, NUM_LEDS);

  // We want Blue for far, Red for close.
  int colorVal = map((long)dist, (long)MIN_DISTANCE_CM, (long)MAX_DISTANCE_CM, 0, 170);
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
