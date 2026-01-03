#include <gtest/gtest.h>

#include "TimeUtil.h"

namespace {

TEST(TimeUtil, TryParseIso_ValidZ_ReturnsTrue) {
    std::int64_t epoch = -1;
    EXPECT_TRUE(ler::tryParseIsoUtcToEpochSeconds(L"2026-01-02T12:34:56Z", epoch));
}

TEST(TimeUtil, TryParseIso_ValidNoZ_TreatedAsUtc_ReturnsTrue) {
    std::int64_t epoch = -1;
    EXPECT_TRUE(ler::tryParseIsoUtcToEpochSeconds(L"2026-01-02T12:34:56", epoch));
}

TEST(TimeUtil, TryParseIso_InvalidLength_ReturnsFalse) {
    std::int64_t epoch = -1;
    EXPECT_FALSE(ler::tryParseIsoUtcToEpochSeconds(L"2026-01-02T12:34", epoch));
}

TEST(TimeUtil, TryParseIso_InvalidSeparator_ReturnsFalse) {
    std::int64_t epoch = -1;
    EXPECT_FALSE(ler::tryParseIsoUtcToEpochSeconds(L"2026/01/02T12:34:56Z", epoch));
}

TEST(TimeUtil, TryParseIso_FractionalSeconds_ReturnsFalse) {
    std::int64_t epoch = -1;
    EXPECT_FALSE(ler::tryParseIsoUtcToEpochSeconds(L"2026-01-02T12:34:56.123Z", epoch));
}

TEST(TimeUtil, FormatEpoch_Zero_ReturnsUnixEpoch) {
    // 0 should be 1970-01-01T00:00:00Z
    EXPECT_EQ(ler::formatEpochSecondsAsIsoUtc(0), L"1970-01-01T00:00:00Z");
}

TEST(TimeUtil, FormatAndParse_RoundTrip_ReturnsSameEpoch) {
    const std::int64_t samples[] = { 0, 1, 60, 3600, 1700000000 }; // arbitrary

    for (auto e : samples) {
        std::wstring s = ler::formatEpochSecondsAsIsoUtc(e);
        std::int64_t back = -1;
        ASSERT_TRUE(ler::tryParseIsoUtcToEpochSeconds(s, back)) << "formatted=" << std::string(s.begin(), s.end());
        EXPECT_EQ(back, e);
    }
}

} // namespace
