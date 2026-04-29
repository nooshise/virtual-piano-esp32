#pragma once

#include <Arduino.h>

void dfInitStart();
void dfInitTick();
bool dfIsReady();
void dfPlayTrack(uint8_t playerIndex, uint16_t track);
