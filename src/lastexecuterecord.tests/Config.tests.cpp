#include <gtest/gtest.h>

#include <Windows.h>
#include <string>

#include "Config.h"
#include "FileUtil.h"

namespace {

static std::wstring makeTempPath(const wchar_t* leaf) {
    wchar_t tmpDir[MAX_PATH] = {};
    DWORD n = GetTempPathW(MAX_PATH, tmpDir);
    EXPECT_GT(n, 0u);

    wchar_t nameBuf[MAX_PATH] = {};
    wsprintfW(nameBuf, L"ler_%lu_%ls", GetCurrentProcessId(), leaf);

    std::wstring path = std::wstring(tmpDir) + nameBuf;
    return path;
}

TEST(Config, Load_MinimalValidConfig_ReturnsCommands) {
    std::wstring path = makeTempPath(L"minimal.json");

    ler::writeWStringToUtf8FileAtomic(path,
        L"{\n"
        L"  \"version\": 1,\n"
        L"  \"commands\": [\n"
        L"    { \"name\": \"c1\", \"exe\": \"C:\\\\Windows\\\\System32\\\\whoami.exe\" }\n"
        L"  ]\n"
        L"}\n");

    ler::AppConfig cfg = ler::loadAndValidateConfig(path);
    ASSERT_EQ(cfg.commands.size(), 1u);
    EXPECT_EQ(cfg.commands[0].name, L"c1");
    EXPECT_EQ(cfg.commands[0].exe, L"C:\\Windows\\System32\\whoami.exe");

    DeleteFileW(path.c_str());
}

TEST(Config, Load_CommandNameMissing_Throws) {
    std::wstring path = makeTempPath(L"noname.json");

    ler::writeWStringToUtf8FileAtomic(path,
        L"{\n"
        L"  \"commands\": [ { \"exe\": \"x\" } ]\n"
        L"}\n");

    EXPECT_THROW((void)ler::loadAndValidateConfig(path), ler::JsonParseError);

    DeleteFileW(path.c_str());
}

TEST(Config, Load_CommandExeMissing_Throws) {
    std::wstring path = makeTempPath(L"noexe.json");

    ler::writeWStringToUtf8FileAtomic(path,
        L"{\n"
        L"  \"commands\": [ { \"name\": \"c1\" } ]\n"
        L"}\n");

    EXPECT_THROW((void)ler::loadAndValidateConfig(path), ler::JsonParseError);

    DeleteFileW(path.c_str());
}

TEST(Config, Load_DefaultsApplied_MinIntervalTimeout) {
    std::wstring path = makeTempPath(L"defaults.json");

    ler::writeWStringToUtf8FileAtomic(path,
        L"{\n"
        L"  \"defaults\": { \"minIntervalSeconds\": 12, \"timeoutSeconds\": 34 },\n"
        L"  \"commands\": [ { \"name\": \"c1\", \"exe\": \"x\" } ]\n"
        L"}\n");

    ler::AppConfig cfg = ler::loadAndValidateConfig(path);
    ASSERT_EQ(cfg.commands.size(), 1u);
    EXPECT_EQ(cfg.commands[0].minIntervalSeconds, 12);
    EXPECT_EQ(cfg.commands[0].timeoutSeconds, 34);

    DeleteFileW(path.c_str());
}

TEST(Config, Load_LastRunUtcEmpty_HasLastRunUtcFalse) {
    std::wstring path = makeTempPath(L"lastrunempty.json");

    ler::writeWStringToUtf8FileAtomic(path,
        L"{\n"
        L"  \"commands\": [ { \"name\": \"c1\", \"exe\": \"x\", \"lastRunUtc\": \"\" } ]\n"
        L"}\n");

    ler::AppConfig cfg = ler::loadAndValidateConfig(path);
    ASSERT_EQ(cfg.commands.size(), 1u);
    EXPECT_FALSE(cfg.commands[0].hasLastRunUtc);

    DeleteFileW(path.c_str());
}

TEST(Config, Load_LastExitCodeNull_HasLastExitCodeFalse) {
    std::wstring path = makeTempPath(L"lastExitNull.json");

    ler::writeWStringToUtf8FileAtomic(path,
        L"{\n"
        L"  \"commands\": [ { \"name\": \"c1\", \"exe\": \"x\", \"lastExitCode\": null } ]\n"
        L"}\n");

    ler::AppConfig cfg = ler::loadAndValidateConfig(path);
    ASSERT_EQ(cfg.commands.size(), 1u);
    EXPECT_FALSE(cfg.commands[0].hasLastExitCode);

    DeleteFileW(path.c_str());
}

} // namespace
