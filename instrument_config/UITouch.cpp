#include "UITouch.h"

#include <Arduino.h>
#include "Instruments.h"
#include "ProjectConfig.h"
#include "RFProtocol.h"
#include "UIDraw.h"
#include "UIState.h"

static void waitRelease() {
  while (lcd.touched()) delay(10);
  delay(50);
}

static void sendColorsForInstrument(uint8_t inst) {
  for (uint8_t k = 0; k < NUM_KEYS; k++) {
    if (keyCfg[k].inst == inst) sendKeyColor(k);
  }
}

void touchTick() {
  if (!lcd.touched()) return;
  delay(30);
  lcd.readTouch();
  int16_t x = lcd.xTouch;
  int16_t y = lcd.yTouch;

  Serial.print("[TOUCH] mapped=");
  Serial.print(x);
  Serial.print(",");
  Serial.print(y);
  Serial.print(" screen=");
  Serial.println(screenName(screen));

  if (screen == SCR_MAIN) {
    int k = hitKey(x, y);
    if (k >= 0) {
      Serial.print("[HIT] key=");
      Serial.println(k + 1);
      selKey = (uint8_t)k;
      drawInstPicker(selKey);
    }
    waitRelease();
    return;
  }

  if (screen == SCR_INST) {
    int8_t btn = lcd.ButtonTouch(x, y);
    if (btn == BTN_BACK) {
      drawMain();
      waitRelease();
      return;
    }
    int inst = hitInst(x, y);
    if (inst >= 0) {
      keyCfg[selKey].inst = (uint8_t)inst;
      keyCfg[selKey].note = 0;
      updateAutoKeyboardMode();
      drawCfg(selKey);
      sendKeyColor(selKey);
    }
    waitRelease();
    return;
  }

  if (screen == SCR_CFG) {
    if (x >= COLOR_BOX_X && x <= (COLOR_BOX_X + COLOR_BOX_W) &&
        y >= COLOR_BOX_Y && y <= (COLOR_BOX_Y + COLOR_BOX_H)) {
      colorPickInstrument = keyCfg[selKey].inst;
      drawColorPicker(colorPickInstrument);
      waitRelease();
      return;
    }

    int8_t btn = lcd.ButtonTouch(x, y);
    if (btn <= 0) {
      waitRelease();
      return;
    }
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

    updateAutoKeyboardMode();
    bool lockChoice = (autoKeyboardActive && keyCfg[selKey].inst != 2);
    if (!lockChoice) {
      if (btn == BTN_NOTE_M) {
        keyCfg[selKey].note = (keyCfg[selKey].note == 0) ? 7 : (keyCfg[selKey].note - 1);
        drawCfg(selKey);
      } else if (btn == BTN_NOTE_P) {
        keyCfg[selKey].note = (keyCfg[selKey].note + 1) & 0x07;
        drawCfg(selKey);
      }
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

    if (btn == BTN_RELEASE) {
      if (pendingTakeColor != NO_COLOR && pendingTakeOwner != 255 && pendingTakeOwner < INST_COUNT) {
        uint8_t oldOwner = pendingTakeOwner;
        instrumentColor[pendingTakeOwner] = NO_COLOR;
        instrumentColor[colorPickInstrument] = pendingTakeColor;
        pendingTakeColor = NO_COLOR;
        pendingTakeOwner = 255;
        drawColorPicker(colorPickInstrument);
        showColorMsg("Released + assigned");
        sendColorsForInstrument(oldOwner);
        sendColorsForInstrument(colorPickInstrument);
        waitRelease();
        return;
      }

      instrumentColor[colorPickInstrument] = NO_COLOR;
      drawColorPicker(colorPickInstrument);
      showColorMsg("Color released");
      sendColorsForInstrument(colorPickInstrument);
      waitRelease();
      return;
    }

    int c = hitColor(x, y);
    if (c >= 0) {
      uint8_t current = getInstrumentColorOrNone(colorPickInstrument);
      uint8_t owner = getColorOwner((uint8_t)c);

      if (current != NO_COLOR && current == (uint8_t)c) {
        showColorMsg("Already selected");
        waitRelease();
        return;
      }

      if (owner != 255 && owner != colorPickInstrument) {
        pendingTakeColor = (uint8_t)c;
        pendingTakeOwner = owner;
        char msg[80];
        snprintf(msg, sizeof(msg), "IN USE by %s -> RELEASE", INST_NAMES[owner]);
        showColorMsg(msg);
        waitRelease();
        return;
      }

      instrumentColor[colorPickInstrument] = (uint8_t)c;
      drawColorPicker(colorPickInstrument);
      showColorMsg("Color assigned");
      sendColorsForInstrument(colorPickInstrument);
      waitRelease();
      return;
    }

    waitRelease();
    return;
  }

  waitRelease();
}
