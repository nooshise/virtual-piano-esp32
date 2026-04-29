#include "RFProtocol.h"

#include "LedStrip.h"
#include "SensorConfig.h"

HardwareSerial RF(1);
static String rfLine;

void rfSendTrig(uint8_t s1to8, uint16_t mm) {
  RF.print("T,");
  RF.print(s1to8);
  RF.print(",");
  RF.print(mm);
  RF.print("\n");
  Serial.print(">>RF T,");
  Serial.print(s1to8);
  Serial.print(",");
  Serial.println(mm);
  ledTrigger(s1to8 - 1);
}

static void processRFLine(String line) {
  line.trim();
  if (!line.length()) return;
  if (line.startsWith("C,")) {
    int p1 = line.indexOf(',', 2);
    if (p1 < 0) return;
    uint8_t ch = (uint8_t)line.substring(2, p1).toInt();
    uint8_t ci = (uint8_t)line.substring(p1 + 1).toInt();
    if (ch >= 1 && ch <= 8) {
      ledSetColorIndex(ch - 1, ci);
      Serial.print("LED CH");
      Serial.print(ch - 1);
      Serial.print(" ci=");
      Serial.println(ci);
    }
    return;
  }
  if (line.startsWith("HELLO,UI")) RF.print("HELLO_ACK,SENS\n");
}

void rfBegin() {
  RF.begin(RF_BAUD, SERIAL_8N1, RF_RX_PIN, RF_TX_PIN);
  RF.print("BOOT,SENS\n");
  Serial.println("RF @9600 RX=34 TX=13");
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
