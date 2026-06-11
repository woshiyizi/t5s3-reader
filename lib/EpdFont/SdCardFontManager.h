#pragma once

#include <cstdint>
#include <string>
#include <vector>

class GfxRenderer;
class SdCardFont;
struct SdCardFontFileInfo;
struct SdCardFontFamilyInfo;

class SdCardFontManager {
 public:
  SdCardFontManager() = default;
  ~SdCardFontManager();
  SdCardFontManager(const SdCardFontManager&) = delete;
  SdCardFontManager& operator=(const SdCardFontManager&) = delete;

  // Load the font file matching the current reader font size enum and a
  // separate fixed-size UI content font from the same family.
  // Returns true on success.
  bool loadFamily(const SdCardFontFamilyInfo& family, GfxRenderer& renderer, uint8_t readerFontSizeEnum,
                  uint8_t uiContentPointSize);

  // Unload everything, unregister from renderer.
  void unloadAll(GfxRenderer& renderer);

  // Look up the font ID for the loaded family and role. Returns 0 if nothing
  // loaded, familyName doesn't match, or the requested role is unavailable.
  int getFontId(const std::string& familyName, uint8_t fontSizeEnum) const;
  int getUiContentFontId(const std::string& familyName) const;

  // Get name of currently loaded family (empty if none).
  const std::string& currentFamilyName() const { return loadedFamilyName_; };

  uint8_t currentReaderPointSize() const { return loadedReaderPointSize_; }
  uint8_t currentUiContentPointSize() const { return loadedUiContentPointSize_; }

 private:
  struct LoadedFont {
    SdCardFont* font;  // heap-allocated, owned
    int fontId;
    uint8_t size;
  };
  static int computeFontId(uint32_t contentHash, const char* familyName, uint8_t pointSize);
  bool loadFontFile(const SdCardFontFamilyInfo& family, const SdCardFontFileInfo& fileInfo, GfxRenderer& renderer,
                    int& outFontId);

  std::string loadedFamilyName_;
  uint8_t loadedReaderSizeEnum_ = 0;
  uint8_t loadedReaderPointSize_ = 0;
  uint8_t loadedUiContentPointSize_ = 0;
  int readerFontId_ = 0;
  int uiContentFontId_ = 0;
  std::vector<LoadedFont> loaded_;
};
