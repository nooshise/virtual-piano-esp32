#include "TriggerState.h"

#include <Arduino.h>
#include "Diagnostics.h"
#include "RFProtocol.h"
#include "SensorConfig.h"
#include "ToFSensors.h"

void tickSensors() {
  uint32_t now = millis();
  for (uint8_t ch = 0; ch < NCH; ch++) {
    if (!chs[ch].ok) continue;
    if (now - chs[ch].lastReadMs < READ_PERIOD_MS) continue;
    chs[ch].lastReadMs = now;

    uint16_t mm = 0;
    if (!readMm(ch, mm)) {
      if (++chs[ch].failStreak >= FAIL_RECOVER) {
        chs[ch].failStreak = 0;
        recoverChannel(ch);
      }
      continue;
    }
    chs[ch].failStreak = 0;
    if (now - chs[ch].lastLogMs >= DIST_LOG_MS) {
      chs[ch].lastLogMs = now;
      Serial.print("DIST CH");
      Serial.print(ch);
      Serial.print(" [");
      Serial.print(sensorTypeName(chs[ch].type));
      Serial.print("]");
      Serial.print(" mm=");
      Serial.print(mm);
      Serial.print(" enter<");
      Serial.print(chs[ch].enterThr);
      Serial.print(" exit<");
      Serial.print(chs[ch].exitThr);
      Serial.print(" state=");
      Serial.print(chs[ch].nearState ? "NEAR" : "FAR");
      Serial.print(" armed=");
      Serial.println(chs[ch].armed ? "Y" : "N");
    }

    bool near = chs[ch].nearState ? (mm <= chs[ch].exitThr) : (mm <= chs[ch].enterThr);

    if (near) {
      if (chs[ch].nearCnt < 255) chs[ch].nearCnt++;
      chs[ch].farCnt = 0;
    } else {
      if (chs[ch].farCnt < 255) chs[ch].farCnt++;
      chs[ch].nearCnt = 0;
    }

    if (!chs[ch].nearState && near && chs[ch].nearCnt >= NEAR_STABLE) {
      chs[ch].nearState = true;
      if (chs[ch].armed && now - chs[ch].lastTrigMs >= MIN_TRIG_GAP) {
        Serial.print("TRIGGER CH");
        Serial.print(ch);
        Serial.print(" mm=");
        Serial.print(mm);
        Serial.print(" nearCnt=");
        Serial.println(chs[ch].nearCnt);
        chs[ch].lastTrigMs = now;
        chs[ch].armed = false;
        rfSendTrig(ch + 1, mm);
      }
    }
    if (chs[ch].nearState && !near && chs[ch].farCnt >= FAR_STABLE) {
      chs[ch].nearState = false;
      chs[ch].armed = true;
      Serial.print("REARM CH");
      Serial.print(ch);
      Serial.print(" mm=");
      Serial.print(mm);
      Serial.print(" farCnt=");
      Serial.println(chs[ch].farCnt);
    }
  }
}
