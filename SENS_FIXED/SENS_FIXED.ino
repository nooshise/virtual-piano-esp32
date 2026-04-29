// ========== SENSOR BOARD: TCA9548A + 8x MIXED (VL53L0X/VL6180X) + RF + NeoPixel ==========
// Sends:    T,<1..8>,<mm>          — finger trigger to UI board
// Receives: C,<1..8>,<colorIdx>    — LED color config from UI board (0..7 or 255=off)
//
// CALIBRATION: sorted-median baseline per channel at boot.
//   enterThr = baseline * (1 - ENTER_RATIO)
//   exitThr  = enterThr + delta * HYSTERESIS_RAT
//   Outlier rejection: sort 20 samples, average the middle half.
//
// NeoPixel: GPIO 25, 8 LEDs. Lights on trigger, auto-off 300ms.
//   Colors set by UI board via RF "C,<ch>,<colorIdx>" commands.
//   Default: rainbow per channel.
// ==============================================================================

#include <Wire.h>
#include <Adafruit_VL53L0X.h>
#include <Adafruit_VL6180X.h>
#include <Adafruit_NeoPixel.h>

typedef enum { ST_NONE=0, ST_VL53=1, ST_VL618=2 } SensorType;

static const int      I2C_SDA  = 21;
static const int      I2C_SCL  = 22;
static const uint32_t I2C_HZ   = 100000;
static const uint8_t  TCA_ADDR = 0x70;

#define RF_BAUD   9600
#define RF_RX_PIN 34
#define RF_TX_PIN 13
HardwareSerial RF(1);
String rfLine;

// ---- NeoPixel ----
#define NEO_PIN    25
#define NEO_COUNT  8
#define NEO_BRIGHT 80
Adafruit_NeoPixel strip(NEO_COUNT, NEO_PIN, NEO_GRB + NEO_KHZ800);

// 8 fixed colors (RGB565 palette, same as UI board)
static const uint16_t COLORS565[8] = {
  0xF800, 0x07E0, 0x001F, 0xFFE0, 0x07FF, 0xF81F, 0xFD20, 0xFFFF
};
static uint32_t cidxToNeo(uint8_t ci) {
  if (ci >= 8) return 0;
  uint16_t c = COLORS565[ci];
  uint8_t r5=(c>>11)&0x1F, g6=(c>>5)&0x3F, b5=c&0x1F;
  return strip.Color((r5*527+23)>>6, (g6*259+33)>>6, (b5*527+23)>>6);
}

uint8_t  ledCi[NEO_COUNT];          // color index per channel (255 = off)
uint32_t ledOffAt[NEO_COUNT]={0};

// Default rainbow colors
static const uint8_t DEF_CI[8] = {0,1,2,3,4,5,6,7};  // Red,Grn,Blue,Yel,Cyan,Mag,Org,Wht

static void ledTick() {
  uint32_t now=millis(); bool any=false;
  for(uint8_t i=0;i<NEO_COUNT;i++) {
    if(ledOffAt[i] && now>=ledOffAt[i]) { strip.setPixelColor(i,0); ledOffAt[i]=0; any=true; }
  }
  if(any) strip.show();
}

// ----- Tuning -----
static const uint8_t  CAL_SAMPLES    = 8;
static const uint16_t CAL_DELAY_MS   = 0;     // VL53 read itself takes ~20ms (timing budget)
static const float    ENTER_RATIO    = 0.20f; // trigger when 20% closer than baseline
static const float    HYSTERESIS_RAT = 0.50f; // exit = enterThr + 50% of delta
static const uint16_t DELTA_MIN      = 12;
static const uint16_t DELTA_MAX      = 80;
static const uint16_t ENTER_FIXED    = 120;   // fallback if calibration fails (12cm)
static const uint16_t EXIT_FIXED     = 160;

static const uint8_t  NEAR_STABLE    = 2;     // 2 readings -> trigger (filters early noise)
static const uint8_t  FAR_STABLE     = 2;     // 2 readings → rearm (~320ms with 20ms budget)
static const uint16_t READ_PERIOD_MS = 30;
static const uint16_t DIST_LOG_MS    = 750;   // periodic per-channel distance diagnostics
static const uint16_t MIN_TRIG_GAP   = 150;
static const uint8_t  FAIL_RECOVER   = 10;
static const uint8_t  NCH            = 8;

static const uint16_t MIN_VL53  = 50;   // normalize: worst-case VL53L0X min range
static const uint16_t MIN_VL618 = 10;
static const uint16_t TRIG_MAX_MM = 150; // never trigger beyond 15cm (avoids far false-triggers)

