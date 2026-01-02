#include "Json.h"

#include <cwctype>
#include <limits>
#include <sstream>

namespace ler {

static std::string narrowContext(const wchar_t* wctx) {
    if (!wctx) return "";
    std::wstring ws(wctx);
    return std::string(ws.begin(), ws.end());
}

const JsonValue* JsonValue::tryGet(const std::wstring& key) const {
    if (type != Type::Object) return nullptr;
    for (const auto& kv : o) {
        if (kv.first == key) return &kv.second;
    }
    return nullptr;
}

JsonValue* JsonValue::tryGet(const std::wstring& key) {
    if (type != Type::Object) return nullptr;
    for (auto& kv : o) {
        if (kv.first == key) return &kv.second;
    }
    return nullptr;
}

const std::wstring& JsonValue::asString(const wchar_t* ctx) const {
    if (type != Type::String) {
        throw JsonParseError("Expected string at " + narrowContext(ctx));
    }
    return s;
}

std::int64_t JsonValue::asInt(const wchar_t* ctx) const {
    if (type == Type::Int) return i;
    if (type == Type::Double) {
        if (d < static_cast<double>(std::numeric_limits<std::int64_t>::min()) ||
            d > static_cast<double>(std::numeric_limits<std::int64_t>::max())) {
            throw JsonParseError("Number out of int64 range at " + narrowContext(ctx));
        }
        return static_cast<std::int64_t>(d);
    }
    throw JsonParseError("Expected number at " + narrowContext(ctx));
}

bool JsonValue::asBool(const wchar_t* ctx) const {
    if (type != Type::Bool) {
        throw JsonParseError("Expected bool at " + narrowContext(ctx));
    }
    return b;
}

struct Parser {
    const std::wstring& t;
    size_t p = 0;

    explicit Parser(const std::wstring& text) : t(text) {}

    void skipWs() {
        while (p < t.size() && iswspace(t[p])) p++;
    }

    wchar_t peek() {
        return p < t.size() ? t[p] : L'\0';
    }

    bool consume(wchar_t c) {
        skipWs();
        if (peek() == c) { p++; return true; }
        return false;
    }

    void expect(wchar_t c, const char* msg) {
        skipWs();
        if (peek() != c) throw JsonParseError(msg);
        p++;
    }

    bool matchLiteral(const wchar_t* lit) {
        skipWs();
        size_t start = p;
        for (size_t i = 0; lit[i] != 0; i++) {
            if (p + i >= t.size() || t[p + i] != lit[i]) {
                p = start;
                return false;
            }
        }
        p += wcslen(lit);
        return true;
    }

    static int hexVal(wchar_t c) {
        if (c >= L'0' && c <= L'9') return c - L'0';
        if (c >= L'a' && c <= L'f') return 10 + (c - L'a');
        if (c >= L'A' && c <= L'F') return 10 + (c - L'A');
        return -1;
    }

    static bool isHighSurrogate(uint16_t u) { return u >= 0xD800 && u <= 0xDBFF; }
    static bool isLowSurrogate(uint16_t u) { return u >= 0xDC00 && u <= 0xDFFF; }

    static void appendCodepoint(std::wstring& out, uint32_t cp) {
        if (cp <= 0xFFFF) {
            out.push_back(static_cast<wchar_t>(cp));
            return;
        }
        cp -= 0x10000;
        wchar_t high = static_cast<wchar_t>(0xD800 + (cp >> 10));
        wchar_t low = static_cast<wchar_t>(0xDC00 + (cp & 0x3FF));
        out.push_back(high);
        out.push_back(low);
    }

