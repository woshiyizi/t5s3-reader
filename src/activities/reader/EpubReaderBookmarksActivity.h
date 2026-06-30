#pragma once

#include <Epub.h>

#include <memory>
#include <vector>

#include "../../BookmarkEntry.h"
#include "../Activity.h"
#include "util/ButtonNavigator.h"

class EpubReaderBookmarksActivity final : public Activity {
  std::shared_ptr<Epub> epub;
  std::string epubPath;
  ButtonNavigator buttonNavigator;
  int selectorIndex = 0;
  std::vector<BookmarkEntry> bookmarks;
  int confirmingDelete = 0;
  bool ignoreNextTouchTap = false;
  bool touchDeleteTriggered = false;

 public:
  explicit EpubReaderBookmarksActivity(GfxRenderer& renderer, MappedInputManager& mappedInput,
                                       const std::shared_ptr<Epub>& epub, const std::string& epubPath)
      : Activity("EpubReaderBookmarks", renderer, mappedInput), epub(epub), epubPath(epubPath) {}

  void onEnter() override;
  void onExit() override;
  void loop() override;
  bool onTouchTap(int16_t x, int16_t y) override;
  void render(RenderLock&&) override;

 private:
  int getGutterBottom(const GfxRenderer& renderer) const;
  int getListHeight(const GfxRenderer& renderer) const;
  int getRowHeight() const;
  int getPageItems() const;
  int getTouchedBookmarkIndex(int16_t y) const;
  void openSelectedBookmark();
  void deleteSelectedBookmark();
};