struct ChanState {
  SensorType type      = ST_NONE;
  bool       ok        = false;
  bool       nearState = false;
  uint8_t    nearCnt   = 0;
  uint8_t    farCnt    = 0;
  bool       armed     = true;
  uint8_t    failStreak= 0;
  uint32_t   lastReadMs= 0;
  uint32_t   lastLogMs = 0;
  uint32_t   lastTrigMs= 0;
  uint16_t   baseline  = 200;
  uint16_t   enterThr  = ENTER_FIXED;
  uint16_t   exitThr   = EXIT_FIXED;
  uint16_t   minValid  = MIN_VL53;
};

ChanState        chs[NCH];
Adafruit_VL53L0X vl53[NCH];
Adafruit_VL6180X vl618[NCH];

static const char* sensorTypeName(SensorType t) {
  return t==ST_VL618 ? "VL6180X" : t==ST_VL53 ? "VL53L0X" : "NONE";
}

// ---- I2C helpers ----
static bool tcaSelect(uint8_t ch) {
  if (ch>7) return false;
  Wire.beginTransmission(TCA_ADDR);
  Wire.write(1<<ch);
  bool ok=(Wire.endTransmission()==0);
  delay(1);
  return ok;
}
static bool i2cPing(uint8_t addr) {
  Wire.beginTransmission(addr);
  return (Wire.endTransmission()==0);
}
static bool i2cReg16(uint8_t addr, uint16_t reg, uint8_t &val) {
  Wire.beginTransmission(addr);
  Wire.write((uint8_t)(reg>>8)); Wire.write((uint8_t)(reg&0xFF));
  if (Wire.endTransmission(false)!=0) return false;
  if (Wire.requestFrom((int)addr,1)!=1) return false;
  val=Wire.read(); return true;
}
static void i2cRecover() {
  Serial.println("I2C bus recovery...");
  pinMode(I2C_SCL,OUTPUT); pinMode(I2C_SDA,INPUT_PULLUP);
  for (int i=0;i<9;i++) {
    digitalWrite(I2C_SCL,LOW);  delayMicroseconds(10);
    digitalWrite(I2C_SCL,HIGH); delayMicroseconds(10);
    if (digitalRead(I2C_SDA)) break;
  }
  pinMode(I2C_SDA,OUTPUT);
  digitalWrite(I2C_SDA,LOW);  delayMicroseconds(10);
  digitalWrite(I2C_SCL,HIGH); delayMicroseconds(10);
  digitalWrite(I2C_SDA,HIGH); delayMicroseconds(10);
  Wire.begin(I2C_SDA,I2C_SCL);
  Wire.setClock(I2C_HZ);
  delay(20);
}

static bool initChannel(uint8_t ch) {
  chs[ch].type=ST_NONE; chs[ch].ok=false;
  chs[ch].nearState=false; chs[ch].nearCnt=0; chs[ch].farCnt=0;
  chs[ch].armed=true; chs[ch].failStreak=0;
  chs[ch].lastReadMs=0; chs[ch].lastLogMs=0; chs[ch].lastTrigMs=0;
  chs[ch].enterThr=ENTER_FIXED; chs[ch].exitThr=EXIT_FIXED;

  if (!tcaSelect(ch)) {
    Serial.print("INIT CH"); Serial.print(ch); Serial.println(": TCA select failed");
    return false;
  }
  if (!i2cPing(0x29)) {
    Serial.print("INIT CH"); Serial.print(ch); Serial.println(": no sensor response @0x29");
    return false;
  }

  // Detect sensor type inline (avoids Arduino auto-prototype conflict with custom return type)
  SensorType t = ST_VL53;
  { uint8_t model=0; if (i2cReg16(0x29,0x0000,model) && model==0xB4) t=ST_VL618; }
  chs[ch].type=t;
  Serial.print("INIT CH"); Serial.print(ch);
  Serial.print(": detected "); Serial.println(sensorTypeName(t));
  if (t==ST_NONE) return false;

  bool ok=false;
  if (t==ST_VL618) {
    tcaSelect(ch);
    ok=vl618[ch].begin();
    chs[ch].minValid=MIN_VL618;
  } else {
    tcaSelect(ch);
    ok=vl53[ch].begin(0x29,false,&Wire);
    if (ok) {
      tcaSelect(ch); vl53[ch].stopRangeContinuous(); delay(5);
      tcaSelect(ch); vl53[ch].setMeasurementTimingBudgetMicroSeconds(20000); // 20ms → faster reads
    }
    chs[ch].minValid=MIN_VL53;
  }
  chs[ch].ok=ok;
  Serial.print("INIT CH"); Serial.print(ch);
  Serial.print(": "); Serial.println(ok ? "OK" : "begin failed");
  return ok;
}