    std::wstring parseString() {
        expect(L'\"', "Expected string");
        std::wstring out;
        while (p < t.size()) {
            wchar_t c = t[p++];
            if (c == L'\"') break;
            if (c == L'\\') {
                if (p >= t.size()) throw JsonParseError("Invalid escape");
                wchar_t e = t[p++];
                switch (e) {
                case L'\"': out.push_back(L'\"'); break;
                case L'\\': out.push_back(L'\\'); break;
                case L'/': out.push_back(L'/'); break;
                case L'b': out.push_back(L'\b'); break;
                case L'f': out.push_back(L'\f'); break;
                case L'n': out.push_back(L'\n'); break;
                case L'r': out.push_back(L'\r'); break;
                case L't': out.push_back(L'\t'); break;
                case L'u': {
                    if (p + 4 > t.size()) throw JsonParseError("Invalid unicode escape");
                    uint16_t u = 0;
                    for (int k = 0; k < 4; k++) {
                        int hv = hexVal(t[p++]);
                        if (hv < 0) throw JsonParseError("Invalid unicode escape");
                        u = static_cast<uint16_t>((u << 4) | hv);
                    }
                    if (isHighSurrogate(u)) {
                        // try surrogate pair
                        size_t save = p;
                        if (p + 6 <= t.size() && t[p] == L'\\' && t[p + 1] == L'u') {
                            p += 2;
                            uint16_t u2 = 0;
                            for (int k = 0; k < 4; k++) {
                                int hv = hexVal(t[p++]);
                                if (hv < 0) throw JsonParseError("Invalid unicode escape");
                                u2 = static_cast<uint16_t>((u2 << 4) | hv);
                            }
                            if (isLowSurrogate(u2)) {
                                uint32_t cp = 0x10000 + (((u - 0xD800) << 10) | (u2 - 0xDC00));
                                appendCodepoint(out, cp);
                                break;
                            }
                        }
                        p = save;
                        out.push_back(static_cast<wchar_t>(u));
                    }
                    else {
                        out.push_back(static_cast<wchar_t>(u));
                    }
                    break;
                }
                default:
                    throw JsonParseError("Invalid escape");
                }
            }
            else {
                out.push_back(c);
            }
        }
        return out;
    }

    JsonValue parseNumber() {
        skipWs();
        size_t start = p;
        if (peek() == L'-') p++;
        if (peek() == L'0') {
            p++;
        }
        else {
            if (!iswdigit(peek())) throw JsonParseError("Invalid number");
            while (iswdigit(peek())) p++;
        }
        bool isFloat = false;
        if (peek() == L'.') {
            isFloat = true;
            p++;
            if (!iswdigit(peek())) throw JsonParseError("Invalid number");
            while (iswdigit(peek())) p++;
        }
        if (peek() == L'e' || peek() == L'E') {
            isFloat = true;
            p++;
            if (peek() == L'+' || peek() == L'-') p++;
            if (!iswdigit(peek())) throw JsonParseError("Invalid number");
            while (iswdigit(peek())) p++;
        }

        std::wstring num = t.substr(start, p - start);
        if (!isFloat) {
            // parse int64
            try {
                size_t idx = 0;
                long long v = std::stoll(num, &idx, 10);
                if (idx != num.size()) throw JsonParseError("Invalid number");
                return JsonValue::makeInt(static_cast<std::int64_t>(v));
            }
            catch (...) {
                // fallback to double
            }
        }

        try {
            size_t idx = 0;
            double dv = std::stod(num, &idx);
            if (idx != num.size()) throw JsonParseError("Invalid number");
            return JsonValue::makeDouble(dv);
        }
        catch (...) {
            throw JsonParseError("Invalid number");
        }
    }

    JsonValue parseArray() {
        expect(L'[', "Expected [");
        std::vector<JsonValue> arr;
        skipWs();
        if (consume(L']')) return JsonValue::makeArray(std::move(arr));
        while (true) {
            arr.push_back(parseValue());
            skipWs();
            if (consume(L',')) continue;
            expect(L']', "Expected ]");
            break;
        }
        return JsonValue::makeArray(std::move(arr));
    }

