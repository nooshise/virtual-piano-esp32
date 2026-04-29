#include "UIDraw.h"

#include <pgmspace.h>
#include "DFPlayers.h"
#include "Icons.h"
#include "Instruments.h"
#include "ProjectConfig.h"
#include "UIState.h"

static const int KB_X0 = 10;
static const int KB_Y0 = 60;
static const int KB_W = 300;
static const int KB_H = 140;
static const int ICON_COLS = 4;
static const int CP_W = 60;
static const int CP_H = 40;

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
                     uint16_t fillColor, uint16_t textColor, const char* label, uint8_t textSize) {
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
  if (ty < KB_Y0 || ty > (KB_Y0 + KB_H)) return -1;
  if (tx < KB_X0 || tx > (KB_X0 + KB_W)) return -1;
  for (uint8_t i = 0; i < NUM_KEYS; i++) {
    if (tx >= keyX[i] && tx < (keyX[i] + keyW[i])) return i;
  }
  return -1;
}

static void iconPos(uint8_t idx, int &x, int &y) {
  uint8_t row = idx / ICON_COLS;
  uint8_t col = idx % ICON_COLS;
  int marginX = (320 - ICON_COLS * ICON_W) / (ICON_COLS + 1);
  if (marginX < 5) marginX = 5;
  x = marginX + col * (ICON_W + marginX);
  y = 50 + row * (ICON_H + 18);
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
  uint8_t row = idx / 4;
  uint8_t col = idx % 4;
  x = 10 + col * (CP_W + 10);
  y = 60 + row * (CP_H + 12);
}

int hitColor(int16_t tx, int16_t ty) {
  for (uint8_t c = 0; c < NUM_COLORS; c++) {
    int x, y;
    cpPos(c, x, y);
    if (tx >= x && tx < x + CP_W && ty >= y && ty < y + CP_H) return c;
  }
  return -1;
}

static const char* choiceLabel(uint8_t keyIndex) {
  uint8_t inst = keyCfg[keyIndex].inst;
  uint8_t ch = effectiveChoiceForKey(keyIndex);
  return (inst == 2) ? DRUM_NAMES[ch] : NOTE_NAMES[ch];
}

void drawTile(uint8_t k) {
  if (k >= NUM_KEYS) return;
  uint8_t inst = keyCfg[k].inst;
  uint8_t ci = getInstrumentColorOrNone(inst);
  uint16_t bar = (ci == NO_COLOR) ? VP_DARKGREY : COLORS565[ci % NUM_COLORS];
  int x = keyX[k];
  int w = keyW[k];
  int y = KB_Y0;
  int h = KB_H;

  lcd.fillRect(x, y, w, h, VP_WHITE);
  lcd.drawRect(x, y, w, h, VP_BLACK);
  lcd.fillRect(x + 1, y + 1, w - 2, 6, bar);
  lcd.drawRect(x + 1, y + 1, w - 2, 6, VP_BLACK);

  lcd.fillRect(x + 2, y + 12, w - 4, 14, VP_WHITE);
  lcd.print(x + 2, y + 12, (char*)INST_NAMES[inst], 1, VP_BLACK, VP_WHITE);

  lcd.fillRect(x + 2, y + h - 14, w - 4, 12, VP_WHITE);
  lcd.print(x + 2, y + h - 14, (char*)choiceLabel(k), 1, VP_BLACK, VP_WHITE);
}

void drawMain() {
  lcd.clearButton();
  updateAutoKeyboardMode();
  calcLayout();
  lcd.fillScreen(VP_BLACK);
  lcd.print(10, 10, (char*)"VIRTUAL PIANO (8 KEYS)", 2, VP_YELLOW, VP_BLACK);
  if (autoKeyboardActive) {
    char buf[48];
    snprintf(buf, sizeof(buf), "AUTO: %s", INST_NAMES[autoInstrumentId]);
    lcd.print(10, 30, buf, 2, VP_GREEN, VP_BLACK);
  } else {
    lcd.print(10, 30, (char*)"Tap key -> choose instrument", 1, VP_GREEN, VP_BLACK);
  }
  lcd.drawRect(KB_X0 - 2, KB_Y0 - 2, KB_W + 4, KB_H + 4, VP_CYAN);
  for (uint8_t k = 0; k < NUM_KEYS; k++) drawTile(k);
  lcd.print(10, 210, (char*)"Instrument color is LOCKED (no duplicates)", 1, VP_CYAN, VP_BLACK);
  setScreen(SCR_MAIN);
}

