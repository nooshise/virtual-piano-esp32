#include "ToFSensors.h"

#include <Adafruit_VL53L0X.h>
#include <Adafruit_VL6180X.h>
#include "Diagnostics.h"
#include "LedStrip.h"
#include "TcaMux.h"

ChanState chs[NCH];
static Adafruit_VL53L0X vl53[NCH];
static Adafruit_VL6180X vl618[NCH];

static bool initChannel(uint8_t ch) {
  chs[ch].type = ST_NONE;
  chs[ch].ok = false;
  chs[ch].nearState = false;
  chs[ch].nearCnt = 0;
  chs[ch].farCnt = 0;
  chs[ch].armed = true;
  chs[ch].failStreak = 0;
  chs[ch].lastReadMs = 0;
  chs[ch].lastLogMs = 0;
  chs[ch].lastTrigMs = 0;
  chs[ch].enterThr = ENTER_FIXED;
  chs[ch].exitThr = EXIT_FIXED;

  if (!tcaSelect(ch)) {
    Serial.print("INIT CH");
    Serial.print(ch);
    Serial.println(": TCA select failed");
    return false;
  }
  if (!i2cPing(0x29)) {
    Serial.print("INIT CH");
    Serial.print(ch);
    Serial.println(": no sensor response @0x29");
    return false;
  }

  SensorType t = ST_VL53;
  uint8_t model = 0;
  if (i2cReg16(0x29, 0x0000, model) && model == 0xB4) t = ST_VL618;
  chs[ch].type = t;
  Serial.print("INIT CH");
  Serial.print(ch);
  Serial.print(": detected ");
  Serial.println(sensorTypeName(t));

  bool ok = false;
  if (t == ST_VL618) {
    tcaSelect(ch);
    ok = vl618[ch].begin();
    chs[ch].minValid = MIN_VL618;
  } else {
    tcaSelect(ch);
    ok = vl53[ch].begin(0x29, false, &Wire);
    if (ok) {
      tcaSelect(ch);
      vl53[ch].stopRangeContinuous();
      delay(5);
      tcaSelect(ch);
      vl53[ch].setMeasurementTimingBudgetMicroSeconds(20000);
    }
    chs[ch].minValid = MIN_VL53;
  }
  chs[ch].ok = ok;
  Serial.print("INIT CH");
  Serial.print(ch);
  Serial.print(": ");
  Serial.println(ok ? "OK" : "begin failed");
  return ok;
}

bool readMm(uint8_t ch, uint16_t &mm) {
  if (!chs[ch].ok) return false;
  if (!tcaSelect(ch)) return false;
  if (chs[ch].type == ST_VL53) {
    VL53L0X_RangingMeasurementData_t m;
    vl53[ch].rangingTest(&m, false);
    if (m.RangeStatus == 4 || m.RangeStatus == 5 || m.RangeStatus == 7) {
      mm = 9999;
      return true;
    }
    if (m.RangeStatus == 3) {
      if (m.RangeMilliMeter == 0 || m.RangeMilliMeter >= 2000) return false;
      mm = (uint16_t)m.RangeMilliMeter;
      return true;
    }
    if (m.RangeStatus != 0) return false;
    if (m.RangeMilliMeter == 0 || m.RangeMilliMeter >= 2000) return false;
    mm = (uint16_t)m.RangeMilliMeter;
    return true;
  }
  if (chs[ch].type == ST_VL618) {
    uint8_t r = vl618[ch].readRange();
    uint8_t st = vl618[ch].readRangeStatus();
    if (st == 0 && r > 0 && r < 250) {
      mm = (uint16_t)r;
      return true;
    }
    if (r >= 200) {
      mm = 9999;
      return true;
    }
    return false;
  }
  return false;
}

static bool readMmPermissive(uint8_t ch, uint16_t &mm) {
  if (!chs[ch].ok) return false;
  if (!tcaSelect(ch)) return false;
  if (chs[ch].type == ST_VL53) {
    VL53L0X_RangingMeasurementData_t m;
    vl53[ch].rangingTest(&m, false);
    if (m.RangeStatus != 0 && m.RangeStatus != 3) return false;
    if (m.RangeMilliMeter == 0 || m.RangeMilliMeter >= 2000) return false;
    mm = (uint16_t)m.RangeMilliMeter;
    return true;
  }
  if (chs[ch].type == ST_VL618) {
    uint8_t r = vl618[ch].readRange();
    if (r == 0 || r >= 250) return false;
    mm = (uint16_t)r;
    return true;
  }
  return false;
}

