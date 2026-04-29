#pragma once

#include <Arduino.h>

void ledBegin();
void ledTick();
void ledSetColorIndex(uint8_t index, uint8_t colorIdx);
void ledTrigger(uint8_t index);
void ledShowActiveSensors(bool active[], uint8_t count);
void ledCalibrationWarning(bool active[], uint8_t count);
void ledFlashCalibrating(uint8_t index);
void ledClearOne(uint8_t index);
