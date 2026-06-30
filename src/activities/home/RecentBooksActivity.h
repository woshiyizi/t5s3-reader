#pragma once
#include <I18n.h>

#include <functional>
#include <string>
#include <vector>

#include "../Activity.h"
#include "RecentBooksStore.h"
#include "util/ButtonNavigator.h"

class RecentBooksActivity final : public Activity {
 private:
  ButtonNavigator buttonNavigator;

  size_t selectorIndex = 0;

  // Recent tab state
  std::vector<RecentBook> recentBooks;
  std::string lastVisibleTextPrewarmKey;
  int confirmingDelete = 0;
  bool touchDeleteTriggered = false;
  bool ignoreNextTouchTap = false;

  // Data loading
  void loadRecentBooks();
  int getPageItems() const;
  int getTouchedRecentBookIndex(int16_t y) const;
  void removeSelectedRecentBook();

 public:
  explicit RecentBooksActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("RecentBooks", renderer, mappedInput) {}
  void onEnter() override;
  void onExit() override;
  void loop() override;
  bool onTouchTap(int16_t x, int16_t y) override;
  void render(RenderLock&&) override;
};
