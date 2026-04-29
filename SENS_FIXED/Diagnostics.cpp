#include "Diagnostics.h"

const char* sensorTypeName(SensorType t) {
  return t == ST_VL618 ? "VL6180X" : t == ST_VL53 ? "VL53L0X" : "NONE";
}
