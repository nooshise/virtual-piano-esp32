#include <Arduino.h>
#include <SoftwareSerial.h>
#include "TFT9341Touch.h"
#ifdef ESP32
  #include <pgmspace.h>
#else
  #include <avr/pgmspace.h>
#endif

#include "icon_piano.h"
#include "icon_guitar.h"
#include "icon_drums.h"
#include "icon_trumpet.h"
#include "icon_violin.h"
#include "icon_accordion.h"
#include "icon_oud.h"
#include "icon_fan_fluit.h"

// ---- TFT (cs=5, dc=4, touch-cs=15, tirq=35) ----
tft9341touch lcd(5, 4, 15, 35);

#define BLACK    0x0000
#define WHITE    0xFFFF
#define RED      0xF800
#define GREEN    0x07E0
#define BLUE     0x001F
#define YELLOW   0xFFE0
#define CYAN     0x07FF
#define MAGENTA  0xF81F
#define DARKGREY 0x7BEF

// ---- RF ----
#define RF_BAUD   9600
#define RF_RX_PIN 34
#define RF_TX_PIN 13
HardwareSerial RF(1);
String rfLine;

// ---- DFPlayer Mini — SoftwareSerial TX-only ----
// SD card layout: 001.mp3 … 064.mp3 in ROOT folder.
//   Track = inst * 8 + note + 1  (instrument 0..7, note 0..7 → track 1..64)
//   inst 0 (Piano):     001–008
//   inst 1 (Guitar):    009–016
//   inst 2 (Drums):     017–024
//   inst 3 (Trumpet):   025–032
//   inst 4 (Violin):    033–040
//   inst 5 (Accordion): 041–048
//   inst 6 (Oud):       049–056
//   inst 7 (Fan Fluit): 057–064
static const uint8_t  N_PLAYERS   = 8;
static const int      DF_TX_PINS[N_PLAYERS] = {16,17,21,22,26,27,32,33};
static const int      DF_RX_DUMMY = 39;   // GPIO39 input-only, safe dummy RX
static const uint8_t  DF_VOLUME   = 28;
static const uint16_t DF_GAP_MS   = 70;
static const uint32_t DF_BOOT_MS  = 6000;

SoftwareSerial* dfSS[N_PLAYERS] = {nullptr};

static void dfSend(uint8_t p, uint8_t cmd, uint16_t param) {
  if (p>=N_PLAYERS || !dfSS[p]) return;
  uint8_t dh=(param>>8)&0xFF, dl=param&0xFF;
  const uint8_t ver=0xFF, len=0x06, fb=0x00;
  uint16_t sum=(uint16_t)ver+len+cmd+fb+dh+dl;
  uint16_t chk=(uint16_t)(0xFFFF-sum+1);
  uint8_t fr[10]={0x7E,ver,len,cmd,fb,dh,dl,(uint8_t)(chk>>8),(uint8_t)chk,0xEF};
  dfSS[p]->write(fr, 10);
}
static void dfSetVol(uint8_t p, uint8_t v) { dfSend(p,0x06,v); }
static void dfStop(uint8_t p)              { dfSend(p,0x16,0); }
// cmd 0x03 = play by FAT index from root (001.mp3, 002.mp3 … in SD root)
static void dfPlay(uint8_t p, uint16_t t)  { if(t<1)t=1; if(t>64)t=64; dfSend(p,0x03,t); }

bool     dfReady  = false;
uint8_t  dfStage  = 0;
uint32_t dfNextMs = 0;

static void dfInitStart() {
  dfReady=false; dfStage=0;
  dfNextMs=millis()+DF_BOOT_MS;
  for (uint8_t i=0;i<N_PLAYERS;i++) {
    if (!dfSS[i]) {
      dfSS[i] = new SoftwareSerial(DF_RX_DUMMY, DF_TX_PINS[i]);
      dfSS[i]->begin(9600);
    }
  }
  Serial.print("DF boot wait "); Serial.print(DF_BOOT_MS); Serial.println("ms...");
}
static void dfInitTick() {
  if (dfReady) return;
  uint32_t now=millis(); if (now<dfNextMs) return;
  if (dfStage==0) {
    Serial.println("=== DF INIT (8 SoftwareSerial TX) ===");
    dfStage=1; dfNextMs=now+20; return;
  }
  uint8_t p=dfStage-1;
  if (p<N_PLAYERS) {
    Serial.print("DF"); Serial.print(p+1);
    Serial.print(" GPIO"); Serial.print(DF_TX_PINS[p]); Serial.println("...");
    dfSetVol(p,DF_VOLUME); delay(DF_GAP_MS);
    dfStop(p);             delay(DF_GAP_MS);
    dfStage++; dfNextMs=now+120; return;
  }
  dfReady=true;
  Serial.println("DF ALL READY");
}

