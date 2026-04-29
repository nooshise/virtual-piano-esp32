#pragma once

#include <Arduino.h>

extern HardwareSerial RF;

void rfBegin();
void rfTick();
void rfSendTrig(uint8_t s1to8, uint16_t mm);
