#include "HalClock.h"

#include <BoardT5S3.h>
#include <Logging.h>
#include <Wire.h>

namespace {
constexpr uint8_t kRtcAddress = T5S3_PCF85063_ADDR;
constexpr uint8_t kRegisterCount = 7;

uint8_t bcdToDec(uint8_t bcd) { return static_cast<uint8_t>(((bcd >> 4) * 10) + (bcd & 0x0F)); }

bool isValidBcd(uint8_t bcd, uint8_t maxValue) {
  if ((bcd & 0x0F) > 9 || ((bcd >> 4) & 0x0F) > 9) {
    return false;
  }
  return bcdToDec(bcd) <= maxValue;
}
}  // namespace

HalClock halClock;

void HalClock::begin() {
  available_ = false;
  variant_ = ChipVariant::Unknown;
  hasCachedTime_ = false;
  lastPollMs_ = 0;

  const bool hasPcf85063 = probeVariant(ChipVariant::Pcf85063);
  const bool hasPcf8563 = probeVariant(ChipVariant::Pcf8563);

  if (hasPcf85063) {
    variant_ = ChipVariant::Pcf85063;
    available_ = true;
    LOG_INF("CLK", hasPcf8563 ? "RTC detected at 0x51 (PCF85063/PCF8563-compatible, using PCF85063 layout)"
                              : "RTC detected at 0x51 (PCF85063 layout)");
  } else if (hasPcf8563) {
    variant_ = ChipVariant::Pcf8563;
    available_ = true;
    LOG_INF("CLK", "RTC detected at 0x51 (PCF8563 layout)");
  } else {
    LOG_INF("CLK", "RTC not found at 0x51");
    return;
  }

  uint8_t hour = 0;
  uint8_t minute = 0;
  (void)getTime(hour, minute);
}

bool HalClock::getTime(uint8_t& hour, uint8_t& minute) const {
  if (!available_) {
    return false;
  }

  const unsigned long now = millis();
  if (hasCachedTime_ && lastPollMs_ != 0 && (now - lastPollMs_) < CLOCK_POLL_MS) {
    hour = cachedHour_;
    minute = cachedMinute_;
    return true;
  }

  uint8_t data[kRegisterCount] = {};
  if (!readRegisters(timeStartRegister(variant_), data, sizeof(data)) || !decodeRegisters(data, cachedHour_, cachedMinute_)) {
    if (!hasCachedTime_) {
      return false;
    }
    hour = cachedHour_;
    minute = cachedMinute_;
    return true;
  }

  hasCachedTime_ = true;
  lastPollMs_ = now;
  hour = cachedHour_;
  minute = cachedMinute_;
  return true;
}

bool HalClock::formatTime(char* buf, size_t bufSize) const {
  if (buf == nullptr || bufSize < 6) {
    return false;
  }

  uint8_t hour = 0;
  uint8_t minute = 0;
  if (!getTime(hour, minute)) {
    return false;
  }

  snprintf(buf, bufSize, "%02u:%02u", static_cast<unsigned>(hour), static_cast<unsigned>(minute));
  return true;
}

bool HalClock::probeVariant(const ChipVariant variant) {
  uint8_t data[kRegisterCount] = {};
  uint8_t hour = 0;
  uint8_t minute = 0;
  return readRegisters(timeStartRegister(variant), data, sizeof(data)) && decodeRegisters(data, hour, minute);
}

bool HalClock::readRegisters(const uint8_t startReg, uint8_t* data, const size_t len) const {
  if (data == nullptr || len == 0) {
    return false;
  }

  BoardT5S3::ScopedI2CLock lock;
  Wire.beginTransmission(kRtcAddress);
  Wire.write(startReg);
  if (Wire.endTransmission(false) != 0) {
    return false;
  }

  const uint8_t requested = static_cast<uint8_t>(len);
  if (Wire.requestFrom(kRtcAddress, requested) != requested) {
    while (Wire.available()) {
      Wire.read();
    }
    return false;
  }

  for (size_t i = 0; i < len; ++i) {
    data[i] = Wire.read();
  }
  return true;
}

uint8_t HalClock::timeStartRegister(const ChipVariant variant) {
  switch (variant) {
    case ChipVariant::Pcf85063:
      return 0x04;
    case ChipVariant::Pcf8563:
    case ChipVariant::Unknown:
    default:
      return 0x02;
  }
}

bool HalClock::decodeRegisters(const uint8_t* data, uint8_t& hour, uint8_t& minute) {
  if (data == nullptr) {
    return false;
  }

  const uint8_t rawSeconds = data[0] & 0x7F;
  const uint8_t rawMinutes = data[1] & 0x7F;
  const uint8_t rawHours = data[2] & 0x3F;
  const uint8_t rawDays = data[3] & 0x3F;
  const uint8_t rawWeekdays = data[4] & 0x07;
  const uint8_t rawMonths = data[5] & 0x1F;
  const uint8_t rawYears = data[6];

  if (!isValidBcd(rawSeconds, 59) || !isValidBcd(rawMinutes, 59) || !isValidBcd(rawHours, 23) ||
      !isValidBcd(rawDays, 31) || !isValidBcd(rawMonths, 12) || !isValidBcd(rawYears, 99)) {
    return false;
  }

  const uint8_t day = bcdToDec(rawDays);
  const uint8_t month = bcdToDec(rawMonths);
  if (day == 0 || month == 0 || rawWeekdays > 6) {
    return false;
  }

  hour = bcdToDec(rawHours);
  minute = bcdToDec(rawMinutes);
  return true;
}