// Runtime read — strict.
// VL53 status 4/5/7 = "no target in range" — valid FAR, not an error → returns mm=9999.
// VL53 status 3 = "min range fail" (target <30mm) — use with caution at runtime.
static bool readMm(uint8_t ch, uint16_t &mm) {
  if (!chs[ch].ok) return false;
  if (!tcaSelect(ch)) return false;
  if (chs[ch].type==ST_VL53) {
    VL53L0X_RangingMeasurementData_t m;
    vl53[ch].rangingTest(&m,false);
    // No-target statuses → valid far reading, not a sensor fault
    if (m.RangeStatus==4||m.RangeStatus==5||m.RangeStatus==7) { mm=9999; return true; }
    // Min-range fail at runtime: still report the value (finger very close)
    if (m.RangeStatus==3) {
      if (m.RangeMilliMeter==0||m.RangeMilliMeter>=2000) return false;
      mm=(uint16_t)m.RangeMilliMeter; return true;
    }
    if (m.RangeStatus!=0) return false;
    if (m.RangeMilliMeter==0||m.RangeMilliMeter>=2000) return false;
    mm=(uint16_t)m.RangeMilliMeter; return true;
  }
  if (chs[ch].type==ST_VL618) {
    uint8_t r=vl618[ch].readRange();
    uint8_t st=vl618[ch].readRangeStatus();
    if (st==0 && r>0 && r<250) { mm=(uint16_t)r; return true; }
    if (r>=200) { mm=9999; return true; }
    return false;
  }
  return false;
}

// Calibration read — also accepts VL53 status 3 (min-range-fail, target <30mm)
static bool readMmPermissive(uint8_t ch, uint16_t &mm) {
  if (!chs[ch].ok) return false;
  if (!tcaSelect(ch)) return false;
  if (chs[ch].type==ST_VL53) {
    VL53L0X_RangingMeasurementData_t m;
    vl53[ch].rangingTest(&m,false);
    if (m.RangeStatus!=0 && m.RangeStatus!=3) return false;
    if (m.RangeMilliMeter==0||m.RangeMilliMeter>=2000) return false;
    mm=(uint16_t)m.RangeMilliMeter; return true;
  }
  if (chs[ch].type==ST_VL618) {
    uint8_t r=vl618[ch].readRange();
    if (r==0||r>=250) return false;
    mm=(uint16_t)r; return true;
  }
  return false;
}

// Sort ascending (bubble sort — small arrays only)
static void sortU16(uint16_t* arr, uint8_t n) {
  for(uint8_t i=0;i<n-1;i++)
    for(uint8_t j=0;j<n-1-i;j++)
      if(arr[j]>arr[j+1]) { uint16_t t=arr[j]; arr[j]=arr[j+1]; arr[j+1]=t; }
}

// Calibrate using sorted-median — rejects outliers caused by noise / interference
static void calibrateChannel(uint8_t ch) {
  if (!chs[ch].ok) return;
  uint16_t samples[CAL_SAMPLES]; uint8_t cnt=0;
  uint8_t rejected=0;
  Serial.print("CAL CH"); Serial.print(ch);
  Serial.print(" ["); Serial.print(sensorTypeName(chs[ch].type));
  Serial.println("]: collecting samples");
  for(uint8_t i=0;i<CAL_SAMPLES;i++) {
    delay(CAL_DELAY_MS);
    uint16_t mm;
    if(readMmPermissive(ch,mm) && mm>=chs[ch].minValid && mm<2000)
      samples[cnt++]=mm;
    else
      rejected++;
  }
  Serial.print("CAL CH"); Serial.print(ch);
  Serial.print(": valid="); Serial.print(cnt);
  Serial.print(" rejected="); Serial.println(rejected);
  if (cnt >= 4) {
    sortU16(samples, cnt);
    // Use middle half — discard bottom 25% and top 25% outliers
    uint8_t lo=cnt/4, hi=cnt-cnt/4;
    uint32_t sum=0;
    for(uint8_t i=lo;i<hi;i++) sum+=samples[i];
    uint16_t base=(uint16_t)(sum/(hi-lo));
    chs[ch].baseline=base;

    uint16_t delta=(uint16_t)(base*ENTER_RATIO);
    if (delta<DELTA_MIN) delta=DELTA_MIN;
    if (delta>DELTA_MAX) delta=DELTA_MAX;
    chs[ch].enterThr = base - delta;
    // Cap: never trigger from beyond TRIG_MAX_MM (avoids false triggers when hand hovers far)
    if (chs[ch].enterThr > TRIG_MAX_MM) chs[ch].enterThr = TRIG_MAX_MM;
    uint16_t hyst = (uint16_t)(delta*HYSTERESIS_RAT);
    if (hyst<5) hyst=5;
    chs[ch].exitThr = chs[ch].enterThr + hyst;
    if (chs[ch].exitThr >= base) chs[ch].exitThr = base - 2;

    Serial.print("CAL CH"); Serial.print(ch);
    Serial.print(" ["); Serial.print(sensorTypeName(chs[ch].type)); Serial.print("]");
    Serial.print(": base="); Serial.print(base);
    Serial.print("mm enter<"); Serial.print(chs[ch].enterThr);
    Serial.print("mm exit<"); Serial.println(chs[ch].exitThr);
  } else {
    chs[ch].ok=false;
    chs[ch].nearState=false; chs[ch].armed=false;
    chs[ch].enterThr=ENTER_FIXED; chs[ch].exitThr=EXIT_FIXED;
    Serial.print("CAL CH"); Serial.print(ch);
    Serial.println(": low valid sample count -> INACTIVE (check wiring/clear path)");
  }
}

