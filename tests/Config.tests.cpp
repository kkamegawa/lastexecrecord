#include <gtest/gtest.h>

#include "Config.h"
#include "FileUtil.h"

TEST(Config, DefaultConfigPath_ReturnsPathBasedOnModule) {
    std::wstring path = ler::defaultConfigPath();
    
    // Should not be empty
    EXPECT_FALSE(path.empty());
    
    // Should end with .json
    EXPECT_NE(path.find(L".json"), std::wstring::npos);
}

#ifndef LASTEXEC_STUB_WINDOWS

TEST(Config, LoadAndValidateConfig_LoadsValidConfig) {
    // Create a temporary config file
    std::wstring configPath = L"test_config.json";
    std::wstring configContent = LR"({
        "version": 1,
        "defaults": {
            "minIntervalSeconds": 60,
            "timeoutSeconds": 300
        },
        "commands": [
            {
                "name": "Test Command",
                "enabled": true,
                "exe": "cmd.exe",
                "args": ["/c", "echo test"],
                "minIntervalSeconds": 30
            }
        ]
    })";
    
    ler::writeWStringToUtf8FileAtomic(configPath, configContent);
    
    // Load config
    ler::AppConfig config = ler::loadAndValidateConfig(configPath);
    
    EXPECT_EQ(config.version, 1);
    EXPECT_EQ(config.defaultMinIntervalSeconds, 60);
    EXPECT_EQ(config.defaultTimeoutSeconds, 300);
    ASSERT_EQ(config.commands.size(), 1u);
    EXPECT_EQ(config.commands[0].name, L"Test Command");
    EXPECT_TRUE(config.commands[0].enabled);
    EXPECT_EQ(config.commands[0].exe, L"cmd.exe");
    EXPECT_EQ(config.commands[0].args.size(), 2u);
    EXPECT_EQ(config.commands[0].minIntervalSeconds, 30);
    
    // Cleanup
    DeleteFileW(configPath.c_str());
}

TEST(Config, LoadAndValidateConfig_AppliesDefaults) {
    std::wstring configPath = L"test_config_defaults.json";
    std::wstring configContent = LR"({
        "version": 1,
        "defaults": {
            "minIntervalSeconds": 100,
            "timeoutSeconds": 200
        },
        "commands": [
            {
                "name": "Test Command",
                "exe": "test.exe"
            }
        ]
    })";
    
    ler::writeWStringToUtf8FileAtomic(configPath, configContent);
    
    ler::AppConfig config = ler::loadAndValidateConfig(configPath);
    
    // Command should inherit defaults
    ASSERT_EQ(config.commands.size(), 1u);
    EXPECT_EQ(config.commands[0].minIntervalSeconds, 100);
    EXPECT_EQ(config.commands[0].timeoutSeconds, 200);
    
    // Cleanup
    DeleteFileW(configPath.c_str());
}

TEST(Config, LoadAndValidateConfig_HandlesOptionalFields) {
    std::wstring configPath = L"test_config_optional.json";
    std::wstring configContent = LR"({
        "commands": [
            {
                "name": "Minimal Command",
                "exe": "test.exe"
            }
        ]
    })";
    
    ler::writeWStringToUtf8FileAtomic(configPath, configContent);
    
    ler::AppConfig config = ler::loadAndValidateConfig(configPath);
    
    // Should have default version
    EXPECT_EQ(config.version, 1);
    ASSERT_EQ(config.commands.size(), 1u);
    EXPECT_EQ(config.commands[0].name, L"Minimal Command");
    EXPECT_TRUE(config.commands[0].enabled);  // default
    
    // Cleanup
    DeleteFileW(configPath.c_str());
}

TEST(Config, LoadAndValidateConfig_ThrowsWhenMissingCommandsArray) {
    std::wstring configPath = L"test_config_invalid_missing_commands.json";
    std::wstring configContent = LR"({"version": 1})";
    ler::writeWStringToUtf8FileAtomic(configPath, configContent);
    EXPECT_THROW((void)ler::loadAndValidateConfig(configPath), std::exception);
    DeleteFileW(configPath.c_str());
}

