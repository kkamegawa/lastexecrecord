#pragma once

#include <cstdint>

namespace ler {

// Check if Windows Installer (msiexec.exe) or other installer processes are running
bool isInstallerRunning();

// Wait for installer processes to finish, with configurable timeout and retries
// Returns true if installer finished (or wasn't running), false if timed out
bool waitForInstallerToFinish(std::int64_t waitSeconds, std::int64_t maxRetries);

} // namespace ler