static void recoverChannel(uint8_t ch) {
  Serial.print("RECOVER CH"); Serial.println(ch);
  for (int a=0;a<3;a++) {
    delay(50);
    if (initChannel(ch)) {
      // Keep existing calibration thresholds — recal during runtime is not safe
      Serial.print("RECOVER CH"); Serial.print(ch); Serial.println(" OK");
      return;
    }
  }
  Serial.print("RECOVER CH"); Serial.print(ch); Serial.println(" FAIL");
}

static void rfSendTrig(uint8_t s1to8, uint16_t mm) {
  RF.print("T,"); RF.print(s1to8); RF.print(","); RF.print(mm); RF.print("\n");
  Serial.print(">>RF T,"); Serial.print(s1to8); Serial.print(","); Serial.println(mm);
  // Light corresponding LED immediately
  uint8_t i = s1to8 - 1;
  strip.setPixelColor(i, cidxToNeo(ledCi[i]));
  strip.show();
  ledOffAt[i] = millis() + 300;
}

static void processRFLine(String line) {
  line.trim(); if (!line.length()) return;
  // "C,<1..8>,<colorIdx>" — UI board sets LED color for a channel
  if (line.startsWith("C,")) {
    int p1 = line.indexOf(',', 2); if (p1<0) return;
    uint8_t ch = (uint8_t)line.substring(2, p1).toInt();
    uint8_t ci = (uint8_t)line.substring(p1+1).toInt();
    if (ch>=1 && ch<=8) {
      ledCi[ch-1] = ci;
      Serial.print("LED CH"); Serial.print(ch-1);
      Serial.print(" ci="); Serial.println(ci);
    }
    return;
  }
  if (line.startsWith("HELLO,UI")) RF.print("HELLO_ACK,SENS\n");
}

static void processRF() {
  while (RF.available()) {
    char c=(char)RF.read();
    if (c=='\r') continue;
    if (c=='\n') { processRFLine(rfLine); rfLine=""; }
    else { if (rfLine.length()<120) rfLine+=c; }
  }
}

