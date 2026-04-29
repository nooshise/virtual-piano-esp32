#pragma once

#include <Arduino.h>
#include "SensorConfig.h"

typedef enum {
  ST_NONE = 0,
  ST_VL53 = 1,
  ST_VL618 = 2
} SensorType;

struct ChanState {
  SensorType type = ST_NONE;
  bool ok = false;
  bool nearState = false;
  uint8_t nearCnt = 0;
  uint8_t farCnt = 0;
  bool armed = true;
  uint8_t failStreak = 0;
  uint32_t lastReadMs = 0;
  uint32_t lastLogMs = 0;
  uint32_t lastTrigMs = 0;
  uint16_t baseline = 200;
  uint16_t enterThr = ENTER_FIXED;
  uint16_t exitThr = EXIT_FIXED;
  uint16_t minValid = MIN_VL53;
};

extern ChanState chs[NCH];

void initAllSensors();
void calibrateAllSensors();
bool readMm(uint8_t ch, uint16_t &mm);
void recoverChannel(uint8_t ch);
