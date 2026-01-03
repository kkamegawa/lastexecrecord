#include "CppUnitTest.h"
#include "Json.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace lastexecuterecordmstest
{
	TEST_CLASS(JsonTests)
	{
	public:
		TEST_METHOD(Parse_Null_ReturnsNull)
		{
			ler::JsonValue v = ler::parseJson(L"null");
			Assert::IsTrue(v.isNull());
		}

		TEST_METHOD(Parse_Bool_ReturnsBool)
		{
			ler::JsonValue v = ler::parseJson(L"true");
			Assert::IsTrue(v.isBool());
			Assert::IsTrue(v.asBool(L"ctx"));
		}

		TEST_METHOD(Parse_Int_ReturnsInt)
		{
			ler::JsonValue v = ler::parseJson(L"123");
			Assert::IsTrue(v.isInt());
			Assert::AreEqual(123LL, v.asInt(L"ctx"));
		}

		TEST_METHOD(Parse_Double_ReturnsDouble)
		{
			ler::JsonValue v = ler::parseJson(L"1.25");
			Assert::IsTrue(v.isDouble());
			Assert::AreEqual(1.25, v.d);
		}

		TEST_METHOD(Parse_String_EscapesHandled)
		{
			ler::JsonValue v = ler::parseJson(L"\"a\\n\\t\\\"\\\\b\"");
			Assert::IsTrue(v.isString());
			Assert::AreEqual(std::wstring(L"a\n\t\"\\b"), v.s);
		}

		TEST_METHOD(Parse_Array_ReturnsArray)
		{
			ler::JsonValue v = ler::parseJson(L"[1,2,3]");
			Assert::IsTrue(v.isArray());
			Assert::AreEqual(3u, static_cast<unsigned>(v.a.size()));
			Assert::AreEqual(1LL, v.a[0].asInt(L"ctx"));
			Assert::AreEqual(2LL, v.a[1].asInt(L"ctx"));
			Assert::AreEqual(3LL, v.a[2].asInt(L"ctx"));
		}

		TEST_METHOD(Parse_Object_PreservesInsertionOrder)
		{
			ler::JsonValue v = ler::parseJson(L"{\"b\":1,\"a\":2}");
			Assert::IsTrue(v.isObject());
			Assert::AreEqual(2u, static_cast<unsigned>(v.o.size()));
			Assert::AreEqual(std::wstring(L"b"), v.o[0].first);
			Assert::AreEqual(std::wstring(L"a"), v.o[1].first);
		}

		TEST_METHOD(Parse_NestedObject_Works)
		{
			ler::JsonValue v = ler::parseJson(L"{\"x\":{\"y\":42}}");
			Assert::IsTrue(v.isObject());
			Assert::AreEqual(1u, static_cast<unsigned>(v.o.size()));
			Assert::IsTrue(v.o[0].second.isObject());
			Assert::AreEqual(42LL, v.o[0].second.o[0].second.asInt(L"ctx"));
		}

		TEST_METHOD(Write_SimpleObject_FormatsCorrectly)
		{
			ler::JsonValue root;
			root = ler::JsonValue::makeObject(std::vector<std::pair<std::wstring, ler::JsonValue>>{});
			root.o.emplace_back(L"name", ler::JsonValue::makeString(L"test"));
			root.o.emplace_back(L"value", ler::JsonValue::makeInt(123));

			std::wstring json = ler::writeJson(root);
			// Should contain both keys
			Assert::IsTrue(json.find(L"\"name\"") != std::wstring::npos);
			Assert::IsTrue(json.find(L"\"test\"") != std::wstring::npos);
			Assert::IsTrue(json.find(L"\"value\"") != std::wstring::npos);
			Assert::IsTrue(json.find(L"123") != std::wstring::npos);
		}

		TEST_METHOD(Write_Array_FormatsCorrectly)
		{
			ler::JsonValue root;
			root = ler::JsonValue::makeArray(std::vector<ler::JsonValue>{});
			root.addItem(ler::JsonValue::makeInt(1));
			root.addItem(ler::JsonValue::makeInt(2));
			root.addItem(ler::JsonValue::makeInt(3));

			std::wstring json = ler::writeJson(root);
			Assert::IsTrue(json.find(L"[") != std::wstring::npos);
			Assert::IsTrue(json.find(L"]") != std::wstring::npos);
			Assert::IsTrue(json.find(L"1") != std::wstring::npos);
			Assert::IsTrue(json.find(L"2") != std::wstring::npos);
			Assert::IsTrue(json.find(L"3") != std::wstring::npos);
		}

		TEST_METHOD(Parse_TrailingComma_Throws)
		{
			auto func = []() { ler::parseJson(L"[1,2,]"); };
			Assert::ExpectException<std::runtime_error>(func);
		}

		TEST_METHOD(Parse_InvalidJson_Throws)
		{
			auto func = []() { ler::parseJson(L"{invalid}"); };
			Assert::ExpectException<std::runtime_error>(func);
		}
	};
}
