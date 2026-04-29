#pragma once

#include <Arduino.h>
#include "ProjectConfig.h"

extern HardwareSerial RF;

void rfBegin();
void rfTick();
void sendKeyColor(uint8_t keyIndex);
void sendAllColors();
