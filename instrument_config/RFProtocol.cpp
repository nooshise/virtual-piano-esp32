#include "RFProtocol.h"

#include "DFPlayers.h"
#include "Instruments.h"
#include "TrackMap.h"
#include "UIDraw.h"
#include "UIState.h"

HardwareSerial RF(1);
static String rfLine;

static void handleTrigger(uint8_t s1to8, uint16_t mm) {
  if (s1to8 < 1 || s1to8 > NUM_KEYS) return;
  uint8_t k = s1to8 - 1;
  uint32_t now = millis();
  if (now - lastTrigMs[k] < MIN_TRIG_MS) return;
  lastTrigMs[k] = now;

  uint8_t inst = keyCfg[k].inst;
  uint8_t note = keyCfg[k].note;
  uint16_t track = trackFor(inst, note);

  Serial.print("[RF TRIG] key=");
  Serial.print(s1to8);
  Serial.print(" mm=");
  Serial.print(mm);
  Serial.print(" instrument=");
  Serial.print(INST_NAMES[inst]);
  Serial.print(" note=");
  Serial.print(NOTE_NAMES[note]);
  Serial.print(" track=");
  Serial.println(track);

  sendKeyColor(k);
  if (!dfIsReady()) {
    Serial.println("[WARN] DF not ready yet");
    return;
  }
  dfPlayTrack(k, track);
  if (screen == SCR_MAIN) drawTile(k);
}

static void processRFLine(String line) {
  line.trim();
  if (!line.length()) return;
  if (line.startsWith("T,")) {
    int p = line.indexOf(',', 2);
    if (p < 0) return;
    handleTrigger((uint8_t)line.substring(2, p).toInt(),
                  (uint16_t)line.substring(p + 1).toInt());
    return;
  }
  if (line.startsWith("BOOT,SENS")) {
    Serial.println("[RF] SENS boot detected - sending all colors");
    delay(200);
    sendAllColors();
    return;
  }
  if (line.startsWith("HELLO,SENS")) RF.print("HELLO_ACK,UI\n");
}

void rfBegin() {
  RF.begin(RF_BAUD, SERIAL_8N1, RF_RX_PIN, RF_TX_PIN);
  RF.print("BOOT,UI\n");
  Serial.println("RF UART RX=34 TX=13 @9600");
}

void rfTick() {
  while (RF.available()) {
    char c = (char)RF.read();
    if (c == '\r') continue;
    if (c == '\n') {
      processRFLine(rfLine);
      rfLine = "";
    } else if (rfLine.length() < 120) {
      rfLine += c;
    }
  }
}

void sendKeyColor(uint8_t keyIndex) {
  if (keyIndex >= NUM_KEYS) return;
  RF.print("C,");
  RF.print(keyIndex + 1);
  RF.print(",");
  RF.println(keyCfg[keyIndex].colorIdx);
}

void sendAllColors() {
  for (uint8_t k = 0; k < NUM_KEYS; k++) {
    sendKeyColor(k);
    delay(10);
  }
}
