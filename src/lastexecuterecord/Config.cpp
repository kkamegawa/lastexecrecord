#include "Config.h"

#include "FileUtil.h"

#include <algorithm>
#include <cwctype>
#include <stdexcept>

namespace ler {

static const JsonValue& requireObjectField(const JsonValue& obj, const std::wstring& key, const wchar_t* ctx) {
    const JsonValue* v = obj.tryGet(key);
    if (!v) throw JsonParseError(std::string("Missing field: ") + std::string(key.begin(), key.end()) + " at " + std::string(std::wstring(ctx).begin(), std::wstring(ctx).end()));
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

std::wstring defaultConfigPath() {
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
    }

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

        // args
        const JsonValue* argsV = c.tryGet(L"args");
        if (argsV) {
            if (!argsV->isArray()) throw JsonParseError("command.args must be array");
            for (const auto& av : argsV->a) {
                cc.args.push_back(av.asString(L"command.args[]"));
            }
        }

        cc.workingDirectory = getStringFieldOrEmpty(c, L"workingDirectory");

        cc.minIntervalSeconds = getIntFieldOrDefault(c, L"minIntervalSeconds", cfg.defaultMinIntervalSeconds);
        if (cc.minIntervalSeconds < 0) throw JsonParseError("minIntervalSeconds must be >= 0");

        cc.timeoutSeconds = getIntFieldOrDefault(c, L"timeoutSeconds", cfg.defaultTimeoutSeconds);
        if (cc.timeoutSeconds < 0) throw JsonParseError("timeoutSeconds must be >= 0");

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
