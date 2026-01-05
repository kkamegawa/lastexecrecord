#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "Json.h"

namespace ler {

struct CommandConfig {
    std::wstring name;
    bool enabled = true;

    std::wstring exe;
    std::vector<std::wstring> args;
    std::wstring workingDirectory;

    std::int64_t minIntervalSeconds = 0;
    std::int64_t timeoutSeconds = 0;

    // persisted fields in config
    bool hasLastRunUtc = false;
    std::wstring lastRunUtc;
    bool hasLastExitCode = false;
    std::int64_t lastExitCode = 0;
};

struct AppConfig {
    std::int64_t version = 1;

    std::int64_t defaultMinIntervalSeconds = 0;
    std::int64_t defaultTimeoutSeconds = 0;

    std::vector<CommandConfig> commands;

    // original JSON for rewrite (with modifications)
    JsonValue root;
    bool dirty = false;
};

AppConfig loadAndValidateConfig(const std::wstring& configPath);
std::wstring defaultConfigPath();

// Creates a minimal, safe sample configuration file if missing.
// - Does not overwrite existing files.
// - Creates parent directory as needed.
void ensureSampleConfigExists(const std::wstring& configPath);

// Update root JSON based on commands[].lastRunUtc/lastExitCode changes
void applyCommandsToJson(AppConfig& cfg);

} // namespace ler
