#pragma once

#include <Arduino.h>

void i2cBegin();
bool tcaPingWithRetry();
bool tcaSelect(uint8_t ch);
bool i2cPing(uint8_t addr);
bool i2cReg16(uint8_t addr, uint16_t reg, uint8_t &val);
void i2cRecover();
