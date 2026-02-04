#include "InstallerUtil.h"

#include <Windows.h>
#include <TlHelp32.h>
#include <algorithm>
#include <cwctype>
#include <limits>

namespace ler {

// Convert string to lowercase for case-insensitive comparison
static std::wstring toLower(const std::wstring& str) {
    std::wstring result = str;
    std::transform(result.begin(), result.end(), result.begin(),
        [](wchar_t c) { return std::towlower(c); });
    return result;
}

// Check if a given process name matches an installer process
static bool isInstallerProcess(const std::wstring& processName) {
    std::wstring lower = toLower(processName);
    
    // Common Windows installer processes
    return lower == L"msiexec.exe" ||
           lower == L"setup.exe" ||
           lower == L"install.exe" ||
           lower == L"setupapi.exe" ||
           lower == L"msiinstall.exe" ||
           lower == L"windows installer.exe";
}

bool isInstallerRunning() {
    // Create a snapshot of all running processes
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return false; // Can't check, assume not running
    }

    PROCESSENTRY32W pe32{};
    pe32.dwSize = sizeof(PROCESSENTRY32W);

    bool installerFound = false;

    // Get the first process
    if (Process32FirstW(hSnapshot, &pe32)) {
        do {
            std::wstring exeFile(pe32.szExeFile);
            if (isInstallerProcess(exeFile)) {
                installerFound = true;
                break;
            }
        } while (Process32NextW(hSnapshot, &pe32));
    }

    CloseHandle(hSnapshot);
    return installerFound;
}

bool waitForInstallerToFinish(std::int64_t waitSeconds, std::int64_t maxRetries) {
    if (maxRetries <= 0) {
        maxRetries = 1; // At least check once
    }

    for (std::int64_t attempt = 0; attempt < maxRetries; attempt++) {
        if (!isInstallerRunning()) {
            return true; // Installer finished or not running
        }

        // Don't sleep on the last attempt - just return the result
        if (attempt < maxRetries - 1) {
            // Convert seconds to milliseconds, being careful of overflow
            DWORD sleepMs;
            if (waitSeconds > (std::numeric_limits<DWORD>::max)() / 1000) {
                sleepMs = (std::numeric_limits<DWORD>::max)();
            } else {
                sleepMs = static_cast<DWORD>(waitSeconds * 1000);
            }
            Sleep(sleepMs);
        }
    }

    // Still running after all retries
    return false;
}

} // namespace ler