// ---- Instruments ----
static const uint8_t INST_COUNT = 8;
static const char* INST_NAMES[INST_COUNT] = {
  "Piano","Guitar","Drums","Trumpet","Violin","Accordn","Oud","Fan"
};
static const uint16_t* INST_ICONS[INST_COUNT] = {
  icon_piano_bitmap, icon_guitar_bitmap, icon_drums_bitmap, icon_trumpet_bitmap,
  icon_violin_bitmap, icon_accordion_bitmap, icon_oud_bitmap, icon_fan_fluit_bitmap
};
static const uint8_t ICON_W=64, ICON_H=64;

// ---- Notes ----
static const char* NOTE_NAMES[8] = {"Do","Re","Mi","Fa","Sol","La","Si","Do2"};

// ---- Colors (RGB565 + names) ----
static const uint8_t  NUM_COLORS = 8;
static const uint8_t  NO_COLOR   = 255;
static const uint16_t COLORS565[NUM_COLORS] = {
  0xF800,0x07E0,0x001F,0xFFE0,0x07FF,0xF81F,0xFD20,0xFFFF
};
static const char* COLOR_NAMES[NUM_COLORS] = {
  "Red","Grn","Blue","Yel","Cyan","Mag","Org","Wht"
};

// Convert color index → RGB components (for RF color commands to SENS board)
static void cidxToRGB(uint8_t ci, uint8_t &r, uint8_t &g, uint8_t &b) {
  if (ci>=NUM_COLORS) { r=0; g=0; b=0; return; }
  uint16_t c=COLORS565[ci];
  uint8_t r5=(c>>11)&0x1F, g6=(c>>5)&0x3F, b5=c&0x1F;
  r=(r5*527+23)>>6; g=(g6*259+33)>>6; b=(b5*527+23)>>6;
}

// ---- Per-key config ----
static const uint8_t NUM_KEYS = 8;
struct KeyCfg { uint8_t inst; uint8_t note; uint8_t colorIdx; };
KeyCfg keyCfg[NUM_KEYS];
uint32_t lastTrigMs[NUM_KEYS] = {0};
static const uint16_t MIN_TRIG_MS = 120;

// ---- LED color sync: "C,<1..8>,<colorIdx>\n" (colorIdx 0..7 or 255=off) ----
static void sendKeyColor(uint8_t k) {
  RF.print("C,"); RF.print(k+1); RF.print(",");
  RF.println(keyCfg[k].colorIdx);
}
static void sendAllColors() {
  for(uint8_t k=0;k<NUM_KEYS;k++) { sendKeyColor(k); delay(10); }
}

// ---- Auto-notes: if all keys share same instrument, assign Do..Do2 sequentially ----
static void checkAutoNotes() {
  uint8_t inst0=keyCfg[0].inst;
  for(uint8_t k=1;k<NUM_KEYS;k++) if(keyCfg[k].inst!=inst0) return;
  // All same — assign sequential notes
  for(uint8_t k=0;k<NUM_KEYS;k++) keyCfg[k].note=k&7;
  Serial.println("Auto-notes assigned (all instruments equal)");
}

// ---- Bitmap draw (for icon picker only) ----
static void drawBmp565(int x, int y, int w, int h, const uint16_t* data) {
  for (int j=0;j<h;j++) for (int i=0;i<w;i++)
    lcd.drawPixel(x+i,y+j,pgm_read_word(&data[j*w+i]));
}

// ---- Screen layout ----
enum Screen { SCR_MAIN, SCR_CFG, SCR_INST, SCR_COLOR };
Screen screen = SCR_MAIN;
uint8_t selKey = 0;
static const int KB_X0=10, KB_Y0=55, KB_W=300, KB_H=155;
int keyX[NUM_KEYS], keyW[NUM_KEYS];

