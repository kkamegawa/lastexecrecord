#pragma once

#include <cstdint>
#include <string>
#include <Windows.h>

namespace ler {

std::int64_t nowEpochSecondsUtc();

// Accepts: YYYY-MM-DDTHH:MM:SSZ or without trailing Z (treated as UTC)
bool tryParseIsoUtcToEpochSeconds(const std::wstring& iso, std::int64_t& outEpochSeconds);
std::wstring formatEpochSecondsAsIsoUtc(std::int64_t epochSeconds);

} // namespace ler
