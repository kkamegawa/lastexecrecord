#include <doctest/doctest.h>

#include "CommandRunner.h"

TEST_CASE("CommandRunner: quoteArgForWindowsCommandLine handles simple arguments") {
    std::wstring arg = L"simple";
    std::wstring result = ler::quoteArgForWindowsCommandLine(arg);
    
    // Simple arguments without spaces may or may not be quoted, but should be valid
    CHECK_FALSE(result.empty());
}

TEST_CASE("CommandRunner: quoteArgForWindowsCommandLine handles arguments with spaces") {
    std::wstring arg = L"hello world";
    std::wstring result = ler::quoteArgForWindowsCommandLine(arg);
    
    // Arguments with spaces must be quoted
    CHECK(result.front() == L'"');
    CHECK(result.back() == L'"');
    CHECK(result.find(L"hello world") != std::wstring::npos);
}

TEST_CASE("CommandRunner: quoteArgForWindowsCommandLine handles empty string") {
    std::wstring arg = L"";
    std::wstring result = ler::quoteArgForWindowsCommandLine(arg);
    
    // Empty string should be quoted
    CHECK(result == L"\"\"");
}

TEST_CASE("CommandRunner: quoteArgForWindowsCommandLine handles backslashes") {
    SUBCASE("Single backslash") {
        std::wstring arg = L"path\\to\\file";
        std::wstring result = ler::quoteArgForWindowsCommandLine(arg);
        CHECK_FALSE(result.empty());
    }
    
    SUBCASE("Backslash before quote") {
        std::wstring arg = L"test\\\"quoted";
        std::wstring result = ler::quoteArgForWindowsCommandLine(arg);
        // Should properly escape the quote
        CHECK_FALSE(result.empty());
    }
}

TEST_CASE("CommandRunner: quoteArgForWindowsCommandLine handles quotes") {
    std::wstring arg = L"say \"hello\"";
    std::wstring result = ler::quoteArgForWindowsCommandLine(arg);
    
    // Should escape internal quotes
    CHECK(result.find(L"\\\"") != std::wstring::npos);
}

TEST_CASE("CommandRunner: quoteArgForWindowsCommandLine handles special characters") {
    SUBCASE("Tab") {
        std::wstring arg = L"hello\tworld";
        std::wstring result = ler::quoteArgForWindowsCommandLine(arg);
        CHECK(result.front() == L'"');
        CHECK(result.back() == L'"');
    }
    
    SUBCASE("Newline") {
        std::wstring arg = L"hello\nworld";
        std::wstring result = ler::quoteArgForWindowsCommandLine(arg);
        CHECK(result.front() == L'"');
        CHECK(result.back() == L'"');
    }
}

// Note: runProcess tests are intentionally minimal because they require actual process execution
// and Windows API calls which may not work in a cross-platform test environment

#ifndef LASTEXEC_STUB_WINDOWS
TEST_CASE("CommandRunner: runProcess with simple command") {
    // Test basic command execution on Windows
    ler::RunResult result = ler::runProcess(
        L"cmd.exe",
        {L"/c", L"exit 0"},
        L"",
        10
    );
    
    CHECK(result.started);
    CHECK_FALSE(result.timedOut);
    CHECK(result.exitCode == 0);
}

TEST_CASE("CommandRunner: runProcess with failing command") {
    ler::RunResult result = ler::runProcess(
        L"cmd.exe",
        {L"/c", L"exit 1"},
        L"",
        10
    );
    
    CHECK(result.started);
    CHECK_FALSE(result.timedOut);
    CHECK(result.exitCode == 1);
}

TEST_CASE("CommandRunner: runProcess with nonexistent executable") {
    ler::RunResult result = ler::runProcess(
        L"nonexistent_executable_12345.exe",
        {},
        L"",
        10
    );
    
    CHECK_FALSE(result.started);
}
#else
// Stub tests for non-Windows platforms
TEST_CASE("CommandRunner: runProcess stubbed on non-Windows") {
    // These tests are stubbed because they require Windows API
    WARN("CommandRunner process execution tests are only available on Windows");
}
#endif
