#pragma once

#include <Arduino.h>
#include "ProjectConfig.h"

// Names and palettes shared by drawing, RF color sync, and playback logs.
static const char* const INST_NAMES[INST_COUNT] = {
  "Piano", "Guitar", "Drums", "Trumpet", "Violin", "Accordn", "Oud", "Fan"
};

static const char* const NOTE_NAMES[NUM_KEYS] = {
  "Do", "Re", "Mi", "Fa", "Sol", "La", "Si", "Do2"
};

static const uint16_t COLORS565[NUM_COLORS] = {
  0xF800, 0x07E0, 0x001F, 0xFFE0, 0x07FF, 0xF81F, 0xFD20, 0xFFFF
};

static const char* const COLOR_NAMES[NUM_COLORS] = {
  "Red", "Grn", "Blue", "Yel", "Cyan", "Mag", "Org", "Wht"
};