static void calcLayout() {
  int base=KB_W/NUM_KEYS, rem=KB_W%NUM_KEYS, x=KB_X0;
  for (uint8_t i=0;i<NUM_KEYS;i++) { int w=base+((i<rem)?1:0); keyX[i]=x; keyW[i]=w; x+=w; }
}
static int hitKey(int16_t tx, int16_t ty) {
  // Accept ±30px outside key area to compensate for touch calibration error
  if (ty < KB_Y0-30 || ty > KB_Y0+KB_H+30) return -1;
  for (uint8_t i=0;i<NUM_KEYS;i++) if (tx>=keyX[i] && tx<(keyX[i]+keyW[i])) return i;
  if (tx < keyX[0]) return 0;           // left of first key → key 0
  return NUM_KEYS-1;                     // right of last key → key 7
}

// ---- waitRelease: waits until TIRQ is stably HIGH (finger truly released) ----
// FIX: old version broke out as soon as FIFO was drained (TIRQ briefly HIGH),
//      even if finger was still touching. STMPE610 re-queues a new sample
//      within ~10-50ms, causing a rapid-fire touch loop and a "stuck" UI.
//      Fix: require 3 consecutive HIGH readings separated by 30ms each.
static void waitRelease() {
  uint8_t clearCnt=0;
  uint32_t t0=millis();
  while (millis()-t0<1200) {
    lcd.drainFifo();
    delay(30);                       // let STMPE610 re-queue a sample if finger is down
    if (!lcd.touched()) {
      if (++clearCnt >= 3) return;   // 3 × 30ms clear = truly released
    } else {
      clearCnt=0;                    // finger still down — reset count
    }
  }
}

// ---- Button IDs ----
static const int BTN_BACK   = 1;
static const int BTN_NOTE_M = 2;
static const int BTN_NOTE_P = 3;
static const int BTN_INST   = 4;
static const int BTN_COLOR  = 5;
static const int BTN_NONE   = 6;

// ---- Draw tile (text-only; icons only in picker) ----
static void drawTile(uint8_t k) {
  int x=keyX[k], w=keyW[k], y=KB_Y0, h=KB_H;
  uint16_t barCol=(keyCfg[k].colorIdx<NUM_COLORS)?COLORS565[keyCfg[k].colorIdx]:DARKGREY;
  int mc=(w-4)/6; if(mc<1)mc=1; if(mc>7)mc=7;
  char b[8];
  lcd.fillRect(x,y,w,h,WHITE);
  lcd.drawRect(x,y,w,h,BLACK);
  lcd.fillRect(x+1,y+1,w-2,12,barCol);
  strncpy(b,INST_NAMES[keyCfg[k].inst],mc); b[mc]=0;
  lcd.print(x+2,y+18,b,1,BLACK,WHITE);
  strncpy(b,NOTE_NAMES[keyCfg[k].note],mc); b[mc]=0;
  lcd.print(x+2,y+34,b,1,RED,WHITE);
}

// ---- Main screen ----
static void drawMain() {
  lcd.clearButtons();
  calcLayout();
  lcd.fillScreen(BLACK);
  lcd.print(10,10,(char*)"VIRTUAL PIANO",2,YELLOW,BLACK);
  lcd.print(10,32,(char*)(dfReady?"DF:READY":"DF:INIT..."),1,CYAN,BLACK);
  lcd.drawRect(KB_X0-2,KB_Y0-2,KB_W+4,KB_H+4,CYAN);
  for (uint8_t k=0;k<NUM_KEYS;k++) drawTile(k);
  lcd.print(10,218,(char*)"Tap a tile to configure",1,GREEN,BLACK);
}

// ---- Config screen ----
static void drawCfg(uint8_t k) {
  lcd.clearButtons();
  lcd.fillScreen(BLACK);
  char hdr[24]; snprintf(hdr,sizeof(hdr),"KEY %u CONFIG",(unsigned)(k+1));
  lcd.print(10,8,hdr,2,YELLOW,BLACK);
  lcd.print(10,38,(char*)INST_NAMES[keyCfg[k].inst],2,WHITE,BLACK);
  char b[16]; snprintf(b,sizeof(b),"Note: %s",NOTE_NAMES[keyCfg[k].note]);
  lcd.print(10,65,b,2,CYAN,BLACK);
  uint16_t barCol=(keyCfg[k].colorIdx<NUM_COLORS)?COLORS565[keyCfg[k].colorIdx]:DARKGREY;
  lcd.fillRect(222,38,70,28,barCol);
  lcd.drawRect(222,38,70,28,WHITE);
  const char* cn=(keyCfg[k].colorIdx<NUM_COLORS)?COLOR_NAMES[keyCfg[k].colorIdx]:"None";
  lcd.print(224,70,(char*)cn,1,CYAN,BLACK);
  lcd.drawButton(BTN_NOTE_M, 10,100, 55,30,3,RED,    WHITE,"-",         2);
  lcd.drawButton(BTN_NOTE_P, 75,100, 55,30,3,GREEN,  WHITE,"+",         2);
  lcd.drawButton(BTN_INST,   10,145,145,30,3,WHITE,  BLACK,"INSTRUMENT",1);
  lcd.drawButton(BTN_COLOR, 165,145,145,30,3,MAGENTA,BLACK,"COLOR",     1);
  lcd.drawButton(BTN_BACK,  110,192,100,30,4,CYAN,   BLACK,"BACK",      2);
}

