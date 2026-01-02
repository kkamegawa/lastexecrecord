#pragma once

#include <string>
#include <Windows.h>

namespace ler {

std::wstring getModulePath();
std::wstring changeExtension(const std::wstring& path, const std::wstring& extWithDot);
std::wstring getDirectoryName(const std::wstring& path);
std::wstring joinPath(const std::wstring& dir, const std::wstring& leaf);

// UTF-8 file IO (accepts UTF-8 with/without BOM)
std::wstring readUtf8FileToWString(const std::wstring& path);
void writeWStringToUtf8FileAtomic(const std::wstring& path, const std::wstring& content);

// simple lock using exclusive open
struct FileLock {
    HANDLE h = INVALID_HANDLE_VALUE;
    FileLock() = default;
    explicit FileLock(HANDLE handle) : h(handle) {}
    FileLock(const FileLock&) = delete;
    FileLock& operator=(const FileLock&) = delete;
    FileLock(FileLock&& other) noexcept : h(other.h) { other.h = INVALID_HANDLE_VALUE; }
    FileLock& operator=(FileLock&& other) noexcept {
        if (this != &other) {
            if (h != INVALID_HANDLE_VALUE) CloseHandle(h);
            h = other.h;
            other.h = INVALID_HANDLE_VALUE;
        }
        return *this;
    }
    ~FileLock() {
        if (h != INVALID_HANDLE_VALUE) CloseHandle(h);
    }
};

FileLock acquireLockFile(const std::wstring& lockPath);

} // namespace ler
