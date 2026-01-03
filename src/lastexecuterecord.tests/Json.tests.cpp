#include <gtest/gtest.h>

#include "Json.h"

namespace {

TEST(Json, Parse_Null_ReturnsNull) {
    ler::JsonValue v = ler::parseJson(L"null");
    EXPECT_TRUE(v.isNull());
}

TEST(Json, Parse_Bool_ReturnsBool) {
    ler::JsonValue v = ler::parseJson(L"true");
    EXPECT_TRUE(v.isBool());
    EXPECT_TRUE(v.asBool(L"ctx"));
}

TEST(Json, Parse_Int_ReturnsInt) {
    ler::JsonValue v = ler::parseJson(L"123");
    EXPECT_TRUE(v.isInt());
    EXPECT_EQ(v.asInt(L"ctx"), 123);
}

TEST(Json, Parse_Double_ReturnsDouble) {
    ler::JsonValue v = ler::parseJson(L"1.25");
    EXPECT_TRUE(v.isDouble());
    EXPECT_DOUBLE_EQ(v.d, 1.25);
}

TEST(Json, Parse_String_EscapesHandled) {
    ler::JsonValue v = ler::parseJson(L"\"a\\n\\t\\\"\\\\b\"");
    EXPECT_TRUE(v.isString());
    EXPECT_EQ(v.s, std::wstring(L"a\n\t\"\\b"));
}

TEST(Json, Parse_Array_ReturnsArray) {
    ler::JsonValue v = ler::parseJson(L"[1,2,3]");
    EXPECT_TRUE(v.isArray());
    ASSERT_EQ(v.a.size(), 3u);
    EXPECT_EQ(v.a[0].asInt(L"ctx"), 1);
    EXPECT_EQ(v.a[1].asInt(L"ctx"), 2);
    EXPECT_EQ(v.a[2].asInt(L"ctx"), 3);
}

TEST(Json, Parse_Object_PreservesInsertionOrder) {
    ler::JsonValue v = ler::parseJson(L"{\"b\":1,\"a\":2}");
    ASSERT_TRUE(v.isObject());
    ASSERT_EQ(v.o.size(), 2u);
    EXPECT_EQ(v.o[0].first, L"b");
    EXPECT_EQ(v.o[1].first, L"a");
}

TEST(Json, Parse_TrailingChars_Throws) {
    EXPECT_THROW((void)ler::parseJson(L"true false"), ler::JsonParseError);
}

TEST(Json, Write_String_Escapes) {
    ler::JsonValue v = ler::JsonValue::makeString(L"a\n\t\"\\");
    std::wstring out = ler::writeJson(v);
    // Should contain escaped sequences
    EXPECT_NE(out.find(L"\\n"), std::wstring::npos);
    EXPECT_NE(out.find(L"\\t"), std::wstring::npos);
    EXPECT_NE(out.find(L"\\\""), std::wstring::npos);
    EXPECT_NE(out.find(L"\\\\"), std::wstring::npos);
}

TEST(Json, Write_Object_IndentationStable) {
    ler::JsonValue obj = ler::JsonValue::makeObject({
        {L"a", ler::JsonValue::makeInt(1)},
        {L"b", ler::JsonValue::makeBool(true)},
    });

    std::wstring out = ler::writeJson(obj, 2);
    // basic shape expectations
    EXPECT_NE(out.find(L"\n"), std::wstring::npos);
    EXPECT_NE(out.find(L"\"a\""), std::wstring::npos);
    EXPECT_NE(out.find(L"\"b\""), std::wstring::npos);
}

} // namespace
