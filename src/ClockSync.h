#pragma once

#include <cstdint>

namespace ClockSync {

bool syncWithNtp(uint32_t timeoutMs, bool force);
bool commitCurrentSystemTime(const char* sourceTag = nullptr);
void stop();

}  // namespace ClockSync
