#include <gtest/gtest.h>

#include "TimeUtil.h"
#include <chrono>

// Known timestamp for testing: 2026-01-02T12:34:56Z
constexpr std::int64_t TEST_EPOCH_2026 = 1767430496;

TEST_CASE("TimeUtil: nowEpochSecondsUtc returns current time") {
    // Get current time using TimeUtil
    std::int64_t t1 = ler::nowEpochSecondsUtc();
    
    // Get current time using standard library
    auto now = std::chrono::system_clock::now();
    auto t2 = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()).count();
    
    // They should be within a few seconds of each other
    CHECK(std::abs(t1 - t2) <= 2);
}

TEST_CASE("TimeUtil: formatEpochSecondsAsIsoUtc formats correctly") {
    // Known timestamp: 2026-01-02T12:34:56Z
    std::wstring result = ler::formatEpochSecondsAsIsoUtc(TEST_EPOCH_2026);
    
    CHECK(result == L"2026-01-02T12:34:56Z");
}

TEST_CASE("TimeUtil: formatEpochSecondsAsIsoUtc handles epoch 0") {
    std::wstring result = ler::formatEpochSecondsAsIsoUtc(0);
    CHECK(result == L"1970-01-01T00:00:00Z");
}

TEST_CASE("TimeUtil: tryParseIsoUtcToEpochSeconds parses valid format with Z") {
    std::int64_t epoch = 0;
    bool success = ler::tryParseIsoUtcToEpochSeconds(L"2026-01-02T12:34:56Z", epoch);
    
    CHECK(success);
    CHECK(epoch == TEST_EPOCH_2026);
}

TEST_CASE("TimeUtil: tryParseIsoUtcToEpochSeconds parses valid format without Z") {
    std::int64_t epoch = 0;
    bool success = ler::tryParseIsoUtcToEpochSeconds(L"2026-01-02T12:34:56", epoch);
    
    CHECK(success);
    CHECK(epoch == TEST_EPOCH_2026);
}

TEST_CASE("TimeUtil: tryParseIsoUtcToEpochSeconds handles epoch 0") {
    std::int64_t epoch = 0;
    bool success = ler::tryParseIsoUtcToEpochSeconds(L"1970-01-01T00:00:00Z", epoch);
    
    CHECK(success);
    CHECK(epoch == 0);
}

TEST(TimeUtil, TryParseIsoUtcToEpochSeconds_RejectsInvalidFormat) {
    std::int64_t epoch = 0;
    
    SUBCASE("Empty string") {
        CHECK_FALSE(ler::tryParseIsoUtcToEpochSeconds(L"", epoch));
    }
    
    SUBCASE("Invalid date") {
        CHECK_FALSE(ler::tryParseIsoUtcToEpochSeconds(L"not-a-date", epoch));
    }
    
    SUBCASE("Wrong format") {
        CHECK_FALSE(ler::tryParseIsoUtcToEpochSeconds(L"2026/01/02 12:34:56", epoch));
    }
    
    SUBCASE("Invalid month") {
        CHECK_FALSE(ler::tryParseIsoUtcToEpochSeconds(L"2026-13-02T12:34:56Z", epoch));
    }
    
    SUBCASE("Invalid day") {
        CHECK_FALSE(ler::tryParseIsoUtcToEpochSeconds(L"2026-01-32T12:34:56Z", epoch));
    }
}

TEST_CASE("TimeUtil: Round-trip conversion") {
    std::int64_t original = TEST_EPOCH_2026;
    std::wstring formatted = ler::formatEpochSecondsAsIsoUtc(original);
    std::int64_t parsed = 0;
    bool success = ler::tryParseIsoUtcToEpochSeconds(formatted, parsed);
    
    CHECK(success);
    CHECK(parsed == original);
}
