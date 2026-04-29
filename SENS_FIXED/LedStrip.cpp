#include "LedStrip.h"

#include <Adafruit_NeoPixel.h>
#include "SensorConfig.h"

static Adafruit_NeoPixel strip(NEO_COUNT, NEO_PIN, NEO_GRB + NEO_KHZ800);

static const uint16_t COLORS565[8] = {
  0xF800, 0x07E0, 0x001F, 0xFFE0, 0x07FF, 0xF81F, 0xFD20, 0xFFFF
};
static const uint8_t DEF_CI[8] = {0, 1, 2, 3, 4, 5, 6, 7};

static uint8_t ledCi[NEO_COUNT];
static uint32_t ledOffAt[NEO_COUNT] = {0};

static uint32_t cidxToNeo(uint8_t ci) {
  if (ci >= 8) return 0;
  uint16_t c = COLORS565[ci];
  uint8_t r5 = (c >> 11) & 0x1F;
  uint8_t g6 = (c >> 5) & 0x3F;
  uint8_t b5 = c & 0x1F;
  return strip.Color((r5 * 527 + 23) >> 6, (g6 * 259 + 33) >> 6, (b5 * 527 + 23) >> 6);
}

void ledBegin() {
  for (uint8_t i = 0; i < NEO_COUNT; i++) ledCi[i] = DEF_CI[i];
  strip.begin();
  strip.setBrightness(NEO_BRIGHT);
  strip.clear();
  strip.show();
}

void ledTick() {
  uint32_t now = millis();
  bool any = false;
  for (uint8_t i = 0; i < NEO_COUNT; i++) {
    if (ledOffAt[i] && now >= ledOffAt[i]) {
      strip.setPixelColor(i, 0);
      ledOffAt[i] = 0;
      any = true;
    }
  }
  if (any) strip.show();
}

void ledSetColorIndex(uint8_t index, uint8_t colorIdx) {
  if (index >= NEO_COUNT) return;
  ledCi[index] = colorIdx;
}

void ledTrigger(uint8_t index) {
  if (index >= NEO_COUNT) return;
  strip.setPixelColor(index, cidxToNeo(ledCi[index]));
  strip.show();
  ledOffAt[index] = millis() + 300;
}

void ledShowActiveSensors(bool active[], uint8_t count) {
  for (uint8_t pass = 0; pass < 2; pass++) {
    for (uint8_t i = 0; i < count && i < NEO_COUNT; i++) {
      if (active[i]) strip.setPixelColor(i, cidxToNeo(ledCi[i]));
    }
    strip.show();
    delay(200);
    strip.clear();
    strip.show();
    delay(100);
  }
}

void ledCalibrationWarning(bool active[], uint8_t count) {
  for (uint8_t flash = 0; flash < 7; flash++) {
    for (uint8_t i = 0; i < count && i < NEO_COUNT; i++) {
      if (active[i]) strip.setPixelColor(i, strip.Color(180, 180, 0));
    }
    strip.show();
    delay(150);
    strip.clear();
    strip.show();
    delay(130);
  }
}

void ledFlashCalibrating(uint8_t index) {
  if (index >= NEO_COUNT) return;
  strip.setPixelColor(index, strip.Color(30, 30, 30));
  strip.show();
}

void ledClearOne(uint8_t index) {
  if (index >= NEO_COUNT) return;
  strip.setPixelColor(index, 0);
  strip.show();
}
