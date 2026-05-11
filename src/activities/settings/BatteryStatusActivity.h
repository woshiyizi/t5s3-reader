#pragma once

#include "BoardT5S3.h"
#include "activities/Activity.h"

class BatteryStatusActivity final : public Activity {
 public:
  explicit BatteryStatusActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("BatteryStatus", renderer, mappedInput) {}

  void onEnter() override;
  void loop() override;
  bool onTouchTap(int16_t x, int16_t y) override;
  void render(RenderLock&&) override;
  bool preventAutoSleep() override { return true; }

 private:
  static constexpr uint32_t REFRESH_INTERVAL_MS = 3000;

  BoardT5S3::BatteryState state = {};
  bool hasState = false;
  uint32_t lastRefreshMs = 0;

  void refreshBattery();
};
