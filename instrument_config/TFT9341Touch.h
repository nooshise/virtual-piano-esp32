#pragma once
// tft9341touch: ILI9341 TFT plus raw SPI resistive touch wrapper.
// The touch path follows the known-working TFT9341Touch examples: TIRQ is
// active-low, and X/Y are read directly with ADS/XPT-style SPI commands.

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

#define TFT_MAX_BUTTONS 10

class tft9341touch : public Adafruit_ILI9341 {
public:
  int16_t xTouch = 0;
  int16_t yTouch = 0;
  int16_t xRaw   = 0;
  int16_t yRaw   = 0;

  tft9341touch(uint8_t cs, uint8_t dc, uint8_t tcs, uint8_t tirq)
    : Adafruit_ILI9341(cs, dc),
      _tftCs(cs),
      _tcs(tcs),
      _tirq(tirq)
  {}

  bool begin() {
    pinMode(_tcs, OUTPUT);
    pinMode(_tirq, INPUT_PULLUP);
    digitalWrite(_tcs, HIGH);

    pinMode(_tftCs, OUTPUT);
    digitalWrite(_tftCs, HIGH);
    SPI.begin();

    Adafruit_ILI9341::begin();
    setRotation(3);       // landscape 320x240 for the project UI
    fillScreen(0x0000);

    digitalWrite(_tftCs, HIGH);
    digitalWrite(_tcs, HIGH);
    _touchOk = true;      // raw SPI touch has no controller init transaction
    return _touchOk;
  }

  void setTouch(int16_t x1, int16_t x2, int16_t y1, int16_t y2) {
    _rx1=x1; _rx2=x2; _ry1=y1; _ry2=y2;
  }

  bool touched() {
    if (!_touchOk) return false;
    return digitalRead(_tirq) == LOW;
  }

  void readTouch() {
    xTouch = -1;
    yTouch = -1;
    if (!_touchOk || !touched()) return;

    uint16_t rawX = readStableAxis(0xD0);
    uint16_t rawY = readStableAxis(0x90);
    xRaw = (int16_t)rawX;
    yRaw = (int16_t)rawY;
    if (rawX == 0 && rawY == 0) return;

    // Match the reference library's default rotation-2 touch transform while
    // keeping Adafruit_ILI9341 rotation 3 so the existing UI remains landscape.
    int16_t mappedX = (int16_t)map((long)rawY, (long)_ry1, (long)_ry2,
                                   10L, (long)(width() - 10));
    int16_t mappedYBase = (int16_t)map((long)rawX, (long)_rx1, (long)_rx2,
                                       10L, (long)(height() - 10));
    int16_t mappedY = (int16_t)((height() - 1) - mappedYBase);

    xTouch = clampToDisplay(mappedX, width());
    yTouch = clampToDisplay(mappedY, height());
  }

  void drainFifo() {
    // The reference touch controller path has no FIFO to drain.
  }

  bool touchOk() { return _touchOk; }

  bool readRawPoint(int16_t &rx, int16_t &ry) {
    if (!_touchOk || !touched()) return false;
    rx = (int16_t)readStableAxis(0xD0);
    ry = (int16_t)readStableAxis(0x90);
    xRaw = rx;
    yRaw = ry;
    return true;
  }

  void print(int16_t x, int16_t y, const char* str, uint8_t scale, uint16_t fg, uint16_t bg) {
    setCursor(x,y); setTextColor(fg,bg); setTextSize(scale);
    Adafruit_GFX::print(str);
  }
  using Print::print;

  void drawButton(uint8_t id,
                  int16_t x, int16_t y, int16_t w, int16_t h, int16_t r,
                  uint16_t border, uint16_t fill,
                  const char* label, uint8_t scale) {
    fillRoundRect(x,y,w,h,r,fill);
    drawRoundRect(x,y,w,h,r,border);
    int16_t tw=(int16_t)(strlen(label)*6*scale);
    int16_t th=(int16_t)(8*scale);
    setCursor(x+w/2-tw/2, y+h/2-th/2);
    setTextColor(border,fill); setTextSize(scale);
    Adafruit_GFX::print(label);
    for (uint8_t i=0;i<TFT_MAX_BUTTONS;i++) {
      if (!_btnUsed[i] || _btnId[i]==id) {
        _btnUsed[i]=true; _btnId[i]=id;
        _btnX[i]=x; _btnY[i]=y; _btnW[i]=w; _btnH[i]=h;
        return;
      }
    }
  }

  int8_t ButtonTouch(int16_t x, int16_t y) {
    for (uint8_t i=0;i<TFT_MAX_BUTTONS;i++) {
      if (_btnUsed[i] && x>=_btnX[i] && x<(_btnX[i]+_btnW[i]) &&
                         y>=_btnY[i] && y<(_btnY[i]+_btnH[i]))
        return (int8_t)_btnId[i];
    }
    return 0;
  }

  void clearButtons() {
    for (uint8_t i=0;i<TFT_MAX_BUTTONS;i++) _btnUsed[i]=false;
  }

private:
  static const uint32_t TOUCH_SPI_FREQUENCY = 1000000;

  uint16_t readAd(uint8_t command) {
    digitalWrite(_tftCs, HIGH);
    digitalWrite(_tcs, LOW);
    SPI.transfer(command);
    uint8_t data1 = SPI.transfer(0x00);
    uint8_t data2 = SPI.transfer(0x00);
    digitalWrite(_tcs, HIGH);
    return (uint16_t)((((uint16_t)data1 << 8) + data2) >> 3) & 0x0FFF;
  }

  uint16_t readStableAxis(uint8_t command) {
    uint16_t a = 0, b = 0, c = 0;
    SPI.beginTransaction(SPISettings(TOUCH_SPI_FREQUENCY, MSBFIRST, SPI_MODE0));
    for (uint8_t attempt=0; attempt<12; attempt++) {
      a = readAd(command);
      delayMicroseconds(5);
      b = readAd(command);
      delayMicroseconds(5);
      c = readAd(command);
      if (closeEnough(a, b) && closeEnough(a, c)) break;
    }
    SPI.endTransaction();
    digitalWrite(_tcs, HIGH);
    return (uint16_t)(((uint32_t)a + b + c) / 3);
  }

  static bool closeEnough(uint16_t a, uint16_t b) {
    return (a > b) ? ((a - b) < 10) : ((b - a) < 10);
  }

  static int16_t clampToDisplay(int16_t value, int16_t limit) {
    if (value < 0) return 0;
    if (value >= limit) return limit - 1;
    return value;
  }

  uint8_t  _tftCs;
  uint8_t  _tcs;
  uint8_t  _tirq;
  bool     _touchOk = false;
  int16_t  _rx1=0, _ry1=0, _rx2=4095, _ry2=4095;
  bool    _btnUsed[TFT_MAX_BUTTONS]={};
  uint8_t _btnId  [TFT_MAX_BUTTONS]={};
  int16_t _btnX   [TFT_MAX_BUTTONS]={};
  int16_t _btnY   [TFT_MAX_BUTTONS]={};
  int16_t _btnW   [TFT_MAX_BUTTONS]={};
  int16_t _btnH   [TFT_MAX_BUTTONS]={};
};
