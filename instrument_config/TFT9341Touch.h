#pragma once
// tft9341touch: ILI9341 TFT + STMPE610 touch wrapper
// TIRQ is active-LOW — do NOT use INPUT_PULLUP.
// KEY FIX: readTouch() drains entire FIFO (not just 1 entry) to prevent freeze.

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_STMPE610.h>

#define TFT_MAX_BUTTONS 10

class tft9341touch : public Adafruit_ILI9341 {
public:
  int16_t xTouch = 0;
  int16_t yTouch = 0;
  int16_t xRaw   = 0;   // last raw STMPE610 X — useful for calibration debug
  int16_t yRaw   = 0;

  tft9341touch(uint8_t cs, uint8_t dc, uint8_t tcs, uint8_t tirq)
    : Adafruit_ILI9341(cs, dc),
      _ts((uint8_t)tcs, &SPI),
      _tirq(tirq)
  {}

  bool begin() {
    Adafruit_ILI9341::begin();
    setRotation(3);       // landscape 320x240, correct orientation
    fillScreen(0x0000);

    pinMode(_tirq, INPUT);  // active-LOW open-drain; board has pull-up

    _touchOk = _ts.begin();
    if (_touchOk) {
      // Drain until TIRQ goes HIGH (hardware signal, never lags unlike FIFO register)
      uint32_t t0 = millis();
      while (millis() - t0 < 2000 && digitalRead(_tirq) == LOW) {
        _ts.getPoint();   // consume one FIFO entry
        delay(10);
      }
    }
    return _touchOk;
  }

  void setTouch(int16_t x1, int16_t y1, int16_t x2, int16_t y2) {
    _rx1=x1; _ry1=y1; _rx2=x2; _ry2=y2;
  }

  // TIRQ is active-LOW primary; _ts.touched() (FIFO register) is fallback
  // in case TIRQ wiring is unreliable on this board.
  bool touched() {
    if (!_touchOk) return false;
    if (digitalRead(_tirq) == LOW) return true;
    return _ts.touched();   // SPI register fallback
  }

  // Drain the ENTIRE FIFO, map the last valid point to xTouch/yTouch.
  // KEY FIX: use digitalRead(TIRQ) to detect empty FIFO — the hardware signal
  // is instant, whereas _ts.touched() reads a SPI register that lags after getPoint().
  // Using the stale SPI register caused TIRQ to stay LOW permanently → frozen UI.
  void readTouch() {
    if (!_touchOk) return;
    xTouch = 0; yTouch = 0;  // reset so stale coords are never used if read fails
    TS_Point last = {0, 0, 0};
    bool     got  = false;
    uint8_t  n    = 0;
    while (n < 16 && (digitalRead(_tirq) == LOW || _ts.touched())) {
      last = _ts.getPoint();
      got  = true;
      n++;
      delay(2);   // 2ms: gives STMPE610 time to dequeue before checking TIRQ again
    }
    if (!got || (last.x == 0 && last.y == 0)) return;

    xRaw = (int16_t)last.x;
    yRaw = (int16_t)last.y;

    int16_t sx = (int16_t)map((long)last.x, (long)_rx1, (long)_rx2, 0L, (long)(width()-1));
    int16_t sy = (int16_t)map((long)last.y, (long)_ry1, (long)_ry2, 0L, (long)(height()-1));
    if (sx < 0) sx = 0; if (sx >= (int16_t)width())  sx = width()-1;
    if (sy < 0) sy = 0; if (sy >= (int16_t)height()) sy = height()-1;
    xTouch = sx;
    yTouch = sy;
  }

  // Drain FIFO completely — use TIRQ (hardware, instant) not SPI register (lags)
  void drainFifo() {
    if (!_touchOk) return;
    uint8_t n = 0;
    while (digitalRead(_tirq) == LOW && n < 32) { _ts.getPoint(); n++; delay(2); }
  }

  bool touchOk() { return _touchOk; }

  bool readRawPoint(int16_t &rx, int16_t &ry) {
    if (!_touchOk || !_ts.touched()) return false;
    TS_Point p = _ts.getPoint();
    rx=(int16_t)p.x; ry=(int16_t)p.y;
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
  Adafruit_STMPE610 _ts;
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