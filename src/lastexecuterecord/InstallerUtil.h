#pragma once

#include <cstdint>

namespace ler {

enum class InstallerCheckStatus {
    NotRunning,
    Running,
    CheckFailed,
};

struct InstallerCheckResult {
    InstallerCheckStatus status = InstallerCheckStatus::CheckFailed;
    std::uint32_t errorCode = 0;
};

// Check installer process status and report snapshot/enumeration failures.
InstallerCheckResult checkInstallerRunning();

// Check if Windows Installer (msiexec.exe) or other installer processes are running
bool isInstallerRunning();

// Wait for installer processes to finish, with configurable timeout and retries
// Returns true if installer finished (or wasn't running), false if timed out
bool waitForInstallerToFinish(std::int64_t waitSeconds, std::int64_t maxRetries);

} // namespace ler
