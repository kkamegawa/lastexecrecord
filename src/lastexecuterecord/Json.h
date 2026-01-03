#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace ler {

struct JsonValue {
    enum class Type {
        Null,
        Bool,
        Int,
        Double,
        String,
        Array,
        Object,
    };

    Type type = Type::Null;

    bool b = false;
    std::int64_t i = 0;
    double d = 0.0;
    std::wstring s;
    std::vector<JsonValue> a;
    // preserve insertion order
    std::vector<std::pair<std::wstring, JsonValue>> o;

    JsonValue() = default;

    static JsonValue makeNull() { return JsonValue(); }
    static JsonValue makeBool(bool v) {
        JsonValue x; x.type = Type::Bool; x.b = v; return x;
    }
    static JsonValue makeInt(std::int64_t v) {
        JsonValue x; x.type = Type::Int; x.i = v; return x;
    }
    static JsonValue makeDouble(double v) {
        JsonValue x; x.type = Type::Double; x.d = v; return x;
    }
    static JsonValue makeString(std::wstring v) {
        JsonValue x; x.type = Type::String; x.s = std::move(v); return x;
    }
    static JsonValue makeArray(std::vector<JsonValue> v) {
        JsonValue x; x.type = Type::Array; x.a = std::move(v); return x;
    }
    static JsonValue makeObject(std::vector<std::pair<std::wstring, JsonValue>> v) {
        JsonValue x; x.type = Type::Object; x.o = std::move(v); return x;
    }

    bool isNull() const { return type == Type::Null; }
    bool isBool() const { return type == Type::Bool; }
    bool isInt() const { return type == Type::Int; }
    bool isDouble() const { return type == Type::Double; }
    bool isNumber() const { return isInt() || isDouble(); }
    bool isString() const { return type == Type::String; }
    bool isArray() const { return type == Type::Array; }
    bool isObject() const { return type == Type::Object; }

    // in-place builders for convenience
    void makeArray() {
        type = Type::Array;
        a.clear();
    }

    void makeObject() {
        type = Type::Object;
        o.clear();
    }

    void addItem(const JsonValue& item) {
        if (type != Type::Array)
            throw std::runtime_error("JsonValue is not an array");
        a.push_back(item);
    }

    const JsonValue* tryGet(const std::wstring& key) const;
    JsonValue* tryGet(const std::wstring& key);

    // convenience getters with validation
    const std::wstring& asString(const wchar_t* ctx) const;
    std::int64_t asInt(const wchar_t* ctx) const;
    bool asBool(const wchar_t* ctx) const;
};

struct JsonParseError : public std::runtime_error {
    explicit JsonParseError(const std::string& msg) : std::runtime_error(msg) {}
};

JsonValue parseJson(const std::wstring& text);
std::wstring writeJson(const JsonValue& v, int indentSpaces = 2);

} // namespace ler
