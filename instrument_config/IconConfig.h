#pragma once
#include <Arduino.h>

struct IconConfig {
  const uint16_t *bitmap;
  const char* name;
  uint8_t sensorIndex;
  uint8_t playerIndex;
  uint8_t trackNumber;   // 1..8
  uint8_t colorIndex;
};