// ---- Instrument picker ----
static const int ICON_COLS=4;
static void iconPos(uint8_t idx, int &x, int &y) {
  uint8_t col=idx%ICON_COLS, row=idx/ICON_COLS;
  int mx=(320-ICON_COLS*ICON_W)/(ICON_COLS+1); if(mx<5)mx=5;
  x=mx+col*(ICON_W+mx); y=28+row*(ICON_H+12);
}
static void drawInstPicker(uint8_t k) {
  lcd.clearButtons();
  lcd.fillScreen(BLACK);
  char hdr[32]; snprintf(hdr,sizeof(hdr),"KEY %u: Instrument",(unsigned)(k+1));
  lcd.print(10,6,hdr,1,YELLOW,BLACK);
  for (uint8_t i=0;i<INST_COUNT;i++) {
    int x,y; iconPos(i,x,y);
    uint16_t border=(i==keyCfg[k].inst)?GREEN:WHITE;
    lcd.fillRect(x,y,ICON_W,ICON_H,DARKGREY);
    lcd.drawRect(x-2,y-2,ICON_W+4,ICON_H+4,border);
    if (INST_ICONS[i]) drawBmp565(x,y,ICON_W,ICON_H,INST_ICONS[i]);
    lcd.fillRect(x,y+ICON_H-10,ICON_W,10,BLACK);
    lcd.print(x+2,y+ICON_H-9,(char*)INST_NAMES[i],1,WHITE,BLACK);
  }
  lcd.drawButton(BTN_BACK,110,212,100,24,4,CYAN,BLACK,"BACK",2);
}
static int hitInst(int16_t tx, int16_t ty) {
  for (uint8_t i=0;i<INST_COUNT;i++) {
    int x,y; iconPos(i,x,y);
    if (tx>=x&&tx<x+ICON_W&&ty>=y&&ty<y+ICON_H) return i;
  }
  return -1;
}

// ---- Color picker ----
// Press NONE first to release current color, then tap a new color.
static const int CP_W=60, CP_H=38;
static void cpPos(uint8_t idx, int &x, int &y) {
  x=10+(idx%4)*(CP_W+10); y=30+(idx/4)*(CP_H+16);
}
static void drawColorPicker(uint8_t k) {
  lcd.clearButtons();
  lcd.fillScreen(BLACK);
  char hdr[24]; snprintf(hdr,sizeof(hdr),"KEY %u: Color",(unsigned)(k+1));
  lcd.print(10,8,hdr,1,YELLOW,BLACK);
  lcd.print(10,18,(char*)"Tap NONE to release, then pick",1,DARKGREY,BLACK);
  for (uint8_t c=0;c<NUM_COLORS;c++) {
    int x,y; cpPos(c,x,y);
    uint16_t border=(keyCfg[k].colorIdx==c)?GREEN:WHITE;
    lcd.fillRect(x,y,CP_W,CP_H,COLORS565[c]);
    lcd.drawRect(x-2,y-2,CP_W+4,CP_H+4,border);
    lcd.print(x+2,y+CP_H+2,(char*)COLOR_NAMES[c],1,WHITE,BLACK);
  }
  lcd.drawButton(BTN_NONE, 10,152, 80,26,3,DARKGREY,WHITE,"NONE",1);
  lcd.drawButton(BTN_BACK,110,152,100,26,4,CYAN,    BLACK,"BACK",2);
}
static int hitColor(int16_t tx, int16_t ty) {
  for (uint8_t c=0;c<NUM_COLORS;c++) {
    int x,y; cpPos(c,x,y);
    if (tx>=x&&tx<x+CP_W&&ty>=y&&ty<y+CP_H) return c;
  }
  return -1;
}

