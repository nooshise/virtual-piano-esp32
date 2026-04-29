#pragma once

#include <Arduino.h>
#include "ProjectConfig.h"

// Small shared state model for the UI screens and per-key configuration.
enum Screen {
  SCR_MAIN,
  SCR_CFG,
  SCR_INST,
  SCR_COLOR
};

struct KeyCfg {
  uint8_t inst;
  uint8_t note;
  uint8_t colorIdx;
};

extern KeyCfg keyCfg[NUM_KEYS];
extern uint32_t lastTrigMs[NUM_KEYS];
extern Screen screen;
extern uint8_t selKey;

const char* screenName(Screen s);
void setScreen(Screen s);
