#include "TcaMux.h"

#include <Wire.h>
#include "SensorConfig.h"

void i2cBegin() {
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(I2C_HZ);
  delay(300);
}

bool tcaPingWithRetry() {
  bool tcaOk = false;
  for (int i = 0; i < 10 && !tcaOk; i++) {
    Wire.beginTransmission(TCA_ADDR);
    tcaOk = (Wire.endTransmission() == 0);
    if (!tcaOk) delay(50);
  }
  Serial.println(tcaOk ? "TCA OK @0x70" : "TCA MISSING @0x70 - check wiring!");
  return tcaOk;
}

bool tcaSelect(uint8_t ch) {
  if (ch > 7) return false;
  Wire.beginTransmission(TCA_ADDR);
  Wire.write(1 << ch);
  bool ok = (Wire.endTransmission() == 0);
  delay(1);
  return ok;
}

bool i2cPing(uint8_t addr) {
  Wire.beginTransmission(addr);
  return (Wire.endTransmission() == 0);
}

bool i2cReg16(uint8_t addr, uint16_t reg, uint8_t &val) {
  Wire.beginTransmission(addr);
  Wire.write((uint8_t)(reg >> 8));
  Wire.write((uint8_t)(reg & 0xFF));
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((int)addr, 1) != 1) return false;
  val = Wire.read();
  return true;
}

void i2cRecover() {
  Serial.println("I2C bus recovery...");
  pinMode(I2C_SCL, OUTPUT);
  pinMode(I2C_SDA, INPUT_PULLUP);
  for (int i = 0; i < 9; i++) {
    digitalWrite(I2C_SCL, LOW);
    delayMicroseconds(10);
    digitalWrite(I2C_SCL, HIGH);
    delayMicroseconds(10);
    if (digitalRead(I2C_SDA)) break;
  }
  pinMode(I2C_SDA, OUTPUT);
  digitalWrite(I2C_SDA, LOW);
  delayMicroseconds(10);
  digitalWrite(I2C_SCL, HIGH);
  delayMicroseconds(10);
  digitalWrite(I2C_SDA, HIGH);
  delayMicroseconds(10);
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(I2C_HZ);
  delay(20);
}