// ---- Trigger ----
// track = inst * 8 + note + 1  (1..64, matches 64 MP3 files in SD root)
static void handleTrigger(uint8_t s1to8, uint16_t mm) {
  if (s1to8<1||s1to8>8) return;
  uint8_t k=s1to8-1;
  uint32_t now=millis();
  if (now-lastTrigMs[k]<MIN_TRIG_MS) return;
  lastTrigMs[k]=now;

  uint8_t  inst  = keyCfg[k].inst;
  uint8_t  note  = keyCfg[k].note;
  uint16_t track = (uint16_t)inst * 8 + note + 1;   // 1..64

  Serial.print("TRIG S"); Serial.print(s1to8);
  Serial.print(" mm="); Serial.print(mm);
  Serial.print(" "); Serial.print(INST_NAMES[inst]);
  Serial.print(" "); Serial.print(NOTE_NAMES[note]);
  Serial.print(" track="); Serial.println(track);

  // Send current LED color to SENS board so it lights the right key
  sendKeyColor(k);

  if (!dfReady) { Serial.println("[WARN] DF not ready yet"); return; }
  dfPlay(k, track);

  if (screen==SCR_MAIN) drawTile(k);
}

// ---- RF ----
static void processRFLine(String line) {
  line.trim(); if (!line.length()) return;
  if (line.startsWith("T,")) {
    int p=line.indexOf(',',2); if (p<0) return;
    handleTrigger((uint8_t)line.substring(2,p).toInt(),
                  (uint16_t)line.substring(p+1).toInt());
    return;
  }
  // SENS board just booted — send it all current colors
  if (line.startsWith("BOOT,SENS")) {
    Serial.println("[RF] SENS boot detected — sending all colors");
    delay(200);        // give SENS board time to enter its loop
    sendAllColors();
    return;
  }
  if (line.startsWith("HELLO,SENS")) RF.print("HELLO_ACK,UI\n");
}
static void processRF() {
  while (RF.available()) {
    char c=(char)RF.read();
    if (c=='\r') continue;
    if (c=='\n') { processRFLine(rfLine); rfLine=""; }
    else { if (rfLine.length()<120) rfLine+=c; }
  }
}

// ---- Touch ----
// FIX: reset xTouch/yTouch before read so stale coords are never used
// FIX: waitRelease now waits for stable TIRQ HIGH (see definition above)
static void touchTick() {
  if (!lcd.touched()) return;
  delay(30);                        // debounce
  if (!lcd.touched()) return;       // noise spike — ignore

  lcd.xTouch = 0; lcd.yTouch = 0;  // reset before read
  lcd.readTouch();
  int16_t x=lcd.xTouch, y=lcd.yTouch;

  // --- Debug: print raw + mapped coords to Serial always ---
  Serial.printf("[TOUCH] raw=%d,%d mapped=%d,%d screen=%s\n",
    lcd.xRaw, lcd.yRaw, x, y,
    screen==SCR_MAIN?"MAIN":screen==SCR_CFG?"CFG":screen==SCR_INST?"INST":"COLOR");
  // Show mapped coords on screen (top-right corner) for visual calibration check
  { char dbuf[16]; snprintf(dbuf,sizeof(dbuf),"%3d,%3d",x,y);
    lcd.fillRect(230,0,90,10,0x0000);
    lcd.print(230,0,dbuf,1,0x07E0,0x0000); }

  if (x<=0 && y<=0) { waitRelease(); return; }  // read failed / invalid

  if (screen==SCR_MAIN) {
    int k=hitKey(x,y);
    if (k>=0) { Serial.print("[HIT] key="); Serial.println(k); selKey=(uint8_t)k; screen=SCR_CFG; Serial.println("[SCREEN] name=CFG"); drawCfg(selKey); }
    waitRelease(); return;
  }
  if (screen==SCR_CFG) {
    int8_t btn=lcd.ButtonTouch(x,y);
    if (btn==BTN_BACK)  { screen=SCR_MAIN;  Serial.println("[SCREEN] name=MAIN");  drawMain(); waitRelease(); return; }
    if (btn==BTN_INST)  { screen=SCR_INST;  Serial.println("[SCREEN] name=INST");  drawInstPicker(selKey); waitRelease(); return; }
    if (btn==BTN_COLOR) { screen=SCR_COLOR; Serial.println("[SCREEN] name=COLOR"); drawColorPicker(selKey); waitRelease(); return; }
    if (btn==BTN_NOTE_M) {
      keyCfg[selKey].note=(keyCfg[selKey].note==0)?7:(keyCfg[selKey].note-1);
      drawCfg(selKey); waitRelease(); return;
    }
    if (btn==BTN_NOTE_P) {
      keyCfg[selKey].note=(keyCfg[selKey].note+1)&7;
      drawCfg(selKey); waitRelease(); return;
    }
    waitRelease(); return;
  }
  if (screen==SCR_INST) {
    int8_t btn=lcd.ButtonTouch(x,y);
    if (btn==BTN_BACK) { screen=SCR_CFG; Serial.println("[SCREEN] name=CFG"); drawCfg(selKey); waitRelease(); return; }
    int inst=hitInst(x,y);
    if (inst>=0) {
      keyCfg[selKey].inst=(uint8_t)inst;
      checkAutoNotes();        // auto-assign Do..Do2 if all keys same instrument
      screen=SCR_CFG; Serial.println("[SCREEN] name=CFG"); drawCfg(selKey);
      // Redraw ALL tiles on main (auto-notes may have changed them all)
      // We'll refresh them when returning to main
    }
    waitRelease(); return;
  }
  if (screen==SCR_COLOR) {
    int8_t btn=lcd.ButtonTouch(x,y);
    if (btn==BTN_BACK)  { screen=SCR_CFG; Serial.println("[SCREEN] name=CFG"); drawCfg(selKey); waitRelease(); return; }
    if (btn==BTN_NONE)  {
      keyCfg[selKey].colorIdx=NO_COLOR;
      sendKeyColor(selKey);        // tell SENS board: LED off for this key
      drawColorPicker(selKey); waitRelease(); return;
    }
    int c=hitColor(x,y);
    if (c>=0) {
      keyCfg[selKey].colorIdx=(uint8_t)c;
      sendKeyColor(selKey);        // update SENS board with new color
      drawColorPicker(selKey);
    }
    waitRelease(); return;
  }
  waitRelease();
}

