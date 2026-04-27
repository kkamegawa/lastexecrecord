#include "CommandRunner.h"

#include <Windows.h>
#include <limits>
#include <stdexcept>

namespace ler {

namespace {

class ScopedHandle {
public:
    ScopedHandle() = default;
    explicit ScopedHandle(HANDLE handle) : h_(handle) {}
    ScopedHandle(const ScopedHandle&) = delete;
    ScopedHandle& operator=(const ScopedHandle&) = delete;

    ScopedHandle(ScopedHandle&& other) noexcept : h_(other.h_) {
        other.h_ = INVALID_HANDLE_VALUE;
    }

    ScopedHandle& operator=(ScopedHandle&& other) noexcept {
        if (this != &other) {
            reset();
            h_ = other.h_;
            other.h_ = INVALID_HANDLE_VALUE;
        }
        return *this;
    }

    ~ScopedHandle() {
        reset();
    }

    HANDLE get() const { return h_; }

    void reset(HANDLE handle = INVALID_HANDLE_VALUE) {
        if (h_ != INVALID_HANDLE_VALUE && h_ != nullptr) {
            CloseHandle(h_);
        }
        h_ = handle;
    }

private:
    HANDLE h_ = INVALID_HANDLE_VALUE;
};

} // namespace

std::wstring quoteArgForWindowsCommandLine(const std::wstring& arg) {
    // Rules aligned with CommandLineToArgvW / MS C runtime parsing.
    if (arg.empty()) return L"\"\"";

    bool needsQuotes = false;
    for (wchar_t c : arg) {
        if (c == L' ' || c == L'\t' || c == L'\n' || c == L'\v' || c == L'\"') {
            needsQuotes = true;
            break;
        }
    }
    if (!needsQuotes && !arg.empty() && arg.back() == L'\\') {
        // Trailing backslashes require quoting so they can be preserved correctly.
        needsQuotes = true;
    }
    if (!needsQuotes) return arg;

    std::wstring out;
    out.push_back(L'\"');

    size_t i = 0;
    while (i < arg.size()) {
        size_t backslashes = 0;
        while (i < arg.size() && arg[i] == L'\\') {
            backslashes++;
            i++;
        }

        if (i == arg.size()) {
            // escape all backslashes at end
            out.append(backslashes * 2, L'\\');
            break;
        }

        if (arg[i] == L'\"') {
            // escape backslashes and the quote
            out.append(backslashes * 2 + 1, L'\\');
            out.push_back(L'\"');
            i++;
            continue;
        }

        // normal char
        out.append(backslashes, L'\\');
        out.push_back(arg[i]);
        i++;
    }

    out.push_back(L'\"');
    return out;
}

static std::wstring buildCommandLine(const std::wstring& exePath, const std::vector<std::wstring>& args) {
    std::wstring cmd;
    cmd += quoteArgForWindowsCommandLine(exePath);
    for (const auto& a : args) {
        cmd.push_back(L' ');
        cmd += quoteArgForWindowsCommandLine(a);
    }
    return cmd;
}

RunResult runProcess(const std::wstring& exePath,
    const std::vector<std::wstring>& args,
    const std::wstring& workingDirectory,
    std::int64_t timeoutSeconds) {

    RunResult rr;

    std::wstring cmdLine = buildCommandLine(exePath, args);
    // CreateProcessW can modify the buffer
    std::vector<wchar_t> cmdBuf(cmdLine.begin(), cmdLine.end());
    cmdBuf.push_back(L'\0');

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};

    BOOL ok = CreateProcessW(
        exePath.c_str(),
        cmdBuf.data(),
        nullptr,
        nullptr,
        FALSE,
        0,
        nullptr,
        workingDirectory.empty() ? nullptr : workingDirectory.c_str(),
        &si,
        &pi);

    if (!ok) {
        rr.started = false;
        rr.exitCode = GetLastError();
        return rr;
    }

    rr.started = true;
    ScopedHandle threadHandle(pi.hThread);
    ScopedHandle processHandle(pi.hProcess);

    DWORD waitMs = INFINITE;
    if (timeoutSeconds > 0) {
        if (timeoutSeconds > (std::numeric_limits<DWORD>::max)() / 1000) {
            waitMs = (std::numeric_limits<DWORD>::max)();
        }
        else {
            waitMs = static_cast<DWORD>(timeoutSeconds * 1000);
        }
    }

    DWORD w = WaitForSingleObject(processHandle.get(), waitMs);
    if (w == WAIT_TIMEOUT) {
        rr.timedOut = true;
        if (!TerminateProcess(processHandle.get(), 1)) {
            rr.exitCode = GetLastError();
            return rr;
        }
        DWORD terminateWait = WaitForSingleObject(processHandle.get(), 5000);
        if (terminateWait == WAIT_FAILED) {
            rr.exitCode = GetLastError();
            return rr;
        }
    }
    else if (w == WAIT_FAILED) {
        rr.exitCode = GetLastError();
        return rr;
    }

    DWORD ec = 0;
    if (!GetExitCodeProcess(processHandle.get(), &ec)) {
        rr.exitCode = GetLastError();
        return rr;
    }
    rr.exitCode = ec;

    return rr;
}

} // namespace ler
