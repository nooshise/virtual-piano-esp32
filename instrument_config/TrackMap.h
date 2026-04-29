#pragma once

#include <Arduino.h>

// SD root layout: 001.mp3 ... 064.mp3.
// track = instrumentId * 8 + noteIndex + 1
static inline uint16_t trackFor(uint8_t instrumentId, uint8_t noteIndex) {
  return (uint16_t)instrumentId * 8 + noteIndex + 1;
}
