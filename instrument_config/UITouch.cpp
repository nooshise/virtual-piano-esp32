#include "UITouch.h"

#include <Arduino.h>
#include "Instruments.h"
#include "ProjectConfig.h"
#include "RFProtocol.h"
#include "UIDraw.h"
#include "UIState.h"

static void waitRelease() {
  uint8_t clearCnt = 0;
  uint32_t t0 = millis();
  while (millis() - t0 < 1200) {
    delay(30);
    if (!lcd.touched()) {
      if (++clearCnt >= 3) return;
    } else {
      clearCnt = 0;
    }
  }
}

static void checkAutoNotes() {
  uint8_t inst0 = keyCfg[0].inst;
  for (uint8_t k = 1; k < NUM_KEYS; k++) {
    if (keyCfg[k].inst != inst0) return;
  }
  for (uint8_t k = 0; k < NUM_KEYS; k++) keyCfg[k].note = k & 7;
  Serial.println("Auto-notes assigned (all instruments equal)");
}

void touchTick() {
  if (!lcd.touched()) return;
  delay(30);
  if (!lcd.touched()) return;

  lcd.readTouch();
  int16_t x = lcd.xTouch;
  int16_t y = lcd.yTouch;
  Serial.print("[TOUCH] mapped=");
  Serial.print(x);
  Serial.print(",");
  Serial.print(y);
  Serial.print(" screen=");
  Serial.println(screenName(screen));

  if (x < 0 || y < 0) {
    waitRelease();
    return;
  }

  if (screen == SCR_MAIN) {
    int k = hitKey(x, y);
    if (k >= 0) {
      Serial.print("[HIT] key=");
      Serial.println(k + 1);
      selKey = (uint8_t)k;
      drawCfg(selKey);
    }
    waitRelease();
    return;
  }

  if (screen == SCR_CFG) {
    int8_t btn = lcd.ButtonTouch(x, y);
    if (btn == BTN_BACK) {
      drawMain();
      waitRelease();
      return;
    }
    if (btn == BTN_INST) {
      drawInstPicker(selKey);
      waitRelease();
      return;
    }
    if (btn == BTN_COLOR) {
      drawColorPicker(selKey);
      waitRelease();
      return;
    }
    if (btn == BTN_NOTE_M) {
      keyCfg[selKey].note = (keyCfg[selKey].note == 0) ? 7 : (keyCfg[selKey].note - 1);
      drawCfg(selKey);
      waitRelease();
      return;
    }
    if (btn == BTN_NOTE_P) {
      keyCfg[selKey].note = (keyCfg[selKey].note + 1) & 7;
      drawCfg(selKey);
      waitRelease();
      return;
    }
    waitRelease();
    return;
  }

  if (screen == SCR_INST) {
    int8_t btn = lcd.ButtonTouch(x, y);
    if (btn == BTN_BACK) {
      drawCfg(selKey);
      waitRelease();
      return;
    }
    int inst = hitInst(x, y);
    if (inst >= 0) {
      keyCfg[selKey].inst = (uint8_t)inst;
      checkAutoNotes();
      drawCfg(selKey);
    }
    waitRelease();
    return;
  }

  if (screen == SCR_COLOR) {
    int8_t btn = lcd.ButtonTouch(x, y);
    if (btn == BTN_BACK) {
      drawCfg(selKey);
      waitRelease();
      return;
    }
    if (btn == BTN_NONE) {
      keyCfg[selKey].colorIdx = NO_COLOR;
      sendKeyColor(selKey);
      drawColorPicker(selKey);
      waitRelease();
      return;
    }
    int c = hitColor(x, y);
    if (c >= 0) {
      keyCfg[selKey].colorIdx = (uint8_t)c;
      sendKeyColor(selKey);
      drawColorPicker(selKey);
    }
    waitRelease();
    return;
  }

  waitRelease();
}
