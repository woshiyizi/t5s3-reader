#include "EpubReaderBookmarksActivity.h"

#include <GfxRenderer.h>
#include <HalStorage.h>
#include <I18n.h>
#include <JsonSettingsIO.h>

#include <algorithm>

#include "MappedInputManager.h"
#include "ProgressMapper.h"
#include "components/UITheme.h"
#include "fontIds.h"
#include "util/BookmarkUtil.h"

namespace {
constexpr int ENTER_DELETE_MODE_MS = 700;
constexpr int DELETE_MODE_OFF = 0;
constexpr int DELETE_MODE_DISPLAY = 1;
constexpr int DELETE_MODE_CONFIRM = 2;
constexpr int TITLE_HEIGHT = 60;
}  // namespace

void EpubReaderBookmarksActivity::onEnter() {
  Activity::onEnter();

  if (!epub) {
    return;
  }

  const std::string path = BookmarkUtil::getBookmarkPath(epubPath);
  if (Storage.exists(path.c_str())) {
    String json = Storage.readFile(path.c_str());
    if (json.isEmpty()) {
      LOG_ERR("EPB", "Failed to load bookmarks from %s. Empty bookmark file", path.c_str());
      bookmarks.clear();
    } else {
      JsonSettingsIO::loadBookmarks(bookmarks, json.c_str());
    }
  } else {
    LOG_DBG("EPB", "No bookmark file found at %s, starting with empty bookmarks", path.c_str());
    bookmarks.clear();
  }

  LOG_DBG("EPB", "Loaded %d bookmarks for book: %s", static_cast<int>(bookmarks.size()), epubPath.c_str());
  requestUpdate();
}

void EpubReaderBookmarksActivity::onExit() { Activity::onExit(); }

int EpubReaderBookmarksActivity::getGutterBottom(const GfxRenderer& renderer) const {
  const auto orientation = renderer.getOrientation();
  const bool isPortrait = orientation == GfxRenderer::Orientation::Portrait;
  return isPortrait ? 75 : 40;
}

int EpubReaderBookmarksActivity::getListHeight(const GfxRenderer& renderer) const {
  const auto pageHeight = renderer.getScreenHeight();
  return pageHeight - getGutterBottom(renderer) - TITLE_HEIGHT;
}

int EpubReaderBookmarksActivity::getRowHeight() const {
  return std::max(1, UITheme::getInstance().getMetrics().listWithSubtitleRowHeight);
}

int EpubReaderBookmarksActivity::getPageItems() const {
  return std::max(1, getListHeight(renderer) / getRowHeight());
}

int EpubReaderBookmarksActivity::getTouchedBookmarkIndex(const int16_t y) const {
  if (bookmarks.empty()) {
    return -1;
  }

  const auto orientation = renderer.getOrientation();
  const bool isPortraitInverted = orientation == GfxRenderer::Orientation::PortraitInverted;
  const int contentY = isPortraitInverted ? 50 : 0;
  const int listY = contentY + TITLE_HEIGHT;
  const int rowHeight = getRowHeight();
  const int pageItems = getPageItems();

  if (y < listY) {
    return -1;
  }

  const int row = (y - listY) / rowHeight;
  if (row < 0 || row >= pageItems) {
    return -1;
  }

  const int pageStartIndex = (selectorIndex / pageItems) * pageItems;
  const int touchedIndex = pageStartIndex + row;
  if (touchedIndex < 0 || touchedIndex >= static_cast<int>(bookmarks.size())) {
    return -1;
  }

  return touchedIndex;
}

void EpubReaderBookmarksActivity::openSelectedBookmark() {
  if (bookmarks.empty()) {
    return;
  }

  const auto& bookmark = bookmarks.at(selectorIndex);
  if (bookmark.computedChapterPageCount > 0) {
    LOG_DBG("EPB", "Opening bookmark using stored page %u/%u in spine %u", bookmark.computedChapterProgress,
            bookmark.computedChapterPageCount, bookmark.computedSpineIndex);
    setResult(ProgressChangeResult{bookmark.computedSpineIndex, bookmark.computedChapterProgress,
                                   bookmark.computedChapterPageCount});
    finish();
    return;
  }

  const auto pos = ProgressMapper::toCrossPoint(epub, {bookmark.xpath, bookmark.percentage},
                                                bookmark.computedSpineIndex, bookmark.computedChapterPageCount);
  setResult(ProgressChangeResult{pos.spineIndex, pos.pageNumber, pos.totalPages});
  finish();
}