static void drawCfgValues(uint8_t k) {
  updateAutoKeyboardMode();
  uint8_t inst = keyCfg[k].inst;
  uint8_t ch = effectiveChoiceForKey(k);

  lcd.fillRect(10, 40, 220, 20, VP_BLACK);
  lcd.print(10, 40, (char*)INST_NAMES[inst], 2, VP_WHITE, VP_BLACK);

  char sbuf[32];
  snprintf(sbuf, sizeof(sbuf), "Sensor: %d", (int)(k + 1));
  lcd.fillRect(10, 130, 220, 16, VP_BLACK);
  lcd.print(10, 130, sbuf, 1, VP_GREEN, VP_BLACK);

  lcd.fillRect(10, 148, 260, 24, VP_BLACK);
  const char* label = (inst == 2) ? DRUM_NAMES[ch] : NOTE_NAMES[ch];
  char buf[48];
  if (autoKeyboardActive && inst != 2) snprintf(buf, sizeof(buf), "Choice: AUTO %s", label);
  else snprintf(buf, sizeof(buf), "Choice: %s", label);
  lcd.print(10, 148, buf, 2, VP_YELLOW, VP_BLACK);

  lcd.fillRect(200, 40, 120, 20, VP_BLACK);
  lcd.print(200, 40, (char*)"Color:", 2, VP_WHITE, VP_BLACK);

  uint8_t ci = getInstrumentColorOrNone(inst);
  if (ci == NO_COLOR) {
    lcd.fillRect(COLOR_BOX_X, COLOR_BOX_Y, COLOR_BOX_W, COLOR_BOX_H, VP_BLACK);
    lcd.drawRect(COLOR_BOX_X - 1, COLOR_BOX_Y - 1, COLOR_BOX_W + 2, COLOR_BOX_H + 2, VP_WHITE);
    lcd.print(COLOR_BOX_X + 5, COLOR_BOX_Y + 5, (char*)"NONE", 1, VP_WHITE, VP_BLACK);
    lcd.fillRect(COLOR_BOX_X - 20, COLOR_BOX_Y + COLOR_BOX_H + 2, 120, 16, VP_BLACK);
    lcd.print(COLOR_BOX_X - 20, COLOR_BOX_Y + COLOR_BOX_H + 2, (char*)"No color (tap)", 1, VP_CYAN, VP_BLACK);
  } else {
    lcd.fillRect(COLOR_BOX_X, COLOR_BOX_Y, COLOR_BOX_W, COLOR_BOX_H, COLORS565[ci % NUM_COLORS]);
    lcd.drawRect(COLOR_BOX_X - 1, COLOR_BOX_Y - 1, COLOR_BOX_W + 2, COLOR_BOX_H + 2, VP_WHITE);
    lcd.fillRect(COLOR_BOX_X - 20, COLOR_BOX_Y + COLOR_BOX_H + 2, 120, 16, VP_BLACK);
    lcd.print(COLOR_BOX_X - 20, COLOR_BOX_Y + COLOR_BOX_H + 2, (char*)COLOR_NAMES[ci], 1, VP_CYAN, VP_BLACK);
  }

  lcd.fillRect(200, 120, 120, 16, VP_BLACK);
  lcd.print(200, 120, (char*)"Tap color to pick", 1, VP_GREEN, VP_BLACK);
}

void drawCfg(uint8_t k) {
  if (k >= NUM_KEYS) return;
  lcd.clearButton();
  lcd.fillScreen(VP_BLACK);
  char hdr[24];
  snprintf(hdr, sizeof(hdr), "KEY %d CONFIG", (int)(k + 1));
  lcd.print(10, 10, hdr, 2, VP_YELLOW, VP_BLACK);

  uint8_t inst = keyCfg[k].inst;
  lcd.fillRect(10, 60, ICON_W, ICON_H, VP_DARKGREY);
  if (INST_ICONS[inst]) drawBmp565(10, 60, ICON_W, ICON_H, INST_ICONS[inst]);
  lcd.drawRect(8, 58, ICON_W + 4, ICON_H + 4, VP_WHITE);

  drawCfgValues(k);

  bool lockChoice = (autoKeyboardActive && inst != 2);
  uint16_t minusCol = lockChoice ? VP_DARKGREY : VP_RED;
  uint16_t plusCol = lockChoice ? VP_DARKGREY : VP_GREEN;
  uiButton(BTN_NOTE_M, 10, 180, 60, 30, 3, minusCol, VP_WHITE, "-", 2);
  uiButton(BTN_NOTE_P, 80, 180, 60, 30, 3, plusCol, VP_WHITE, "+", 2);
  uiButton(BTN_INST, 160, 180, 150, 30, 3, VP_WHITE, VP_BLACK, "INSTRUMENT", 1);
  uiButton(BTN_BACK, 110, 220, 100, 30, 4, VP_CYAN, VP_BLACK, "BACK", 2);
  lcd.print(10, 205, (char*)"Color is per-instrument (LOCK)", 1, VP_GREEN, VP_BLACK);
  setScreen(SCR_CFG);
}

