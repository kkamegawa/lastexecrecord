#include "FileUtil.h"

#include <cwctype>
#include <limits>
#include <stdexcept>
#include <vector>

namespace ler {

static std::runtime_error win32Error(const char* msg) {
    DWORD e = GetLastError();
    return std::runtime_error(std::string(msg) + " (GetLastError=" + std::to_string(e) + ")");
}

std::wstring getEnvVar(const wchar_t* name) {
    DWORD n = GetEnvironmentVariableW(name, nullptr, 0);
    if (n == 0) {
        DWORD e = GetLastError();
        if (e == ERROR_ENVVAR_NOT_FOUND) return L"";
        throw win32Error("GetEnvironmentVariableW failed");
    }
    std::wstring v;
    v.resize(n);
    DWORD r = GetEnvironmentVariableW(name, &v[0], n);
    if (r == 0) throw win32Error("GetEnvironmentVariableW failed");
    if (!v.empty() && v.back() == L'\0') v.pop_back();
    return v;
}

bool fileExists(const std::wstring& path) {
    DWORD a = GetFileAttributesW(path.c_str());
    if (a == INVALID_FILE_ATTRIBUTES) return false;
    return (a & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

bool directoryExists(const std::wstring& path) {
    DWORD a = GetFileAttributesW(path.c_str());
    if (a == INVALID_FILE_ATTRIBUTES) return false;
    return (a & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

bool pathIsAbsolute(const std::wstring& path) {
    if (path.size() >= 3 &&
        std::iswalpha(path[0]) &&
        path[1] == L':' &&
        (path[2] == L'\\' || path[2] == L'/')) {
        return true;
    }
    return path.size() >= 2 && path[0] == L'\\' && path[1] == L'\\';
}

void ensureDirectoryExists(const std::wstring& path) {
    if (path.empty()) return;
    if (directoryExists(path)) return;

    // Create parent first
    std::wstring parent = getDirectoryName(path);
    if (!parent.empty() && parent != path) {
        ensureDirectoryExists(parent);
    }

    if (!CreateDirectoryW(path.c_str(), nullptr)) {
        DWORD e = GetLastError();
        if (e == ERROR_ALREADY_EXISTS && directoryExists(path)) return;
        throw win32Error("CreateDirectoryW failed");
    }
}

std::wstring getModulePath() {
    std::wstring buf;
    buf.resize(32768);
    DWORD n = GetModuleFileNameW(nullptr, &buf[0], static_cast<DWORD>(buf.size()));
    if (n == 0 || n >= buf.size()) throw win32Error("GetModuleFileNameW failed");
    buf.resize(n);
    return buf;
}

std::wstring getFullPath(const std::wstring& path) {
    DWORD needed = GetFullPathNameW(path.c_str(), 0, nullptr, nullptr);
    if (needed == 0) throw win32Error("GetFullPathNameW failed");

    std::wstring full;
    full.resize(needed);
    DWORD n = GetFullPathNameW(path.c_str(), needed, &full[0], nullptr);
    if (n == 0 || n >= needed) throw win32Error("GetFullPathNameW failed");
    full.resize(n);
    return full;
}

std::wstring getSystemDirectoryPath() {
    std::wstring buf;
    buf.resize(MAX_PATH);
    UINT n = GetSystemDirectoryW(&buf[0], static_cast<UINT>(buf.size()));
    if (n == 0) throw win32Error("GetSystemDirectoryW failed");
    if (n >= buf.size()) {
        buf.resize(static_cast<size_t>(n) + 1);
        n = GetSystemDirectoryW(&buf[0], static_cast<UINT>(buf.size()));
        if (n == 0 || n >= buf.size()) throw win32Error("GetSystemDirectoryW failed");
    }
    buf.resize(n);
    return buf;
}

std::wstring changeExtension(const std::wstring& path, const std::wstring& extWithDot) {
    size_t dot = path.find_last_of(L'.');
    size_t slash = path.find_last_of(L"\\/");
    if (dot == std::wstring::npos || (slash != std::wstring::npos && dot < slash)) {
        return path + extWithDot;
    }
    return path.substr(0, dot) + extWithDot;
}

std::wstring getDirectoryName(const std::wstring& path) {
    size_t slash = path.find_last_of(L"\\/");
    if (slash == std::wstring::npos) return L"";
    return path.substr(0, slash);
}

std::wstring joinPath(const std::wstring& dir, const std::wstring& leaf) {
    if (dir.empty()) return leaf;
    if (dir.back() == L'\\' || dir.back() == L'/') return dir + leaf;
    return dir + L"\\" + leaf;
}

static std::wstring utf8ToWString(const std::string& s) {
    if (s.empty()) return L"";
    int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.data(), static_cast<int>(s.size()), nullptr, 0);
    if (n <= 0) throw win32Error("MultiByteToWideChar failed");
    std::wstring w;
    w.resize(n);
    int r = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.data(), static_cast<int>(s.size()), &w[0], n);
    if (r != n) throw win32Error("MultiByteToWideChar failed");
    return w;
}

static std::string wStringToUtf8(const std::wstring& w) {
    if (w.empty()) return "";
    int n = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, w.data(), static_cast<int>(w.size()), nullptr, 0, nullptr, nullptr);
    if (n <= 0) throw win32Error("WideCharToMultiByte failed");
    std::string s;
    s.resize(n);
    int r = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, w.data(), static_cast<int>(w.size()), &s[0], n, nullptr, nullptr);
    if (r != n) throw win32Error("WideCharToMultiByte failed");
    return s;
}

std::wstring readUtf8FileToWString(const std::wstring& path) {
    HANDLE h = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) throw win32Error("CreateFileW(read) failed");

