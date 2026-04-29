#include "UIDraw.h"

#include <pgmspace.h>
#include "DFPlayers.h"
#include "Icons.h"
#include "Instruments.h"
#include "ProjectConfig.h"
#include "UIState.h"

static const int KB_X0 = 10;
static const int KB_Y0 = 55;
static const int KB_W = 300;
static const int KB_H = 155;
static const int ICON_COLS = 4;
static const int CP_W = 60;
static const int CP_H = 38;

static int keyX[NUM_KEYS];
static int keyW[NUM_KEYS];

const char* screenName(Screen s) {
  switch (s) {
    case SCR_MAIN: return "MAIN";
    case SCR_CFG: return "CFG";
    case SCR_INST: return "INST";
    case SCR_COLOR: return "COLOR";
  }
  return "?";
}

void setScreen(Screen s) {
  screen = s;
  Serial.print("[SCREEN] name=");
  Serial.println(screenName(screen));
}

static void uiButton(uint8_t id, int16_t x, int16_t y, int16_t w, int16_t h, int16_t r,
                     uint16_t textColor, uint16_t fillColor, const char* label, uint8_t textSize) {
  lcd.drawButton(id, x, y, w, h, r, fillColor, textColor, (char*)label, textSize);
  lcd.drawRoundRect(x, y, w, h, r, textColor);
}

static void drawBmp565(int x, int y, int w, int h, const uint16_t* data) {
  for (int j = 0; j < h; j++) {
    for (int i = 0; i < w; i++) {
      lcd.drawPixel(x + i, y + j, pgm_read_word(&data[j * w + i]));
    }
  }
}

void calcLayout() {
  int base = KB_W / NUM_KEYS;
  int rem = KB_W % NUM_KEYS;
  int x = KB_X0;
  for (uint8_t i = 0; i < NUM_KEYS; i++) {
    int w = base + ((i < rem) ? 1 : 0);
    keyX[i] = x;
    keyW[i] = w;
    x += w;
  }
}

int hitKey(int16_t tx, int16_t ty) {
  if (ty < KB_Y0 - 30 || ty > KB_Y0 + KB_H + 30) return -1;
  for (uint8_t i = 0; i < NUM_KEYS; i++) {
    if (tx >= keyX[i] && tx < (keyX[i] + keyW[i])) return i;
  }
  if (tx < keyX[0]) return 0;
  return NUM_KEYS - 1;
}

static void iconPos(uint8_t idx, int &x, int &y) {
  uint8_t col = idx % ICON_COLS;
  uint8_t row = idx / ICON_COLS;
  int mx = (320 - ICON_COLS * ICON_W) / (ICON_COLS + 1);
  if (mx < 5) mx = 5;
  x = mx + col * (ICON_W + mx);
  y = 28 + row * (ICON_H + 12);
}

int hitInst(int16_t tx, int16_t ty) {
  for (uint8_t i = 0; i < INST_COUNT; i++) {
    int x, y;
    iconPos(i, x, y);
    if (tx >= x && tx < x + ICON_W && ty >= y && ty < y + ICON_H) return i;
  }
  return -1;
}

static void cpPos(uint8_t idx, int &x, int &y) {
  x = 10 + (idx % 4) * (CP_W + 10);
  y = 30 + (idx / 4) * (CP_H + 16);
}

int hitColor(int16_t tx, int16_t ty) {
  for (uint8_t c = 0; c < NUM_COLORS; c++) {
    int x, y;
    cpPos(c, x, y);
    if (tx >= x && tx < x + CP_W && ty >= y && ty < y + CP_H) return c;
  }
  return -1;
}

void drawTile(uint8_t k) {
  if (k >= NUM_KEYS) return;
  int x = keyX[k];
  int w = keyW[k];
  int y = KB_Y0;
  int h = KB_H;
  uint16_t barCol = (keyCfg[k].colorIdx < NUM_COLORS) ? COLORS565[keyCfg[k].colorIdx] : VP_DARKGREY;
  int mc = (w - 4) / 6;
  if (mc < 1) mc = 1;
  if (mc > 7) mc = 7;
  char b[8];

  lcd.fillRect(x, y, w, h, VP_WHITE);
  lcd.drawRect(x, y, w, h, VP_BLACK);
  lcd.fillRect(x + 1, y + 1, w - 2, 12, barCol);
  strncpy(b, INST_NAMES[keyCfg[k].inst], mc);
  b[mc] = 0;
  lcd.print(x + 2, y + 18, b, 1, VP_BLACK, VP_WHITE);
  strncpy(b, NOTE_NAMES[keyCfg[k].note], mc);
  b[mc] = 0;
  lcd.print(x + 2, y + 34, b, 1, VP_RED, VP_WHITE);
}

void drawMain() {
  lcd.clearButton();
  calcLayout();
  lcd.fillScreen(VP_BLACK);
  lcd.print(10, 10, (char*)"VIRTUAL PIANO", 2, VP_YELLOW, VP_BLACK);
  lcd.print(10, 32, (char*)(dfIsReady() ? "DF:READY" : "DF:INIT..."), 1, VP_CYAN, VP_BLACK);
  lcd.drawRect(KB_X0 - 2, KB_Y0 - 2, KB_W + 4, KB_H + 4, VP_CYAN);
  for (uint8_t k = 0; k < NUM_KEYS; k++) drawTile(k);
  lcd.print(10, 218, (char*)"Tap a tile to configure", 1, VP_GREEN, VP_BLACK);
  setScreen(SCR_MAIN);
}

