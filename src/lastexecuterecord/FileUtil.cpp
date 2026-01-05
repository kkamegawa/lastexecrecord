#include "FileUtil.h"

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

static bool directoryExists(const std::wstring& path) {
    DWORD a = GetFileAttributesW(path.c_str());
    if (a == INVALID_FILE_ATTRIBUTES) return false;
    return (a & FILE_ATTRIBUTE_DIRECTORY) != 0;
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

    DWORD read = 0;
    if (size.QuadPart > 0) {
        if (!ReadFile(h, &bytes[0], static_cast<DWORD>(bytes.size()), &read, nullptr)) {
            CloseHandle(h);
            throw win32Error("ReadFile failed");
        }
        bytes.resize(read);
    }
    CloseHandle(h);

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
    std::wstring dir = getDirectoryName(path);
    std::wstring tmp = path + L".tmp";

    std::string bytes = wStringToUtf8(content);

    HANDLE h = CreateFileW(tmp.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) throw win32Error("CreateFileW(write tmp) failed");

    DWORD written = 0;
    if (!bytes.empty()) {
        if (!WriteFile(h, bytes.data(), static_cast<DWORD>(bytes.size()), &written, nullptr) || written != bytes.size()) {
            CloseHandle(h);
            DeleteFileW(tmp.c_str());
            throw win32Error("WriteFile failed");
        }
    }
    FlushFileBuffers(h);
    CloseHandle(h);

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

} // namespace ler
