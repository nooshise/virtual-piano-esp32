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
extern uint8_t instrumentColor[INST_COUNT];
extern uint32_t lastTrigMs[NUM_KEYS];
extern Screen screen;
extern uint8_t selKey;
extern uint8_t colorPickInstrument;
extern uint8_t pendingTakeColor;
extern uint8_t pendingTakeOwner;
extern bool autoKeyboardActive;
extern uint8_t autoInstrumentId;

const char* screenName(Screen s);
void setScreen(Screen s);
void updateAutoKeyboardMode();
uint8_t effectiveChoiceForKey(uint8_t keyIndex);
uint8_t getInstrumentColorOrNone(uint8_t inst);
uint8_t getColorOwner(uint8_t colorIndex);
uint8_t colorForKey(uint8_t keyIndex);