    JsonValue parseObject() {
        expect(L'{', "Expected {");
        std::vector<std::pair<std::wstring, JsonValue>> obj;
        skipWs();
        if (consume(L'}')) return JsonValue::makeObject(std::move(obj));
        while (true) {
            skipWs();
            std::wstring key = parseString();
            skipWs();
            expect(L':', "Expected :");
            JsonValue value = parseValue();
            obj.push_back(std::make_pair(std::move(key), std::move(value)));
            skipWs();
            if (consume(L',')) continue;
            expect(L'}', "Expected }");
            break;
        }
        return JsonValue::makeObject(std::move(obj));
    }

    JsonValue parseValue() {
        skipWs();
        wchar_t c = peek();
        if (c == L'\"') return JsonValue::makeString(parseString());
        if (c == L'{') return parseObject();
        if (c == L'[') return parseArray();
        if (c == L'-' || iswdigit(c)) return parseNumber();
        if (matchLiteral(L"true")) return JsonValue::makeBool(true);
        if (matchLiteral(L"false")) return JsonValue::makeBool(false);
        if (matchLiteral(L"null")) return JsonValue::makeNull();
        throw JsonParseError("Unexpected token");
    }
};

JsonValue parseJson(const std::wstring& text) {
    Parser p(text);
    JsonValue v = p.parseValue();
    p.skipWs();
    if (p.p != text.size()) throw JsonParseError("Trailing characters");
    return v;
}

static void writeEscapedString(std::wstringstream& ss, const std::wstring& s) {
    ss << L'\"';
    for (wchar_t c : s) {
        switch (c) {
        case L'\"': ss << L"\\\""; break;
        case L'\\': ss << L"\\\\"; break;
        case L'\b': ss << L"\\b"; break;
        case L'\f': ss << L"\\f"; break;
        case L'\n': ss << L"\\n"; break;
        case L'\r': ss << L"\\r"; break;
        case L'\t': ss << L"\\t"; break;
        default:
            if (c < 0x20) {
                ss << L"\\u";
                ss << std::hex;
                ss.width(4);
                ss.fill(L'0');
                ss << static_cast<int>(c);
                ss << std::dec;
            }
            else {
                ss << c;
            }
            break;
        }
    }
    ss << L'\"';
}

static void writeValue(std::wstringstream& ss, const JsonValue& v, int indentSpaces, int depth) {
    auto indent = [&]() {
        for (int i = 0; i < depth * indentSpaces; i++) ss << L' ';
    };

    switch (v.type) {
    case JsonValue::Type::Null: ss << L"null"; break;
    case JsonValue::Type::Bool: ss << (v.b ? L"true" : L"false"); break;
    case JsonValue::Type::Int: ss << v.i; break;
    case JsonValue::Type::Double: {
        ss << v.d;
        break;
    }
    case JsonValue::Type::String:
        writeEscapedString(ss, v.s);
        break;
    case JsonValue::Type::Array: {
        ss << L'[';
        if (!v.a.empty()) {
            ss << L'\n';
            for (size_t idx = 0; idx < v.a.size(); idx++) {
                for (int i = 0; i < (depth + 1) * indentSpaces; i++) ss << L' ';
                writeValue(ss, v.a[idx], indentSpaces, depth + 1);
                if (idx + 1 < v.a.size()) ss << L',';
                ss << L'\n';
            }
            indent();
        }
        ss << L']';
        break;
    }
    case JsonValue::Type::Object: {
        ss << L'{';
        if (!v.o.empty()) {
            ss << L'\n';
            for (size_t idx = 0; idx < v.o.size(); idx++) {
                for (int i = 0; i < (depth + 1) * indentSpaces; i++) ss << L' ';
                writeEscapedString(ss, v.o[idx].first);
                ss << L": ";
                writeValue(ss, v.o[idx].second, indentSpaces, depth + 1);
                if (idx + 1 < v.o.size()) ss << L',';
                ss << L'\n';
            }
            indent();
        }
        ss << L'}';
        break;
    }
    }
}

std::wstring writeJson(const JsonValue& v, int indentSpaces) {
    std::wstringstream ss;
    writeValue(ss, v, indentSpaces, 0);
    ss << L'\n';
    return ss.str();
}

} // namespace ler
