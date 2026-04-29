#include "DFPlayers.h"

#include <SoftwareSerial.h>
#include "ProjectConfig.h"

static SoftwareSerial* dfSS[N_PLAYERS] = {nullptr};
static bool dfReady = false;
static uint8_t dfStage = 0;
static uint32_t dfNextMs = 0;

static void dfSend(uint8_t p, uint8_t cmd, uint16_t param) {
  if (p >= N_PLAYERS || !dfSS[p]) return;
  uint8_t dh = (param >> 8) & 0xFF;
  uint8_t dl = param & 0xFF;
  const uint8_t ver = 0xFF;
  const uint8_t len = 0x06;
  const uint8_t fb = 0x00;
  uint16_t sum = (uint16_t)ver + len + cmd + fb + dh + dl;
  uint16_t chk = (uint16_t)(0xFFFF - sum + 1);
  uint8_t fr[10] = {0x7E, ver, len, cmd, fb, dh, dl, (uint8_t)(chk >> 8), (uint8_t)chk, 0xEF};
  dfSS[p]->write(fr, 10);
}

static void dfSetVol(uint8_t p, uint8_t v) {
  dfSend(p, 0x06, v);
}

static void dfStop(uint8_t p) {
  dfSend(p, 0x16, 0);
}

static void dfPlay(uint8_t p, uint16_t t) {
  if (t < 1) t = 1;
  if (t > 64) t = 64;
  dfSend(p, 0x03, t);
}

void dfInitStart() {
  dfReady = false;
  dfStage = 0;
  dfNextMs = millis() + DF_BOOT_MS;
  for (uint8_t i = 0; i < N_PLAYERS; i++) {
    if (!dfSS[i]) {
      dfSS[i] = new SoftwareSerial(DF_RX_DUMMY, DF_TX_PINS[i]);
      dfSS[i]->begin(9600);
    }
  }
  Serial.print("DF boot wait ");
  Serial.print(DF_BOOT_MS);
  Serial.println("ms...");
}

void dfInitTick() {
  if (dfReady) return;
  uint32_t now = millis();
  if (now < dfNextMs) return;
  if (dfStage == 0) {
    Serial.println("=== DF INIT (8 SoftwareSerial TX) ===");
    dfStage = 1;
    dfNextMs = now + 20;
    return;
  }
  uint8_t p = dfStage - 1;
  if (p < N_PLAYERS) {
    Serial.print("DF");
    Serial.print(p + 1);
    Serial.print(" GPIO");
    Serial.print(DF_TX_PINS[p]);
    Serial.println("...");
    dfSetVol(p, DF_VOLUME);
    delay(DF_GAP_MS);
    dfStop(p);
    delay(DF_GAP_MS);
    dfStage++;
    dfNextMs = now + 120;
    return;
  }
  dfReady = true;
  Serial.println("DF ALL READY");
}

bool dfIsReady() {
  return dfReady;
}

void dfPlayTrack(uint8_t playerIndex, uint16_t track) {
  if (playerIndex >= N_PLAYERS) return;
  Serial.print("[PLAY] player=");
  Serial.print(playerIndex + 1);
  Serial.print(" track=");
  Serial.println(track);
  dfPlay(playerIndex, track);
}
