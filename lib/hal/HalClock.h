#pragma once

#include <Arduino.h>

class HalClock;
extern HalClock halClock;

class HalClock {
 public:
  void begin();
  bool isAvailable() const { return available_; }
  bool getTime(uint8_t& hour, uint8_t& minute) const;
  bool formatTime(char* buf, size_t bufSize) const;

 private:
  enum class ChipVariant : uint8_t { Unknown = 0, Pcf85063, Pcf8563 };

  static constexpr unsigned long CLOCK_POLL_MS = 10000;

  bool probeVariant(ChipVariant variant);
  bool readRegisters(uint8_t startReg, uint8_t* data, size_t len) const;
  static uint8_t timeStartRegister(ChipVariant variant);
  static bool decodeRegisters(const uint8_t* data, uint8_t& hour, uint8_t& minute);

  ChipVariant variant_ = ChipVariant::Unknown;
  bool available_ = false;
  mutable uint8_t cachedHour_ = 0;
  mutable uint8_t cachedMinute_ = 0;
  mutable bool hasCachedTime_ = false;
  mutable unsigned long lastPollMs_ = 0;
};