void drawInstPicker(uint8_t k) {
  if (k >= NUM_KEYS) return;
  lcd.clearButton();
  lcd.fillScreen(VP_BLACK);
  char hdr[40];
  snprintf(hdr, sizeof(hdr), "KEY %d: Pick Instrument", (int)(k + 1));
  lcd.print(10, 10, hdr, 2, VP_YELLOW, VP_BLACK);

  for (uint8_t i = 0; i < INST_COUNT; i++) {
    int x, y;
    iconPos(i, x, y);
    uint16_t border = (i == keyCfg[k].inst) ? VP_GREEN : VP_WHITE;
    lcd.drawRect(x - 2, y - 2, ICON_W + 4, ICON_H + 4, border);
    lcd.fillRect(x, y, ICON_W, ICON_H, VP_DARKGREY);
    if (INST_ICONS[i]) drawBmp565(x, y, ICON_W, ICON_H, INST_ICONS[i]);
    lcd.print(x + 2, y + ICON_H + 5, (char*)INST_NAMES[i], 1, VP_CYAN, VP_BLACK);
  }

  uiButton(BTN_BACK, 110, 220, 100, 30, 4, VP_CYAN, VP_BLACK, "BACK", 2);
  setScreen(SCR_INST);
}

void showColorMsg(const char* msg) {
  lcd.fillRect(10, 190, 300, 14, VP_BLACK);
  lcd.print(10, 190, (char*)msg, 1, VP_YELLOW, VP_BLACK);
}

void drawColorPicker(uint8_t instrumentId) {
  if (instrumentId >= INST_COUNT) return;
  pendingTakeColor = NO_COLOR;
  pendingTakeOwner = 255;
  lcd.clearButton();
  lcd.fillScreen(VP_BLACK);

  char hdr[48];
  snprintf(hdr, sizeof(hdr), "Pick Color for %s", INST_NAMES[instrumentId]);
  lcd.print(10, 10, hdr, 2, VP_YELLOW, VP_BLACK);
  lcd.print(10, 32, (char*)"Green=Selected Red=In use White=Free", 1, VP_CYAN, VP_BLACK);

  uint8_t current = getInstrumentColorOrNone(instrumentId);
  for (uint8_t c = 0; c < NUM_COLORS; c++) {
    int x, y;
    cpPos(c, x, y);
    uint8_t owner = getColorOwner(c);
    bool selected = (current != NO_COLOR && current == c);
    uint16_t border = VP_WHITE;
    if (selected) border = VP_GREEN;
    else if (owner != 255 && owner != instrumentId) border = VP_RED;

    lcd.fillRect(x, y, CP_W, CP_H, COLORS565[c]);
    lcd.drawRect(x - 2, y - 2, CP_W + 4, CP_H + 4, border);
    lcd.fillRect(x, y + CP_H + 2, CP_W, 12, VP_BLACK);
    lcd.print(x, y + CP_H + 2, (char*)COLOR_NAMES[c], 1, VP_WHITE, VP_BLACK);
    if (owner != 255 && owner != instrumentId) {
      lcd.fillRect(x + 2, y + 2, CP_W - 4, 10, VP_BLACK);
      lcd.print(x + 2, y + 2, (char*)INST_NAMES[owner], 1, VP_WHITE, VP_BLACK);
    }
  }

  uiButton(BTN_RELEASE, 10, 205, 140, 30, 3, VP_RED, VP_WHITE, "RELEASE", 2);
  uiButton(BTN_BACK, 170, 205, 140, 30, 3, VP_CYAN, VP_BLACK, "BACK", 2);
  showColorMsg("Tip: tap a red color, then RELEASE");
  setScreen(SCR_COLOR);
}
