#include "Config.h"

#include "FileUtil.h"

#include <algorithm>
#include <cwctype>
#include <stdexcept>

namespace ler {

static std::string narrowForError(const std::wstring& value) {
    std::string out;
    out.reserve(value.size());
    for (wchar_t c : value) {
        out.push_back(c >= 0 && c <= 0x7F ? static_cast<char>(c) : '?');
    }
    return out;
}

static const JsonValue& requireObjectField(const JsonValue& obj, const std::wstring& key, const wchar_t* ctx) {
    const JsonValue* v = obj.tryGet(key);
    if (!v) throw JsonParseError("Missing field: " + narrowForError(key) + " at " + narrowForError(ctx ? std::wstring(ctx) : std::wstring()));
    return *v;
}

static std::wstring getStringFieldOrEmpty(const JsonValue& obj, const std::wstring& key) {
    const JsonValue* v = obj.tryGet(key);
    if (!v) return L"";
    if (v->isNull()) return L"";
    return v->asString(key.c_str());
}

static bool getBoolFieldOrDefault(const JsonValue& obj, const std::wstring& key, bool def) {
    const JsonValue* v = obj.tryGet(key);
    if (!v) return def;
    if (v->isNull()) return def;
    return v->asBool(key.c_str());
}

static std::int64_t getIntFieldOrDefault(const JsonValue& obj, const std::wstring& key, std::int64_t def) {
    const JsonValue* v = obj.tryGet(key);
    if (!v) return def;
    if (v->isNull()) return def;
    return v->asInt(key.c_str());
}

static InstallerWaitBehavior getInstallerWaitBehaviorOrDefault(const JsonValue& obj, const std::wstring& key, InstallerWaitBehavior def) {
    const JsonValue* v = obj.tryGet(key);
    if (!v) return def;
    if (v->isNull()) return def;
    if (v->isString()) {
        std::wstring s = v->asString(key.c_str());
        std::wstring lower = s;
        std::transform(lower.begin(), lower.end(), lower.begin(), [](wchar_t c) { return std::towlower(c); });
        if (lower == L"wait") return InstallerWaitBehavior::Wait;
        if (lower == L"skip") return InstallerWaitBehavior::Skip;
        throw JsonParseError("installerWaitBehavior must be 'wait' or 'skip'");
    }
    if (v->isInt()) {
        std::int64_t val = v->asInt(key.c_str());
        if (val == 0) return InstallerWaitBehavior::Wait;
        if (val == 1) return InstallerWaitBehavior::Skip;
        throw JsonParseError("installerWaitBehavior must be 0 (wait) or 1 (skip)");
    }
    // Invalid type - throw error instead of silently returning default
    std::string msg = "Invalid type for field ";
    msg += narrowForError(key);
    msg += " in installerWaitBehavior; expected string or integer";
    throw JsonParseError(msg);
}

static void upsertObjectField(JsonValue& obj, const std::wstring& key, JsonValue value) {
    if (!obj.isObject()) throw std::runtime_error("not an object");
    for (auto& kv : obj.o) {
        if (kv.first == key) {
            kv.second = std::move(value);
            return;
        }
    }
    obj.o.push_back(std::make_pair(key, std::move(value)));
}

static void validateExecutablePath(CommandConfig& command) {
    if (!pathIsAbsolute(command.exe)) {
        throw JsonParseError("command.exe must be an absolute path: " + narrowForError(command.name));
    }

    command.exe = getFullPath(command.exe);
    if (!fileExists(command.exe)) {
        throw JsonParseError("command.exe must refer to an existing file: " + narrowForError(command.name));
    }
}

static void validateWorkingDirectory(CommandConfig& command) {
    if (command.workingDirectory.empty()) return;

    if (!pathIsAbsolute(command.workingDirectory)) {
        throw JsonParseError("command.workingDirectory must be an absolute path: " + narrowForError(command.name));
    }

    command.workingDirectory = getFullPath(command.workingDirectory);
    if (!directoryExists(command.workingDirectory)) {
        throw JsonParseError("command.workingDirectory must refer to an existing directory: " + narrowForError(command.name));
    }
}

static std::wstring sampleConfigText() {
    // Minimal and safe: default command is disabled.
    std::wstring s;
    s += L"{\n";
    s += L"  \"version\": 1,\n";
    s += L"  \"networkOption\": 2,\n";  // 2 = AlwaysExecute (default)
    s += L"  \"defaults\": {\n";
    s += L"    \"minIntervalSeconds\": 0,\n";
    s += L"    \"timeoutSeconds\": 0,\n";
    s += L"    \"installerWaitBehavior\": \"wait\",\n";
    s += L"    \"installerWaitSeconds\": 30,\n";
    s += L"    \"installerMaxRetries\": 10\n";
    s += L"  },\n";
    s += L"  \"commands\": [\n";
    s += L"    {\n";
    s += L"      \"name\": \"example (disabled)\",\n";
    s += L"      \"enabled\": false,\n";
    s += L"      \"exe\": \"C:\\\\Windows\\\\System32\\\\whoami.exe\",\n";
    s += L"      \"args\": []\n";
    s += L"    }\n";
    s += L"  ]\n";
    s += L"}\n";
    return s;
}

void ensureSampleConfigExists(const std::wstring& configPath) {
    if (fileExists(configPath)) return;

    std::wstring dir = getDirectoryName(configPath);
    if (dir.empty()) {
        throw std::runtime_error("Config path has no directory component");
    }

    ensureDirectoryExists(dir);

    // Re-check after creating directory to avoid overwriting if another process wrote it.
    if (fileExists(configPath)) return;

    // Atomic write will replace, so only do it if still missing.
    writeWStringToUtf8FileAtomic(configPath, sampleConfigText());
}

std::wstring defaultConfigPath() {
    std::wstring profile = getEnvVar(L"USERPROFILE");
    if (!profile.empty()) {
        std::wstring dir = joinPath(profile, L".lastexecrecord");
        return joinPath(dir, L"config.json");
    }

    // fallback (legacy behavior)
    return changeExtension(getModulePath(), L".json");
}

AppConfig loadAndValidateConfig(const std::wstring& configPath) {
    AppConfig cfg;

    std::wstring text = readUtf8FileToWString(configPath);
    cfg.root = parseJson(text);

    if (!cfg.root.isObject()) throw JsonParseError("Config root must be object");

    cfg.version = getIntFieldOrDefault(cfg.root, L"version", 1);

    // defaults
    const JsonValue* defaults = cfg.root.tryGet(L"defaults");
    if (defaults && defaults->isObject()) {
        cfg.defaultMinIntervalSeconds = getIntFieldOrDefault(*defaults, L"minIntervalSeconds", 0);
        cfg.defaultTimeoutSeconds = getIntFieldOrDefault(*defaults, L"timeoutSeconds", 0);
        cfg.defaultInstallerWaitBehavior = getInstallerWaitBehaviorOrDefault(*defaults, L"installerWaitBehavior", InstallerWaitBehavior::Wait);
        cfg.defaultInstallerWaitSeconds = getIntFieldOrDefault(*defaults, L"installerWaitSeconds", 30);
        cfg.defaultInstallerMaxRetries = getIntFieldOrDefault(*defaults, L"installerMaxRetries", 10);
        
        // Validate default values
        if (cfg.defaultInstallerWaitSeconds < 0) {
            throw JsonParseError("defaults.installerWaitSeconds must be >= 0");
        }
        if (cfg.defaultInstallerMaxRetries < 0) {
            throw JsonParseError("defaults.installerMaxRetries must be >= 0");
        }
    }

    // networkOption: 0=connected only, 1=metered ok, 2=always (default: 2)
    // Check root level first, then fall back to defaults.networkOption
    const JsonValue* rootNetOpt = cfg.root.tryGet(L"networkOption");
    const JsonValue* defaultsNetOpt = (defaults && defaults->isObject()) ? defaults->tryGet(L"networkOption") : nullptr;
    std::int64_t netOpt = 2; // AlwaysExecute
    if (rootNetOpt && !rootNetOpt->isNull()) {
        netOpt = rootNetOpt->asInt(L"networkOption");
    }
    else if (defaultsNetOpt && !defaultsNetOpt->isNull()) {
        netOpt = defaultsNetOpt->asInt(L"defaults.networkOption");
    }
    if (netOpt < 0 || netOpt > 2) {
        throw JsonParseError("networkOption must be 0, 1, or 2");
    }
    cfg.networkOption = static_cast<NetworkOption>(netOpt);

    const JsonValue& cmdsV = requireObjectField(cfg.root, L"commands", L"root");
    if (!cmdsV.isArray()) throw JsonParseError("commands must be array");

    cfg.commands.clear();
    cfg.commands.reserve(cmdsV.a.size());

    for (size_t idx = 0; idx < cmdsV.a.size(); idx++) {
        const JsonValue& c = cmdsV.a[idx];
        if (!c.isObject()) throw JsonParseError("command entry must be object");

        CommandConfig cc;
        cc.name = getStringFieldOrEmpty(c, L"name");
        if (cc.name.empty()) {
            // allow id as alias
            cc.name = getStringFieldOrEmpty(c, L"id");
        }
        if (cc.name.empty()) {
            throw JsonParseError("command.name (or id) is required");
        }

        cc.enabled = getBoolFieldOrDefault(c, L"enabled", true);

        cc.exe = getStringFieldOrEmpty(c, L"exe");
        if (cc.exe.empty()) {
            throw JsonParseError("command.exe is required");
        }
        validateExecutablePath(cc);

        // args
        const JsonValue* argsV = c.tryGet(L"args");
        if (argsV) {
            if (!argsV->isArray()) throw JsonParseError("command.args must be array");
            for (const auto& av : argsV->a) {
                cc.args.push_back(av.asString(L"command.args[]"));
            }
        }

        cc.workingDirectory = getStringFieldOrEmpty(c, L"workingDirectory");
        validateWorkingDirectory(cc);

        cc.minIntervalSeconds = getIntFieldOrDefault(c, L"minIntervalSeconds", cfg.defaultMinIntervalSeconds);
        if (cc.minIntervalSeconds < 0) throw JsonParseError("minIntervalSeconds must be >= 0");

        cc.timeoutSeconds = getIntFieldOrDefault(c, L"timeoutSeconds", cfg.defaultTimeoutSeconds);
        if (cc.timeoutSeconds < 0) throw JsonParseError("timeoutSeconds must be >= 0");

        cc.installerWaitBehavior = getInstallerWaitBehaviorOrDefault(c, L"installerWaitBehavior", cfg.defaultInstallerWaitBehavior);
        cc.installerWaitSeconds = getIntFieldOrDefault(c, L"installerWaitSeconds", cfg.defaultInstallerWaitSeconds);
        if (cc.installerWaitSeconds < 0) throw JsonParseError("installerWaitSeconds must be >= 0");
        cc.installerMaxRetries = getIntFieldOrDefault(c, L"installerMaxRetries", cfg.defaultInstallerMaxRetries);
        if (cc.installerMaxRetries < 0) throw JsonParseError("installerMaxRetries must be >= 0");

        cc.lastRunUtc = getStringFieldOrEmpty(c, L"lastRunUtc");
        cc.hasLastRunUtc = !cc.lastRunUtc.empty();

        const JsonValue* lec = c.tryGet(L"lastExitCode");
        if (lec && !lec->isNull()) {
            cc.hasLastExitCode = true;
            cc.lastExitCode = lec->asInt(L"lastExitCode");
        }

        cfg.commands.push_back(std::move(cc));
    }

    return cfg;
}

void applyCommandsToJson(AppConfig& cfg) {
    if (!cfg.root.isObject()) return;
    JsonValue* cmds = cfg.root.tryGet(L"commands");
    if (!cmds || !cmds->isArray()) return;

    for (size_t idx = 0; idx < cfg.commands.size() && idx < cmds->a.size(); idx++) {
        JsonValue& c = cmds->a[idx];
        if (!c.isObject()) continue;
        const CommandConfig& cc = cfg.commands[idx];

        if (cc.hasLastRunUtc) {
            upsertObjectField(c, L"lastRunUtc", JsonValue::makeString(cc.lastRunUtc));
        }
        if (cc.hasLastExitCode) {
            upsertObjectField(c, L"lastExitCode", JsonValue::makeInt(cc.lastExitCode));
        }
    }
}

} // namespace ler