void EpubReaderBookmarksActivity::deleteSelectedBookmark() {
  if (bookmarks.empty() || selectorIndex < 0 || selectorIndex >= static_cast<int>(bookmarks.size())) {
    return;
  }

  bookmarks.erase(bookmarks.begin() + selectorIndex);
  const std::string path = BookmarkUtil::getBookmarkPath(epubPath);
  Storage.mkdir("/.crosspoint");
  Storage.mkdir(BookmarkUtil::getBookmarksDir().c_str());
  if (!JsonSettingsIO::saveBookmarks(bookmarks, path.c_str())) {
    LOG_ERR("EPB", "Failed to save bookmarks after delete");
  }

  if (selectorIndex >= static_cast<int>(bookmarks.size()) && selectorIndex > 0) {
    selectorIndex--;
  }

  if (bookmarks.empty()) {
    ActivityResult result;
    result.isCancelled = true;
    setResult(std::move(result));
    finish();
    return;
  }

  confirmingDelete = DELETE_MODE_OFF;
  requestUpdate();
}

void EpubReaderBookmarksActivity::loop() {
  MappedInputManager::TouchPoint touchPoint{};
  unsigned long touchHeldMs = 0;
  if (confirmingDelete == DELETE_MODE_OFF && mappedInput.getTouchHold(touchPoint, touchHeldMs, renderer)) {
    const int touchedIndex = getTouchedBookmarkIndex(touchPoint.y);
    if (touchedIndex >= 0) {
      if (selectorIndex != touchedIndex) {
        selectorIndex = touchedIndex;
        requestUpdate();
      }

      if (touchHeldMs >= ENTER_DELETE_MODE_MS && !touchDeleteTriggered) {
        confirmingDelete = DELETE_MODE_DISPLAY;
        touchDeleteTriggered = true;
        ignoreNextTouchTap = true;
        requestUpdate();
        return;
      }
    }
  } else {
    touchDeleteTriggered = false;
  }

  if (confirmingDelete >= DELETE_MODE_DISPLAY) {
    if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
      if (confirmingDelete == DELETE_MODE_DISPLAY) {
        confirmingDelete = DELETE_MODE_CONFIRM;
        requestUpdate();
        return;
      }

      deleteSelectedBookmark();
      return;
    }

    if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
      confirmingDelete = DELETE_MODE_OFF;
      requestUpdate();
      return;
    }
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    if (bookmarks.empty()) {
      return;
    }

    openSelectedBookmark();
    return;
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    ActivityResult result;
    result.isCancelled = true;
    setResult(std::move(result));
    finish();
    return;
  }

  if (mappedInput.isPressed(MappedInputManager::Button::Confirm) &&
      mappedInput.getHeldTime() > ENTER_DELETE_MODE_MS) {
    if (!bookmarks.empty()) {
      confirmingDelete = DELETE_MODE_DISPLAY;
      requestUpdate();
    }
    return;
  }

  buttonNavigator.onNextRelease([this] {
    selectorIndex = ButtonNavigator::nextIndex(selectorIndex, static_cast<int>(bookmarks.size()));
    requestUpdate();
  });

  buttonNavigator.onPreviousRelease([this] {
    selectorIndex = ButtonNavigator::previousIndex(selectorIndex, static_cast<int>(bookmarks.size()));
    requestUpdate();
  });

  buttonNavigator.onNextContinuous([this] {
    selectorIndex = ButtonNavigator::nextPageIndex(selectorIndex, static_cast<int>(bookmarks.size()), getPageItems());
    requestUpdate();
  });

  buttonNavigator.onPreviousContinuous([this] {
    selectorIndex =
        ButtonNavigator::previousPageIndex(selectorIndex, static_cast<int>(bookmarks.size()), getPageItems());
    requestUpdate();
  });
}

bool EpubReaderBookmarksActivity::onTouchTap(int16_t, int16_t y) {
  if (ignoreNextTouchTap) {
    ignoreNextTouchTap = false;
    return true;
  }

  if (bookmarks.empty()) {
    return false;
  }

  if (confirmingDelete >= DELETE_MODE_DISPLAY) {
    const int rowHeight = getRowHeight();
    const int confirmRowY = renderer.getScreenHeight() / 2;
    if (y >= confirmRowY && y < confirmRowY + rowHeight) {
      if (confirmingDelete == DELETE_MODE_DISPLAY) {
        confirmingDelete = DELETE_MODE_CONFIRM;
        requestUpdate();
      } else {
        deleteSelectedBookmark();
      }
      return true;
    }
    return false;
  }

  const int touchedIndex = getTouchedBookmarkIndex(y);
  if (touchedIndex < 0) {
    return false;
  }

  selectorIndex = touchedIndex;
  openSelectedBookmark();
  return true;
}

