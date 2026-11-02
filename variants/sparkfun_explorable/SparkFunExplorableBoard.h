#pragma once

#include <Arduino.h>
#include <MeshCore.h>
#include <helpers/KeyValueStore.h>

class SparkFunExplorableBoard : public mesh::MainBoard {
  uint8_t _startup_reason;

public:
  void begin();

  void attachDynamicPrefs(KeyValueStore* prefs) { }
  uint16_t getBattMilliVolts() override { return 0; }
  const char* getManufacturerName() const override { return "SparkFun expLoRaBLE"; }
  uint8_t getStartupReason() const override { return _startup_reason; }
  void reboot() override { NVIC_SystemReset(); }
};