static void sortU16(uint16_t* arr, uint8_t n) {
  for (uint8_t i = 0; i < n - 1; i++) {
    for (uint8_t j = 0; j < n - 1 - i; j++) {
      if (arr[j] > arr[j + 1]) {
        uint16_t t = arr[j];
        arr[j] = arr[j + 1];
        arr[j + 1] = t;
      }
    }
  }
}

static void calibrateChannel(uint8_t ch) {
  if (!chs[ch].ok) return;
  uint16_t samples[CAL_SAMPLES];
  uint8_t cnt = 0;
  uint8_t rejected = 0;
  Serial.print("CAL CH");
  Serial.print(ch);
  Serial.print(" [");
  Serial.print(sensorTypeName(chs[ch].type));
  Serial.println("]: collecting samples");
  for (uint8_t i = 0; i < CAL_SAMPLES; i++) {
    delay(CAL_DELAY_MS);
    uint16_t mm;
    if (readMmPermissive(ch, mm) && mm >= chs[ch].minValid && mm < 2000) {
      samples[cnt++] = mm;
    } else {
      rejected++;
    }
  }
  Serial.print("CAL CH");
  Serial.print(ch);
  Serial.print(": valid=");
  Serial.print(cnt);
  Serial.print(" rejected=");
  Serial.println(rejected);
  if (cnt >= 4) {
    sortU16(samples, cnt);
    uint8_t lo = cnt / 4;
    uint8_t hi = cnt - cnt / 4;
    uint32_t sum = 0;
    for (uint8_t i = lo; i < hi; i++) sum += samples[i];
    uint16_t base = (uint16_t)(sum / (hi - lo));
    chs[ch].baseline = base;

    uint16_t delta = (uint16_t)(base * ENTER_RATIO);
    if (delta < DELTA_MIN) delta = DELTA_MIN;
    if (delta > DELTA_MAX) delta = DELTA_MAX;
    chs[ch].enterThr = base - delta;
    if (chs[ch].enterThr > TRIG_MAX_MM) chs[ch].enterThr = TRIG_MAX_MM;
    uint16_t hyst = (uint16_t)(delta * HYSTERESIS_RAT);
    if (hyst < 5) hyst = 5;
    chs[ch].exitThr = chs[ch].enterThr + hyst;
    if (chs[ch].exitThr >= base) chs[ch].exitThr = base - 2;

    Serial.print("CAL CH");
    Serial.print(ch);
    Serial.print(" [");
    Serial.print(sensorTypeName(chs[ch].type));
    Serial.print("]");
    Serial.print(": base=");
    Serial.print(base);
    Serial.print("mm enter<");
    Serial.print(chs[ch].enterThr);
    Serial.print("mm exit<");
    Serial.println(chs[ch].exitThr);
  } else {
    chs[ch].ok = false;
    chs[ch].nearState = false;
    chs[ch].armed = false;
    chs[ch].enterThr = ENTER_FIXED;
    chs[ch].exitThr = EXIT_FIXED;
    Serial.print("CAL CH");
    Serial.print(ch);
    Serial.println(": low valid sample count -> INACTIVE (check wiring/clear path)");
  }
}

void recoverChannel(uint8_t ch) {
  Serial.print("RECOVER CH");
  Serial.println(ch);
  for (int a = 0; a < 3; a++) {
    delay(50);
    if (initChannel(ch)) {
      Serial.print("RECOVER CH");
      Serial.print(ch);
      Serial.println(" OK");
      return;
    }
  }
  Serial.print("RECOVER CH");
  Serial.print(ch);
  Serial.println(" FAIL");
}

void initAllSensors() {
  Serial.println("--- Init CH0..CH7 ---");
  for (uint8_t ch = 0; ch < NCH; ch++) {
    bool ok = false;
    for (int a = 0; a < 3 && !ok; a++) {
      if (a > 0) delay(100);
      if (a == 1) i2cRecover();
      ok = initChannel(ch);
    }
    Serial.print("CH");
    Serial.print(ch);
    Serial.print(": ");
    Serial.println(!ok ? "NONE/FAIL" : (chs[ch].type == ST_VL618 ? "VL6180X OK" : "VL53L0X OK"));
  }
}

void calibrateAllSensors() {
  Serial.println("--- Calibrating now ---");
  for (uint8_t ch = 0; ch < NCH; ch++) {
    if (!chs[ch].ok) continue;
    ledFlashCalibrating(ch);
    calibrateChannel(ch);
    ledClearOne(ch);
  }
}
