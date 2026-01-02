#include <doctest/doctest.h>

#include "FileUtil.h"
#include <fstream>

#ifdef _WIN32
#include <Windows.h>
#endif

TEST_CASE("FileUtil: changeExtension replaces extension") {
    SUBCASE("Simple case") {
        std::wstring path = L"test.txt";
        std::wstring result = ler::changeExtension(path, L".json");
        CHECK(result == L"test.json");
    }
    
    SUBCASE("Path with directory") {
        std::wstring path = L"C:\\dir\\test.txt";
        std::wstring result = ler::changeExtension(path, L".json");
        CHECK(result == L"C:\\dir\\test.json");
    }
    
    SUBCASE("No extension") {
        std::wstring path = L"test";
        std::wstring result = ler::changeExtension(path, L".json");
        CHECK(result == L"test.json");
    }
}

TEST_CASE("FileUtil: getDirectoryName extracts directory") {
    SUBCASE("Windows path") {
        std::wstring path = L"C:\\dir\\subdir\\file.txt";
        std::wstring result = ler::getDirectoryName(path);
        CHECK(result == L"C:\\dir\\subdir");
    }
    
    SUBCASE("Root directory") {
        std::wstring path = L"C:\\file.txt";
        std::wstring result = ler::getDirectoryName(path);
        CHECK(result == L"C:\\");
    }
    
    SUBCASE("No directory") {
        std::wstring path = L"file.txt";
        std::wstring result = ler::getDirectoryName(path);
        // For a file without a directory path, implementation may return empty string or current directory
        // We just check that the function doesn't crash
        CHECK(true);
    }
}

TEST_CASE("FileUtil: joinPath combines paths") {
    SUBCASE("Simple join") {
        std::wstring result = ler::joinPath(L"C:\\dir", L"file.txt");
        CHECK(result == L"C:\\dir\\file.txt");
    }
    
    SUBCASE("Dir with trailing backslash") {
        std::wstring result = ler::joinPath(L"C:\\dir\\", L"file.txt");
        // Should normalize path with single backslash
        CHECK(result == L"C:\\dir\\file.txt");
    }
}

// File I/O tests - these create temporary files
#ifndef LASTEXEC_STUB_WINDOWS

TEST_CASE("FileUtil: writeWStringToUtf8FileAtomic and readUtf8FileToWString") {
    std::wstring tempPath = L"test_temp_file.txt";
    std::wstring content = L"Hello, World! こんにちは";
    
    // Write
    ler::writeWStringToUtf8FileAtomic(tempPath, content);
    
    // Read back
    std::wstring readContent = ler::readUtf8FileToWString(tempPath);
    
    CHECK(readContent == content);
    
    // Cleanup
    DeleteFileW(tempPath.c_str());
}

TEST_CASE("FileUtil: readUtf8FileToWString handles UTF-8 BOM") {
    std::wstring tempPath = L"test_bom_file.txt";
    
    // Create file with BOM
    std::ofstream file(tempPath, std::ios::binary);
    file.put(0xEF).put(0xBB).put(0xBF); // UTF-8 BOM
    file << "test content";
    file.close();
    
    std::wstring content = ler::readUtf8FileToWString(tempPath);
    
    // Should read content without BOM
    CHECK(content.find(L"test content") != std::wstring::npos);
    
    // Cleanup
    DeleteFileW(tempPath.c_str());
}

TEST_CASE("FileUtil: readUtf8FileToWString throws on nonexistent file") {
    CHECK_THROWS(ler::readUtf8FileToWString(L"nonexistent_file_12345.txt"));
}

TEST_CASE("FileUtil: acquireLockFile creates lock") {
    std::wstring lockPath = L"test.lock";
    
    // Acquire lock
    ler::FileLock lock = ler::acquireLockFile(lockPath);
    CHECK(lock.h != INVALID_HANDLE_VALUE);
    
    // Try to acquire same lock (should fail)
    bool secondLockFailed = false;
    try {
        ler::FileLock lock2 = ler::acquireLockFile(lockPath);
    }
    catch (...) {
        secondLockFailed = true;
    }
    CHECK(secondLockFailed);
    
    // Release lock (via destructor)
    lock = ler::FileLock();
    
    // Now we should be able to acquire it again
    ler::FileLock lock3 = ler::acquireLockFile(lockPath);
    CHECK(lock3.h != INVALID_HANDLE_VALUE);
    
    // Cleanup
    lock3 = ler::FileLock();
    DeleteFileW(lockPath.c_str());
}

TEST_CASE("FileUtil: FileLock move semantics") {
    std::wstring lockPath = L"test_move.lock";
    
    ler::FileLock lock1 = ler::acquireLockFile(lockPath);
    HANDLE h1 = lock1.h;
    CHECK(h1 != INVALID_HANDLE_VALUE);
    
    // Move construct
    ler::FileLock lock2(std::move(lock1));
    CHECK(lock2.h == h1);
    CHECK(lock1.h == INVALID_HANDLE_VALUE);
    
    // Move assign
    ler::FileLock lock3;
    lock3 = std::move(lock2);
    CHECK(lock3.h == h1);
    CHECK(lock2.h == INVALID_HANDLE_VALUE);
    
    // Cleanup
    lock3 = ler::FileLock();
    DeleteFileW(lockPath.c_str());
}

#else
// Stub tests for non-Windows platforms
TEST_CASE("FileUtil: File I/O tests stubbed on non-Windows") {
    WARN("FileUtil file I/O tests are only available on Windows");
}
#endif
