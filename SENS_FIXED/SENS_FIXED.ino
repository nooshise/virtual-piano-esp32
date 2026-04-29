#include <Arduino.h>

#include "LedStrip.h"
#include "RFProtocol.h"
#include "SensorConfig.h"
#include "TcaMux.h"
#include "ToFSensors.h"
#include "TriggerState.h"

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n=== SENS: TCA + MIXED-TOF + RF + CAL + NEOPIXEL ===");

  ledBegin();

  i2cBegin();
  tcaPingWithRetry();

  rfBegin();

  initAllSensors();

  bool active[NCH];
  for (uint8_t i = 0; i < NCH; i++) active[i] = chs[i].ok;
  ledShowActiveSensors(active, NCH);

  Serial.println("--- Calibrating in 2s - KEEP SENSORS CLEAR! ---");
  ledCalibrationWarning(active, NCH);
  calibrateAllSensors();
  Serial.println("--- Ready ---");
}

void loop() {
  rfTick();
  tickSensors();
  ledTick();
  delay(0);
}
