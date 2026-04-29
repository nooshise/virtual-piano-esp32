#pragma once

#include <Arduino.h>

// Sensor board wiring and behavior constants.
static const int I2C_SDA = 21;
static const int I2C_SCL = 22;
static const uint32_t I2C_HZ = 100000;
static const uint8_t TCA_ADDR = 0x70;

static const uint32_t RF_BAUD = 9600;
static const uint8_t RF_RX_PIN = 34;
static const uint8_t RF_TX_PIN = 13;

static const uint8_t NCH = 8;

static const uint8_t NEO_PIN = 25;
static const uint8_t NEO_COUNT = 8;
static const uint8_t NEO_BRIGHT = 80;

static const uint8_t CAL_SAMPLES = 8;
static const uint16_t CAL_DELAY_MS = 0;
static const float ENTER_RATIO = 0.20f;
static const float HYSTERESIS_RAT = 0.50f;
static const uint16_t DELTA_MIN = 12;
static const uint16_t DELTA_MAX = 80;
static const uint16_t ENTER_FIXED = 120;
static const uint16_t EXIT_FIXED = 160;

static const uint8_t NEAR_STABLE = 2;
static const uint8_t FAR_STABLE = 2;
static const uint16_t READ_PERIOD_MS = 30;
static const uint16_t DIST_LOG_MS = 750;
static const uint16_t MIN_TRIG_GAP = 150;
static const uint8_t FAIL_RECOVER = 10;

static const uint16_t MIN_VL53 = 50;
static const uint16_t MIN_VL618 = 10;
static const uint16_t TRIG_MAX_MM = 150;
