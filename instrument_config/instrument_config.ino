#include <Arduino.h>
#include <TFT9341Touch.h>

#include "DFPlayers.h"
#include "ProjectConfig.h"
#include "RFProtocol.h"
#include "UIDraw.h"
#include "UIState.h"
#include "UITouch.h"

tft9341touch lcd(TFT_CS_PIN, TFT_DC_PIN, TOUCH_CS_PIN, TOUCH_IRQ_PIN);

KeyCfg keyCfg[NUM_KEYS];
uint32_t lastTrigMs[NUM_KEYS] = {0};
Screen screen = SCR_MAIN;
uint8_t selKey = 0;

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n=== VIRTUAL PIANO UI ===");

  Serial.println("[TFT] begin");
  lcd.begin();
  lcd.setTouch(3780, 372, 489, 3811);
  Serial.println("[TOUCH] using external library");

  for (uint8_t k = 0; k < NUM_KEYS; k++) {
    keyCfg[k].inst = k;
    keyCfg[k].note = k & 7;
    keyCfg[k].colorIdx = k % NUM_COLORS;
  }
  calcLayout();

  rfBegin();
  delay(300);
  sendAllColors();

  drawMain();
  dfInitStart();
}

void loop() {
  rfTick();
  dfInitTick();
  touchTick();

  static bool dfShown = false;
  if (dfIsReady() && !dfShown) {
    dfShown = true;
    if (screen == SCR_MAIN) {
      lcd.fillRect(10, 32, 120, 12, VP_BLACK);
      lcd.print(10, 32, (char*)"DF:READY", 1, VP_CYAN, VP_BLACK);
    }
  }

  static uint32_t colorSyncMs = 0;
  if (millis() - colorSyncMs >= 15000) {
    colorSyncMs = millis();
    sendAllColors();
  }

  static uint32_t hbMs = 0;
  static bool hb = false;
  if (millis() - hbMs >= 500) {
    hbMs = millis();
    hb = !hb;
    lcd.fillRect(305, 10, 10, 10, hb ? VP_GREEN : VP_RED);
  }

  delay(0);
}