// ================================================================
void setup() {
  Serial.begin(115200); delay(300);
  Serial.println("\n=== VIRTUAL PIANO UI ===");

  Serial.println("[TFT] begin");
  lcd.begin();
  lcd.setTouch(3780, 372, 489, 3811);
  Serial.println(lcd.touchOk() ? "[TOUCH] OK" : "[TOUCH] FAIL");
  if (!lcd.touchOk()) {
    Serial.print("[TOUCH] diag tcs=15 tirq=35 tirqLevel=");
    Serial.println(digitalRead(35));
  }

  // Default key config: each key gets a unique instrument + sequential notes + color
  for (uint8_t k=0;k<NUM_KEYS;k++) {
    keyCfg[k].inst     = k;         // each key = different instrument
    keyCfg[k].note     = k & 7;     // Do Re Mi Fa Sol La Si Do2
    keyCfg[k].colorIdx = k % NUM_COLORS;
  }
  calcLayout();

  RF.begin(RF_BAUD, SERIAL_8N1, RF_RX_PIN, RF_TX_PIN);
  RF.print("BOOT,UI\n");
  Serial.println("RF UART RX=34 TX=13 @9600");

  // Send initial colors to SENS board (it may not be ready yet — BOOT,SENS handler
  // will re-send them when SENS announces itself)
  delay(300);
  sendAllColors();

  drawMain();
  Serial.println("[SCREEN] name=MAIN");
  dfInitStart();
}

void loop() {
  processRF();
  dfInitTick();
  touchTick();

  // Update DF:READY status label once
  static bool dfShown=false;
  if (dfReady && !dfShown) {
    dfShown=true;
    if (screen==SCR_MAIN) {
      lcd.fillRect(10,32,120,12,BLACK);
      lcd.print(10,32,(char*)"DF:READY",1,CYAN,BLACK);
    }
  }

  // Periodic color sync: re-send all colors every 15s in case SENS missed them
  static uint32_t colorSyncMs=0;
  if (millis()-colorSyncMs>=15000) { colorSyncMs=millis(); sendAllColors(); }

  // Heartbeat: alternating green/red dot proves loop is alive
  static uint32_t hbMs=0; static bool hb=false;
  if (millis()-hbMs>=500) { hbMs=millis(); hb=!hb; lcd.fillRect(305,10,10,10,hb?GREEN:RED); }

  delay(0);
}