TEST(Config, LoadAndValidateConfig_ThrowsWhenCommandMissingName) {
    std::wstring configPath = L"test_config_invalid_missing_name.json";
    std::wstring configContent = LR"({
            "commands": [{"exe": "test.exe"}]
        })";
    ler::writeWStringToUtf8FileAtomic(configPath, configContent);
    EXPECT_THROW((void)ler::loadAndValidateConfig(configPath), std::exception);
    DeleteFileW(configPath.c_str());
}

TEST(Config, LoadAndValidateConfig_ThrowsWhenCommandMissingExe) {
    std::wstring configPath = L"test_config_invalid_missing_exe.json";
    std::wstring configContent = LR"({
            "commands": [{"name": "Test"}]
        })";
    ler::writeWStringToUtf8FileAtomic(configPath, configContent);
    EXPECT_THROW((void)ler::loadAndValidateConfig(configPath), std::exception);
    DeleteFileW(configPath.c_str());
}

TEST(Config, LoadAndValidateConfig_PreservesExecutionHistory) {
    std::wstring configPath = L"test_config_history.json";
    std::wstring configContent = LR"({
        "commands": [
            {
                "name": "Test Command",
                "exe": "test.exe",
                "lastRunUtc": "2026-01-02T12:34:56Z",
                "lastExitCode": 0
            }
        ]
    })";
    
    ler::writeWStringToUtf8FileAtomic(configPath, configContent);
    
    ler::AppConfig config = ler::loadAndValidateConfig(configPath);
    
    ASSERT_EQ(config.commands.size(), 1u);
    EXPECT_TRUE(config.commands[0].hasLastRunUtc);
    EXPECT_EQ(config.commands[0].lastRunUtc, L"2026-01-02T12:34:56Z");
    EXPECT_TRUE(config.commands[0].hasLastExitCode);
    EXPECT_EQ(config.commands[0].lastExitCode, 0);
    
    // Cleanup
    DeleteFileW(configPath.c_str());
}

TEST(Config, ApplyCommandsToJson_UpdatesJson) {
    std::wstring configPath = L"test_config_update.json";
    std::wstring configContent = LR"({
        "commands": [
            {
                "name": "Test Command",
                "exe": "test.exe"
            }
        ]
    })";
    
    ler::writeWStringToUtf8FileAtomic(configPath, configContent);
    
    ler::AppConfig config = ler::loadAndValidateConfig(configPath);
    
    // Modify command
    config.commands[0].hasLastRunUtc = true;
    config.commands[0].lastRunUtc = L"2026-01-02T12:34:56Z";
    config.commands[0].hasLastExitCode = true;
    config.commands[0].lastExitCode = 0;
    config.dirty = true;
    
    // Apply changes to JSON
    ler::applyCommandsToJson(config);
    
    // Check that JSON was updated
    auto* commands = config.root.tryGet(L"commands");
    ASSERT_NE(commands, nullptr);
    ASSERT_TRUE(commands->isArray());
    ASSERT_GT(commands->a.size(), 0u);
    
    auto* lastRunUtc = commands->a[0].tryGet(L"lastRunUtc");
    ASSERT_NE(lastRunUtc, nullptr);
    EXPECT_EQ(lastRunUtc->s, L"2026-01-02T12:34:56Z");
    
    auto* lastExitCode = commands->a[0].tryGet(L"lastExitCode");
    ASSERT_NE(lastExitCode, nullptr);
    EXPECT_EQ(lastExitCode->i, 0);
    
    // Cleanup
    DeleteFileW(configPath.c_str());
}

#else
// Stub tests for non-Windows platforms
TEST(Config, StubbedOnNonWindows) {
    GTEST_SKIP() << "Config tests are only available on Windows";
    
    // Still test the defaultConfigPath as it should work cross-platform
    std::wstring path = ler::defaultConfigPath();
    EXPECT_FALSE(path.empty());
}
#endif
