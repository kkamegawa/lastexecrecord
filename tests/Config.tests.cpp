#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "Config.h"
#include "FileUtil.h"

TEST_CASE("Config: defaultConfigPath returns path based on module") {
    std::wstring path = ler::defaultConfigPath();
    
    // Should not be empty
    CHECK_FALSE(path.empty());
    
    // Should end with .json
    CHECK(path.find(L".json") != std::wstring::npos);
}

#ifndef LASTEXEC_STUB_WINDOWS

TEST_CASE("Config: loadAndValidateConfig loads valid config") {
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
    
    CHECK(config.version == 1);
    CHECK(config.defaultMinIntervalSeconds == 60);
    CHECK(config.defaultTimeoutSeconds == 300);
    CHECK(config.commands.size() == 1);
    CHECK(config.commands[0].name == L"Test Command");
    CHECK(config.commands[0].enabled == true);
    CHECK(config.commands[0].exe == L"cmd.exe");
    CHECK(config.commands[0].args.size() == 2);
    CHECK(config.commands[0].minIntervalSeconds == 30);
    
    // Cleanup
    DeleteFileW(configPath.c_str());
}

TEST_CASE("Config: loadAndValidateConfig applies defaults") {
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
    CHECK(config.commands[0].minIntervalSeconds == 100);
    CHECK(config.commands[0].timeoutSeconds == 200);
    
    // Cleanup
    DeleteFileW(configPath.c_str());
}

TEST_CASE("Config: loadAndValidateConfig handles optional fields") {
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
    CHECK(config.version == 1);
    CHECK(config.commands.size() == 1);
    CHECK(config.commands[0].name == L"Minimal Command");
    CHECK(config.commands[0].enabled == true);  // default
    
    // Cleanup
    DeleteFileW(configPath.c_str());
}

TEST_CASE("Config: loadAndValidateConfig throws on missing required fields") {
    std::wstring configPath = L"test_config_invalid.json";
    
    SUBCASE("Missing commands array") {
        std::wstring configContent = LR"({"version": 1})";
        ler::writeWStringToUtf8FileAtomic(configPath, configContent);
        CHECK_THROWS(ler::loadAndValidateConfig(configPath));
        DeleteFileW(configPath.c_str());
    }
    
    SUBCASE("Command missing name") {
        std::wstring configContent = LR"({
            "commands": [{"exe": "test.exe"}]
        })";
        ler::writeWStringToUtf8FileAtomic(configPath, configContent);
        CHECK_THROWS(ler::loadAndValidateConfig(configPath));
        DeleteFileW(configPath.c_str());
    }
    
    SUBCASE("Command missing exe") {
        std::wstring configContent = LR"({
            "commands": [{"name": "Test"}]
        })";
        ler::writeWStringToUtf8FileAtomic(configPath, configContent);
        CHECK_THROWS(ler::loadAndValidateConfig(configPath));
        DeleteFileW(configPath.c_str());
    }
}

TEST_CASE("Config: loadAndValidateConfig preserves execution history") {
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
    
    CHECK(config.commands[0].hasLastRunUtc);
    CHECK(config.commands[0].lastRunUtc == L"2026-01-02T12:34:56Z");
    CHECK(config.commands[0].hasLastExitCode);
    CHECK(config.commands[0].lastExitCode == 0);
    
    // Cleanup
    DeleteFileW(configPath.c_str());
}

TEST_CASE("Config: applyCommandsToJson updates JSON") {
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
    REQUIRE(commands != nullptr);
    REQUIRE(commands->isArray());
    REQUIRE(commands->a.size() > 0);
    
    auto* lastRunUtc = commands->a[0].tryGet(L"lastRunUtc");
    REQUIRE(lastRunUtc != nullptr);
    CHECK(lastRunUtc->s == L"2026-01-02T12:34:56Z");
    
    auto* lastExitCode = commands->a[0].tryGet(L"lastExitCode");
    REQUIRE(lastExitCode != nullptr);
    CHECK(lastExitCode->i == 0);
    
    // Cleanup
    DeleteFileW(configPath.c_str());
}

#else
// Stub tests for non-Windows platforms
TEST_CASE("Config: Config tests stubbed on non-Windows") {
    WARN("Config tests are only available on Windows");
    
    // Still test the defaultConfigPath as it should work cross-platform
    std::wstring path = ler::defaultConfigPath();
    CHECK_FALSE(path.empty());
}
#endif
