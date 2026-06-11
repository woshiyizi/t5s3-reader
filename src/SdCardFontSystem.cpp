#include "SdCardFontSystem.h"

#include <GfxRenderer.h>
#include <Logging.h>

#include <cstdlib>

#include "CrossPointSettings.h"

namespace {

constexpr uint8_t kUiContentFontPointSize = 12;

uint8_t fontSizeEnumFromSettings() {
  uint8_t e = SETTINGS.fontSize;
  if (e >= CrossPointSettings::FONT_SIZE_COUNT) e = 1;  // default to MEDIUM
  return e;
}

uint8_t readerPointSizeForFamily(const SdCardFontFamilyInfo& family, uint8_t fontSizeEnum) {
  auto sizes = family.availableSizes();
  if (sizes.empty()) {
    return 0;
  }

  uint8_t idx = fontSizeEnum;
  if (idx >= sizes.size()) {
    idx = sizes.size() - 1;
  }
  return sizes[idx];
}

uint8_t closestPointSizeForFamily(const SdCardFontFamilyInfo& family, uint8_t targetPointSize) {
  auto sizes = family.availableSizes();
  if (sizes.empty()) {
    return 0;
  }

  uint8_t bestSize = sizes.front();
  int bestDelta = std::abs(static_cast<int>(bestSize) - static_cast<int>(targetPointSize));
  for (const auto size : sizes) {
    const int delta = std::abs(static_cast<int>(size) - static_cast<int>(targetPointSize));
    if (delta < bestDelta || (delta == bestDelta && size < bestSize)) {
      bestSize = size;
      bestDelta = delta;
    }
  }

  return bestSize;
}

}  // namespace

void SdCardFontSystem::begin(GfxRenderer& renderer) {
  registry_.discover();

  SETTINGS.sdFontIdResolver = [](void* ctx, const char* familyName, uint8_t fontSizeEnum) -> int {
    return static_cast<SdCardFontSystem*>(ctx)->resolveFontId(familyName, fontSizeEnum);
  };
  SETTINGS.sdFontResolverCtx = this;

  if (SETTINGS.sdFontFamilyName[0] != '\0') {
    const auto* family = registry_.findFamily(SETTINGS.sdFontFamilyName);
    if (family) {
      if (manager_.loadFamily(*family, renderer, fontSizeEnumFromSettings(), kUiContentFontPointSize)) {
        LOG_DBG("SDFS", "Loaded SD card font family: %s", SETTINGS.sdFontFamilyName);
      } else {
        LOG_ERR("SDFS", "Failed to load SD font family: %s (clearing)", SETTINGS.sdFontFamilyName);
        SETTINGS.sdFontFamilyName[0] = '\0';
      }
    } else {
      LOG_DBG("SDFS", "SD font family not found on card: %s (clearing)", SETTINGS.sdFontFamilyName);
      SETTINGS.sdFontFamilyName[0] = '\0';
    }
  }

  LOG_DBG("SDFS", "SD font system ready (%d families discovered)", registry_.getFamilyCount());
}

void SdCardFontSystem::ensureLoaded(GfxRenderer& renderer) {
  const bool registryWasDirty = registryDirty_.exchange(false, std::memory_order_acquire);
  if (registryWasDirty) {
    LOG_DBG("SDFS", "Registry dirty - re-discovering fonts");
    registry_.discover();
  }

  const char* wantedFamily = SETTINGS.sdFontFamilyName;
  const std::string& currentFamily = manager_.currentFamilyName();
  const uint8_t sizeEnum = fontSizeEnumFromSettings();

  if (wantedFamily[0] == '\0') {
    if (!currentFamily.empty()) {
      manager_.unloadAll(renderer);
    }
    return;
  }

  bool familyMatches = (currentFamily == wantedFamily);
  if (familyMatches) {
    const auto* family = registry_.findFamily(wantedFamily);
    if (!family) {
      LOG_DBG("SDFS", "SD font family disappeared: %s (clearing)", wantedFamily);
      manager_.unloadAll(renderer);
      SETTINGS.sdFontFamilyName[0] = '\0';
      return;
    }

    const uint8_t wantedReaderPt = readerPointSizeForFamily(*family, sizeEnum);
    const uint8_t wantedUiPt = closestPointSizeForFamily(*family, kUiContentFontPointSize);
    if (!registryWasDirty && wantedReaderPt == manager_.currentReaderPointSize() &&
        wantedUiPt == manager_.currentUiContentPointSize()) {
      return;
    }

    LOG_DBG("SDFS", "Reloading %s: reader %u -> %u (enum %u), ui %u -> %u%s", wantedFamily,
            manager_.currentReaderPointSize(), wantedReaderPt, sizeEnum, manager_.currentUiContentPointSize(),
            wantedUiPt, registryWasDirty ? " [registry dirty]" : "");
  }

  if (!currentFamily.empty()) {
    manager_.unloadAll(renderer);
  }

  const auto* family = registry_.findFamily(wantedFamily);
  if (family) {
    if (manager_.loadFamily(*family, renderer, sizeEnum, kUiContentFontPointSize)) {
      LOG_DBG("SDFS", "Loaded SD font family: %s", wantedFamily);
    } else {
      LOG_ERR("SDFS", "Failed to load SD font family: %s (clearing)", wantedFamily);
      SETTINGS.sdFontFamilyName[0] = '\0';
    }
  } else {
    LOG_DBG("SDFS", "SD font family not found: %s (clearing)", wantedFamily);
    SETTINGS.sdFontFamilyName[0] = '\0';
  }
}

int SdCardFontSystem::resolveFontId(const char* familyName, uint8_t fontSizeEnum) const {
  return manager_.getFontId(familyName ? familyName : "", fontSizeEnum);
}

int SdCardFontSystem::getUserContentFontId() const {
  if (SETTINGS.sdFontFamilyName[0] == '\0') {
    return 0;
  }
  return manager_.getUiContentFontId(SETTINGS.sdFontFamilyName);
}
