#pragma once

#include <Arduino.h>
#include <TFT9341Touch.h>

// UI board wiring and shared compile-time settings.
static const uint8_t TFT_CS_PIN = 5;
static const uint8_t TFT_DC_PIN = 4;
static const uint8_t TOUCH_CS_PIN = 15;
static const uint8_t TOUCH_IRQ_PIN = 35;

static const uint32_t RF_BAUD = 9600;
static const uint8_t RF_RX_PIN = 34;
static const uint8_t RF_TX_PIN = 13;

static const uint8_t NUM_KEYS = 8;
static const uint8_t INST_COUNT = 8;
static const uint8_t NUM_COLORS = 8;
static const uint8_t NO_COLOR = 255;

static const uint8_t N_PLAYERS = 8;
static const int DF_TX_PINS[N_PLAYERS] = {16, 17, 21, 22, 26, 27, 32, 33};
static const int DF_RX_DUMMY = 39;
static const uint8_t DF_VOLUME = 28;
static const uint16_t DF_GAP_MS = 70;
static const uint32_t DF_BOOT_MS = 6000;

static const uint16_t MIN_TRIG_MS = 120;

static const uint16_t VP_BLACK = 0x0000;
static const uint16_t VP_WHITE = 0xFFFF;
static const uint16_t VP_RED = 0xF800;
static const uint16_t VP_GREEN = 0x07E0;
static const uint16_t VP_BLUE = 0x001F;
static const uint16_t VP_YELLOW = 0xFFE0;
static const uint16_t VP_CYAN = 0x07FF;
static const uint16_t VP_MAGENTA = 0xF81F;
static const uint16_t VP_DARKGREY = 0x7BEF;

extern tft9341touch lcd;
