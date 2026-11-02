#include "SparkFunExplorableBoard.h"

#include <Wire.h>

void SparkFunExplorableBoard::begin() {
  _startup_reason = BD_STARTUP_NORMAL;
  Wire.begin();
  delay(10);
}
