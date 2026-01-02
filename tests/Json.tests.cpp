#include <doctest/doctest.h>

#include "Json.h"

TEST_CASE("Json: Parse null") {
    auto value = ler::parseJson(L"null");
    CHECK(value.isNull());
}

TEST_CASE("Json: Parse boolean values") {
    SUBCASE("true") {
        auto value = ler::parseJson(L"true");
        CHECK(value.isBool());
        CHECK(value.b == true);
    }
    
    SUBCASE("false") {
        auto value = ler::parseJson(L"false");
        CHECK(value.isBool());
        CHECK(value.b == false);
    }
}

TEST_CASE("Json: Parse integers") {
    SUBCASE("Positive") {
        auto value = ler::parseJson(L"42");
        CHECK(value.isInt());
        CHECK(value.i == 42);
    }
    
    SUBCASE("Negative") {
        auto value = ler::parseJson(L"-123");
        CHECK(value.isInt());
        CHECK(value.i == -123);
    }
    
    SUBCASE("Zero") {
        auto value = ler::parseJson(L"0");
        CHECK(value.isInt());
        CHECK(value.i == 0);
    }
}

TEST_CASE("Json: Parse doubles") {
    SUBCASE("Positive decimal") {
        auto value = ler::parseJson(L"3.14");
        CHECK(value.isDouble());
        CHECK(value.d == doctest::Approx(3.14));
    }
    
    SUBCASE("Negative decimal") {
        auto value = ler::parseJson(L"-2.5");
        CHECK(value.isDouble());
        CHECK(value.d == doctest::Approx(-2.5));
    }
}

TEST_CASE("Json: Parse strings") {
    SUBCASE("Simple string") {
        auto value = ler::parseJson(L"\"hello\"");
        CHECK(value.isString());
        CHECK(value.s == L"hello");
    }
    
    SUBCASE("Empty string") {
        auto value = ler::parseJson(L"\"\"");
        CHECK(value.isString());
        CHECK(value.s == L"");
    }
    
    SUBCASE("String with spaces") {
        auto value = ler::parseJson(L"\"hello world\"");
        CHECK(value.isString());
        CHECK(value.s == L"hello world");
    }
}

TEST_CASE("Json: Parse arrays") {
    SUBCASE("Empty array") {
        auto value = ler::parseJson(L"[]");
        CHECK(value.isArray());
        CHECK(value.a.empty());
    }
    
    SUBCASE("Array with integers") {
        auto value = ler::parseJson(L"[1, 2, 3]");
        CHECK(value.isArray());
        CHECK(value.a.size() == 3);
        CHECK(value.a[0].i == 1);
        CHECK(value.a[1].i == 2);
        CHECK(value.a[2].i == 3);
    }
    
    SUBCASE("Array with mixed types") {
        auto value = ler::parseJson(L"[1, \"two\", true, null]");
        CHECK(value.isArray());
        CHECK(value.a.size() == 4);
        CHECK(value.a[0].isInt());
        CHECK(value.a[1].isString());
        CHECK(value.a[2].isBool());
        CHECK(value.a[3].isNull());
    }
}

TEST_CASE("Json: Parse objects") {
    SUBCASE("Empty object") {
        auto value = ler::parseJson(L"{}");
        CHECK(value.isObject());
        CHECK(value.o.empty());
    }
    
    SUBCASE("Simple object") {
        auto value = ler::parseJson(L"{\"name\": \"test\", \"value\": 42}");
        CHECK(value.isObject());
        CHECK(value.o.size() == 2);
        
        auto* name = value.tryGet(L"name");
        REQUIRE(name != nullptr);
        CHECK(name->isString());
        CHECK(name->s == L"test");
        
        auto* val = value.tryGet(L"value");
        REQUIRE(val != nullptr);
        CHECK(val->isInt());
        CHECK(val->i == 42);
    }
}

TEST_CASE("Json: Parse nested structures") {
    std::wstring json = LR"({
        "config": {
            "enabled": true,
            "count": 5
        },
        "items": [1, 2, 3]
    })";
    
    auto value = ler::parseJson(json);
    CHECK(value.isObject());
    
    auto* config = value.tryGet(L"config");
    REQUIRE(config != nullptr);
    CHECK(config->isObject());
    
    auto* enabled = config->tryGet(L"enabled");
    REQUIRE(enabled != nullptr);
    CHECK(enabled->isBool());
    CHECK(enabled->b == true);
    
    auto* items = value.tryGet(L"items");
    REQUIRE(items != nullptr);
    CHECK(items->isArray());
    CHECK(items->a.size() == 3);
}

TEST_CASE("Json: Invalid JSON throws exception") {
    SUBCASE("Unclosed string") {
        CHECK_THROWS_AS(ler::parseJson(L"\"unclosed"), ler::JsonParseError);
    }
    
    SUBCASE("Invalid value") {
        CHECK_THROWS_AS(ler::parseJson(L"undefined"), ler::JsonParseError);
    }
    
    SUBCASE("Trailing comma in array") {
        CHECK_THROWS_AS(ler::parseJson(L"[1, 2,]"), ler::JsonParseError);
    }
}

TEST_CASE("Json: Write JSON") {
    SUBCASE("Null") {
        auto value = ler::JsonValue::makeNull();
        auto json = ler::writeJson(value);
        CHECK(json.find(L"null") != std::wstring::npos);
    }
    
    SUBCASE("Boolean") {
        auto value = ler::JsonValue::makeBool(true);
        auto json = ler::writeJson(value);
        CHECK(json.find(L"true") != std::wstring::npos);
    }
    
    SUBCASE("Integer") {
        auto value = ler::JsonValue::makeInt(42);
        auto json = ler::writeJson(value);
        CHECK(json.find(L"42") != std::wstring::npos);
    }
    
    SUBCASE("String") {
        auto value = ler::JsonValue::makeString(L"hello");
        auto json = ler::writeJson(value);
        CHECK(json.find(L"\"hello\"") != std::wstring::npos);
    }
    
    SUBCASE("Array") {
        auto value = ler::JsonValue::makeArray({
            ler::JsonValue::makeInt(1),
            ler::JsonValue::makeInt(2),
            ler::JsonValue::makeInt(3)
        });
        auto json = ler::writeJson(value);
        CHECK(json.find(L"1") != std::wstring::npos);
        CHECK(json.find(L"2") != std::wstring::npos);
        CHECK(json.find(L"3") != std::wstring::npos);
    }
    
    SUBCASE("Object") {
        auto value = ler::JsonValue::makeObject({
            {L"name", ler::JsonValue::makeString(L"test")},
            {L"value", ler::JsonValue::makeInt(42)}
        });
        auto json = ler::writeJson(value);
        CHECK(json.find(L"\"name\"") != std::wstring::npos);
        CHECK(json.find(L"\"test\"") != std::wstring::npos);
        CHECK(json.find(L"\"value\"") != std::wstring::npos);
        CHECK(json.find(L"42") != std::wstring::npos);
    }
}

TEST_CASE("Json: Round-trip conversion") {
    std::wstring original = LR"({"name":"test","count":42,"enabled":true,"items":[1,2,3]})";
    auto parsed = ler::parseJson(original);
    auto written = ler::writeJson(parsed, 0);
    
    // Parse again to compare structure
    auto reparsed = ler::parseJson(written);
    CHECK(reparsed.isObject());
    CHECK(reparsed.tryGet(L"name")->s == L"test");
    CHECK(reparsed.tryGet(L"count")->i == 42);
    CHECK(reparsed.tryGet(L"enabled")->b == true);
    CHECK(reparsed.tryGet(L"items")->a.size() == 3);
}