    LARGE_INTEGER size{};
    if (!GetFileSizeEx(h, &size)) {
        CloseHandle(h);
        throw win32Error("GetFileSizeEx failed");
    }
    if (size.QuadPart > 64LL * 1024LL * 1024LL) {
        CloseHandle(h);
        throw std::runtime_error("Config file too large");
    }

    std::string bytes;
    bytes.resize(static_cast<size_t>(size.QuadPart));

    if (size.QuadPart > 0) {
        size_t totalRead = 0;
        while (totalRead < bytes.size()) {
            DWORD toRead = static_cast<DWORD>(bytes.size() - totalRead);
            DWORD bytesRead = 0;
            if (!ReadFile(h, &bytes[totalRead], toRead, &bytesRead, nullptr)) {
                CloseHandle(h);
                throw win32Error("ReadFile failed");
            }
            if (bytesRead == 0) {
                CloseHandle(h);
                throw std::runtime_error("ReadFile returned fewer bytes than expected");
            }
            totalRead += bytesRead;
        }
    }
    if (!CloseHandle(h)) throw win32Error("CloseHandle(read) failed");

    // strip UTF-8 BOM
    if (bytes.size() >= 3 &&
        static_cast<unsigned char>(bytes[0]) == 0xEF &&
        static_cast<unsigned char>(bytes[1]) == 0xBB &&
        static_cast<unsigned char>(bytes[2]) == 0xBF) {
        bytes = bytes.substr(3);
    }
    return utf8ToWString(bytes);
}

void writeWStringToUtf8FileAtomic(const std::wstring& path, const std::wstring& content) {
    std::string bytes = wStringToUtf8(content);
    if (bytes.size() > (std::numeric_limits<DWORD>::max)()) {
        throw std::runtime_error("Content too large to write");
    }

    std::wstring tmp;
    HANDLE h = INVALID_HANDLE_VALUE;
    for (int attempt = 0; attempt < 100; ++attempt) {
        tmp = path + L"." + std::to_wstring(GetCurrentProcessId()) + L"." +
            std::to_wstring(GetTickCount64()) + L"." + std::to_wstring(attempt) + L".tmp";
        h = CreateFileW(tmp.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW,
            FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_TEMPORARY, nullptr);
        if (h != INVALID_HANDLE_VALUE) break;
        DWORD e = GetLastError();
        if (e != ERROR_FILE_EXISTS && e != ERROR_ALREADY_EXISTS) {
            throw win32Error("CreateFileW(write tmp) failed");
        }
    }
    if (h == INVALID_HANDLE_VALUE) throw std::runtime_error("Unable to create unique temporary file");

    DWORD written = 0;
    if (!bytes.empty()) {
        if (!WriteFile(h, bytes.data(), static_cast<DWORD>(bytes.size()), &written, nullptr) || written != bytes.size()) {
            CloseHandle(h);
            DeleteFileW(tmp.c_str());
            throw win32Error("WriteFile failed");
        }
    }
    if (!FlushFileBuffers(h)) {
        CloseHandle(h);
        DeleteFileW(tmp.c_str());
        throw win32Error("FlushFileBuffers failed");
    }
    if (!CloseHandle(h)) {
        DeleteFileW(tmp.c_str());
        throw win32Error("CloseHandle(write tmp) failed");
    }

    if (!MoveFileExW(tmp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        DeleteFileW(tmp.c_str());
        throw win32Error("MoveFileExW failed");
    }
}

FileLock acquireLockFile(const std::wstring& lockPath) {
    HANDLE h = CreateFileW(lockPath.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
        OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) throw win32Error("Failed to acquire lock file");
    return FileLock(h);
}

FileLock tryAcquireLockFile(const std::wstring& lockPath) {
    HANDLE h = CreateFileW(lockPath.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
        OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) {
        DWORD e = GetLastError();
        if (e == ERROR_SHARING_VIOLATION || e == ERROR_LOCK_VIOLATION) {
            return FileLock{}; // Lock held by another process; caller should retry
        }
        throw win32Error("Failed to acquire lock file");
    }
    return FileLock(h);
}

} // namespace ler
