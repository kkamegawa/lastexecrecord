#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace ler {

struct RunResult {
    bool started = false;
    bool timedOut = false;
    std::uint32_t exitCode = 0;
};

std::wstring quoteArgForWindowsCommandLine(const std::wstring& arg);

RunResult runProcess(const std::wstring& exePath,
    const std::vector<std::wstring>& args,
    const std::wstring& workingDirectory,
    std::int64_t timeoutSeconds);

} // namespace ler