static void tickSensors() {
  uint32_t now=millis();
  for (uint8_t ch=0;ch<NCH;ch++) {
    if (!chs[ch].ok) continue;
    if (now-chs[ch].lastReadMs<READ_PERIOD_MS) continue;
    chs[ch].lastReadMs=now;

    uint16_t mm=0;
    if (!readMm(ch,mm)) {
      if (++chs[ch].failStreak>=FAIL_RECOVER) { chs[ch].failStreak=0; recoverChannel(ch); }
      continue;
    }
    chs[ch].failStreak=0;
    // (no skip for mm<MIN_VL53 — close finger readings are valid, state machine needs them)
    if (now-chs[ch].lastLogMs>=DIST_LOG_MS) {
      chs[ch].lastLogMs=now;
      Serial.print("DIST CH"); Serial.print(ch);
      Serial.print(" ["); Serial.print(sensorTypeName(chs[ch].type)); Serial.print("]");
      Serial.print(" mm="); Serial.print(mm);
      Serial.print(" enter<"); Serial.print(chs[ch].enterThr);
      Serial.print(" exit<"); Serial.print(chs[ch].exitThr);
      Serial.print(" state="); Serial.print(chs[ch].nearState ? "NEAR" : "FAR");
      Serial.print(" armed="); Serial.println(chs[ch].armed ? "Y" : "N");
    }

    bool near = chs[ch].nearState ? (mm<=chs[ch].exitThr) : (mm<=chs[ch].enterThr);

    if (near)  { if(chs[ch].nearCnt<255) chs[ch].nearCnt++; chs[ch].farCnt=0; }
    else       { if(chs[ch].farCnt<255)  chs[ch].farCnt++;  chs[ch].nearCnt=0; }

    if (!chs[ch].nearState && near && chs[ch].nearCnt>=NEAR_STABLE) {
      chs[ch].nearState=true;
      if (chs[ch].armed && now-chs[ch].lastTrigMs>=MIN_TRIG_GAP) {
        Serial.print("TRIGGER CH"); Serial.print(ch);
        Serial.print(" mm="); Serial.print(mm);
        Serial.print(" nearCnt="); Serial.println(chs[ch].nearCnt);
        chs[ch].lastTrigMs=now; chs[ch].armed=false;
        rfSendTrig(ch+1, mm);
      }
    }
    if (chs[ch].nearState && !near && chs[ch].farCnt>=FAR_STABLE) {
      chs[ch].nearState=false; chs[ch].armed=true;
      Serial.print("REARM CH"); Serial.print(ch);
      Serial.print(" mm="); Serial.print(mm);
      Serial.print(" farCnt="); Serial.println(chs[ch].farCnt);
    }
  }
}

void setup() {
  Serial.begin(115200); delay(300);
  Serial.println("\n=== SENS: TCA + MIXED-TOF + RF + CAL + NEOPIXEL ===");

  // Init NeoPixel with default rainbow colors
  for(uint8_t i=0;i<NEO_COUNT;i++) ledCi[i]=DEF_CI[i];
  strip.begin();
  strip.setBrightness(NEO_BRIGHT);
  strip.clear(); strip.show();

  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(I2C_HZ);
  delay(300);

  // Retry TCA boot ping — chip needs ~200ms after power-on
  bool tcaOk=false;
  for (int i=0;i<10&&!tcaOk;i++) {
    Wire.beginTransmission(TCA_ADDR);
    tcaOk=(Wire.endTransmission()==0);
    if (!tcaOk) delay(50);
  }
  Serial.println(tcaOk ? "TCA OK @0x70" : "TCA MISSING @0x70 — check wiring!");

  RF.begin(RF_BAUD, SERIAL_8N1, RF_RX_PIN, RF_TX_PIN);
  RF.print("BOOT,SENS\n");
  Serial.println("RF @9600 RX=34 TX=13");

  Serial.println("--- Init CH0..CH7 ---");
  for (uint8_t ch=0;ch<NCH;ch++) {
    bool ok=false;
    for (int a=0;a<3&&!ok;a++) {
      if (a>0) delay(100);
      if (a==1) i2cRecover();
      ok=initChannel(ch);
    }
    Serial.print("CH"); Serial.print(ch); Serial.print(": ");
    Serial.println(!ok ? "NONE/FAIL" : (chs[ch].type==ST_VL618 ? "VL6180X OK" : "VL53L0X OK"));
  }

  // Brief startup LED animation
  for(uint8_t pass=0;pass<2;pass++) {
    for(uint8_t i=0;i<NCH;i++)
      if(chs[i].ok) strip.setPixelColor(i, cidxToNeo(ledCi[i]));
    strip.show(); delay(200);
    strip.clear(); strip.show(); delay(100);
  }

  // Calibrate — RAPID YELLOW FLASH warns user to remove hands for 2 seconds
  Serial.println("--- Calibrating in 2s — KEEP SENSORS CLEAR! ---");
  for (uint8_t flash=0;flash<7;flash++) {
    for(uint8_t i=0;i<NCH;i++) if(chs[i].ok) strip.setPixelColor(i,strip.Color(180,180,0));
    strip.show(); delay(150);
    strip.clear(); strip.show(); delay(130);
  }
  Serial.println("--- Calibrating now ---");
  for (uint8_t ch=0;ch<NCH;ch++) {
    if (!chs[ch].ok) continue;
    // Dim flash on LED being calibrated
    strip.setPixelColor(ch, strip.Color(30,30,30)); strip.show();
    calibrateChannel(ch);
    strip.setPixelColor(ch, 0); strip.show();
  }
  Serial.println("--- Ready ---");
}

void loop() {
  processRF();
  tickSensors();
  ledTick();
  delay(0);
}
