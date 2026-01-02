#include "TimeUtil.h"

#include <iomanip>
#include <sstream>

namespace ler {

static std::int64_t fileTimeToEpochSeconds(const FILETIME& ft) {
    ULARGE_INTEGER uli;
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    // FILETIME is 100ns since 1601-01-01
    static const std::uint64_t EPOCH_DIFF_100NS = 116444736000000000ULL;
    if (uli.QuadPart < EPOCH_DIFF_100NS) return 0;
    std::uint64_t unix100ns = uli.QuadPart - EPOCH_DIFF_100NS;
    return static_cast<std::int64_t>(unix100ns / 10000000ULL);
}

static FILETIME epochSecondsToFileTime(std::int64_t epochSeconds) {
    static const std::uint64_t EPOCH_DIFF_100NS = 116444736000000000ULL;
    std::uint64_t unix100ns = static_cast<std::uint64_t>(epochSeconds) * 10000000ULL;
    ULARGE_INTEGER uli;
    uli.QuadPart = unix100ns + EPOCH_DIFF_100NS;
    FILETIME ft;
    ft.dwLowDateTime = uli.LowPart;
    ft.dwHighDateTime = uli.HighPart;
    return ft;
}

std::int64_t nowEpochSecondsUtc() {
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    return fileTimeToEpochSeconds(ft);
}

static bool parse2(const std::wstring& s, size_t off, int& out) {
    if (off + 2 > s.size()) return false;
    if (s[off] < L'0' || s[off] > L'9' || s[off + 1] < L'0' || s[off + 1] > L'9') return false;
    out = (s[off] - L'0') * 10 + (s[off + 1] - L'0');
    return true;
}

static bool parse4(const std::wstring& s, size_t off, int& out) {
    if (off + 4 > s.size()) return false;
    out = 0;
    for (size_t i = 0; i < 4; i++) {
        wchar_t c = s[off + i];
        if (c < L'0' || c > L'9') return false;
        out = out * 10 + (c - L'0');
    }
    return true;
}

bool tryParseIsoUtcToEpochSeconds(const std::wstring& iso, std::int64_t& outEpochSeconds) {
    // Expected minimal length 19: YYYY-MM-DDTHH:MM:SS
    if (iso.size() < 19) return false;

    int y = 0, mon = 0, d = 0, hh = 0, mm = 0, ss = 0;
    if (!parse4(iso, 0, y)) return false;
    if (iso[4] != L'-') return false;
    if (!parse2(iso, 5, mon)) return false;
    if (iso[7] != L'-') return false;
    if (!parse2(iso, 8, d)) return false;
    if (iso[10] != L'T' && iso[10] != L't' && iso[10] != L' ') return false;
    if (!parse2(iso, 11, hh)) return false;
    if (iso[13] != L':') return false;
    if (!parse2(iso, 14, mm)) return false;
    if (iso[16] != L':') return false;
    if (!parse2(iso, 17, ss)) return false;

    // allow trailing Z or nothing
    if (iso.size() > 19) {
        if (!(iso.size() == 20 && (iso[19] == L'Z' || iso[19] == L'z'))) {
            // ignore fractional/offsets for now
            return false;
        }
    }

    SYSTEMTIME st{};
    st.wYear = static_cast<WORD>(y);
    st.wMonth = static_cast<WORD>(mon);
    st.wDay = static_cast<WORD>(d);
    st.wHour = static_cast<WORD>(hh);
    st.wMinute = static_cast<WORD>(mm);
    st.wSecond = static_cast<WORD>(ss);

    FILETIME ft{};
    if (!SystemTimeToFileTime(&st, &ft)) return false;

    outEpochSeconds = fileTimeToEpochSeconds(ft);
    return true;
}

std::wstring formatEpochSecondsAsIsoUtc(std::int64_t epochSeconds) {
    FILETIME ft = epochSecondsToFileTime(epochSeconds);
    SYSTEMTIME st{};
    FileTimeToSystemTime(&ft, &st);

    std::wstringstream ss;
    ss << std::setfill(L'0')
        << std::setw(4) << st.wYear << L"-"
        << std::setw(2) << st.wMonth << L"-"
        << std::setw(2) << st.wDay << L"T"
        << std::setw(2) << st.wHour << L":"
        << std::setw(2) << st.wMinute << L":"
        << std::setw(2) << st.wSecond << L"Z";
    return ss.str();
}

} // namespace ler
