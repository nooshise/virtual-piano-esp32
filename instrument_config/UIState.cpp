#include "UIState.h"

uint8_t getInstrumentColorOrNone(uint8_t inst) {
  if (inst >= INST_COUNT) return NO_COLOR;
  return instrumentColor[inst];
}

uint8_t getColorOwner(uint8_t colorIndex) {
  for (uint8_t i = 0; i < INST_COUNT; i++) {
    if (instrumentColor[i] == colorIndex) return i;
  }
  return 255;
}

void updateAutoKeyboardMode() {
  uint8_t inst0 = keyCfg[0].inst;
  for (uint8_t k = 1; k < NUM_KEYS; k++) {
    if (keyCfg[k].inst != inst0) {
      autoKeyboardActive = false;
      return;
    }
  }
  if (inst0 == 2) {
    autoKeyboardActive = false;
    return;
  }
  autoKeyboardActive = true;
  autoInstrumentId = inst0;
}

uint8_t effectiveChoiceForKey(uint8_t keyIndex) {
  if (keyIndex >= NUM_KEYS) return 0;
  if (autoKeyboardActive) return keyIndex & 0x07;
  return keyCfg[keyIndex].note & 0x07;
}

uint8_t colorForKey(uint8_t keyIndex) {
  if (keyIndex >= NUM_KEYS) return NO_COLOR;
  return getInstrumentColorOrNone(keyCfg[keyIndex].inst);
}
