#pragma once

#include <cstdint>
#include <string>

// A single bookmark entry representing a saved reading position in a book.
struct BookmarkEntry {
  std::string xpath;
  std::string summary;
  float percentage;

  uint16_t computedSpineIndex = 0;
  uint16_t computedChapterPageCount = 0;
  uint16_t computedChapterProgress = 0;
};