void drawCfg(uint8_t k) {
  if (k >= NUM_KEYS) return;
  lcd.clearButton();
  lcd.fillScreen(VP_BLACK);
  char hdr[24];
  snprintf(hdr, sizeof(hdr), "KEY %u CONFIG", (unsigned)(k + 1));
  lcd.print(10, 8, hdr, 2, VP_YELLOW, VP_BLACK);
  lcd.print(10, 38, (char*)INST_NAMES[keyCfg[k].inst], 2, VP_WHITE, VP_BLACK);
  char b[16];
  snprintf(b, sizeof(b), "Note: %s", NOTE_NAMES[keyCfg[k].note]);
  lcd.print(10, 65, b, 2, VP_CYAN, VP_BLACK);
  uint16_t barCol = (keyCfg[k].colorIdx < NUM_COLORS) ? COLORS565[keyCfg[k].colorIdx] : VP_DARKGREY;
  lcd.fillRect(222, 38, 70, 28, barCol);
  lcd.drawRect(222, 38, 70, 28, VP_WHITE);
  const char* cn = (keyCfg[k].colorIdx < NUM_COLORS) ? COLOR_NAMES[keyCfg[k].colorIdx] : "None";
  lcd.print(224, 70, (char*)cn, 1, VP_CYAN, VP_BLACK);
  uiButton(BTN_NOTE_M, 10, 100, 55, 30, 3, VP_RED, VP_WHITE, "-", 2);
  uiButton(BTN_NOTE_P, 75, 100, 55, 30, 3, VP_GREEN, VP_WHITE, "+", 2);
  uiButton(BTN_INST, 10, 145, 145, 30, 3, VP_WHITE, VP_BLACK, "INSTRUMENT", 1);
  uiButton(BTN_COLOR, 165, 145, 145, 30, 3, VP_MAGENTA, VP_BLACK, "COLOR", 1);
  uiButton(BTN_BACK, 110, 192, 100, 30, 4, VP_CYAN, VP_BLACK, "BACK", 2);
  setScreen(SCR_CFG);
}

void drawInstPicker(uint8_t k) {
  if (k >= NUM_KEYS) return;
  lcd.clearButton();
  lcd.fillScreen(VP_BLACK);
  char hdr[32];
  snprintf(hdr, sizeof(hdr), "KEY %u: Instrument", (unsigned)(k + 1));
  lcd.print(10, 6, hdr, 1, VP_YELLOW, VP_BLACK);
  for (uint8_t i = 0; i < INST_COUNT; i++) {
    int x, y;
    iconPos(i, x, y);
    uint16_t border = (i == keyCfg[k].inst) ? VP_GREEN : VP_WHITE;
    lcd.fillRect(x, y, ICON_W, ICON_H, VP_DARKGREY);
    lcd.drawRect(x - 2, y - 2, ICON_W + 4, ICON_H + 4, border);
    if (INST_ICONS[i]) drawBmp565(x, y, ICON_W, ICON_H, INST_ICONS[i]);
    lcd.fillRect(x, y + ICON_H - 10, ICON_W, 10, VP_BLACK);
    lcd.print(x + 2, y + ICON_H - 9, (char*)INST_NAMES[i], 1, VP_WHITE, VP_BLACK);
  }
  uiButton(BTN_BACK, 110, 212, 100, 24, 4, VP_CYAN, VP_BLACK, "BACK", 2);
  setScreen(SCR_INST);
}

void drawColorPicker(uint8_t k) {
  if (k >= NUM_KEYS) return;
  lcd.clearButton();
  lcd.fillScreen(VP_BLACK);
  char hdr[24];
  snprintf(hdr, sizeof(hdr), "KEY %u: Color", (unsigned)(k + 1));
  lcd.print(10, 8, hdr, 1, VP_YELLOW, VP_BLACK);
  lcd.print(10, 18, (char*)"Tap NONE to release, then pick", 1, VP_DARKGREY, VP_BLACK);
  for (uint8_t c = 0; c < NUM_COLORS; c++) {
    int x, y;
    cpPos(c, x, y);
    uint16_t border = (keyCfg[k].colorIdx == c) ? VP_GREEN : VP_WHITE;
    lcd.fillRect(x, y, CP_W, CP_H, COLORS565[c]);
    lcd.drawRect(x - 2, y - 2, CP_W + 4, CP_H + 4, border);
    lcd.print(x + 2, y + CP_H + 2, (char*)COLOR_NAMES[c], 1, VP_WHITE, VP_BLACK);
  }
  uiButton(BTN_NONE, 10, 152, 80, 26, 3, VP_DARKGREY, VP_WHITE, "NONE", 1);
  uiButton(BTN_BACK, 110, 152, 100, 26, 4, VP_CYAN, VP_BLACK, "BACK", 2);
  setScreen(SCR_COLOR);
}