void EpubReaderBookmarksActivity::render(RenderLock&&) {
  renderer.clearScreen();

  const auto pageWidth = renderer.getScreenWidth();
  const auto pageHeight = renderer.getScreenHeight();
  const auto orientation = renderer.getOrientation();
  const bool isLandscapeCw = orientation == GfxRenderer::Orientation::LandscapeClockwise;
  const bool isLandscapeCcw = orientation == GfxRenderer::Orientation::LandscapeCounterClockwise;
  const bool isPortraitInverted = orientation == GfxRenderer::Orientation::PortraitInverted;
  const int hintGutterWidth = (isLandscapeCw || isLandscapeCcw) ? 40 : 0;
  const int contentX = isLandscapeCw ? hintGutterWidth : 0;
  const int contentWidth = pageWidth - hintGutterWidth;
  const int contentY = isPortraitInverted ? 50 : 0;
  const int hintGutterBottom = getGutterBottom(renderer);
  const int listY = contentY + TITLE_HEIGHT;
  const int listHeight = getListHeight(renderer);
  const int rowHeight = getRowHeight();
  const int numBookmarks = static_cast<int>(bookmarks.size());

  const int titleX =
      contentX + (contentWidth - renderer.getTextWidth(UI_12_FONT_ID, tr(STR_BOOKMARKS), EpdFontFamily::BOLD)) / 2;
  renderer.drawText(UI_12_FONT_ID, titleX, 15 + contentY, tr(STR_BOOKMARKS), true, EpdFontFamily::BOLD);

  const auto getBookmarkTitle = [this](int index) -> std::string {
    return bookmarks.at(confirmingDelete >= DELETE_MODE_DISPLAY ? selectorIndex : index).summary;
  };
  const auto getBookmarkSubtitle = [this](int index) {
    const auto& bookmark = bookmarks.at(confirmingDelete >= DELETE_MODE_DISPLAY ? selectorIndex : index);
    const auto tocIndex = epub->getTocIndexForSpineIndex(bookmark.computedSpineIndex);
    const auto tocTitle = (tocIndex >= 0) ? epub->getTocItem(tocIndex).title : tr(STR_UNNAMED);
    std::string subtitle = std::to_string(static_cast<int>(std::clamp(bookmark.percentage, 0.0f, 1.0f) * 100.0f + 0.5f)) +
                           "% - ";
    if (bookmark.computedChapterPageCount > 0) {
      subtitle += std::to_string(bookmark.computedChapterProgress + 1) + "/" +
                  std::to_string(bookmark.computedChapterPageCount) + " - ";
    }
    return subtitle + tocTitle;
  };

  if (numBookmarks > 0) {
    if (confirmingDelete >= DELETE_MODE_DISPLAY) {
      GUI.drawHelpText(renderer, Rect{contentX, pageHeight / 2 - TITLE_HEIGHT, contentWidth, TITLE_HEIGHT},
                       tr(STR_CONFIRM_DELETE_BOOKMARK));
      GUI.drawList(renderer, Rect{contentX, pageHeight / 2, contentWidth, rowHeight}, 1, 0, getBookmarkTitle,
                   getBookmarkSubtitle);
    } else {
      GUI.drawList(renderer, Rect{contentX, listY, contentWidth, listHeight}, numBookmarks, selectorIndex,
                   getBookmarkTitle, getBookmarkSubtitle);
      GUI.drawHelpText(renderer, Rect{contentX, pageHeight - hintGutterBottom, contentWidth, TITLE_HEIGHT},
                       tr(STR_HOLD_OPEN_TO_DELETE));
    }
  }

  const auto backLabel = confirmingDelete >= DELETE_MODE_DISPLAY ? tr(STR_CANCEL) : tr(STR_BACK);
  const auto confirmLabel =
      numBookmarks > 0 ? (confirmingDelete >= DELETE_MODE_DISPLAY ? tr(STR_DELETE) : tr(STR_SELECT)) : "";
  const auto labels = mappedInput.mapLabels(backLabel, confirmLabel, tr(STR_DIR_UP), tr(STR_DIR_DOWN));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

  renderer.displayBuffer();
}
