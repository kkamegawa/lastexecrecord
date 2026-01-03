#include "CppUnitTest.h"
#include "CommandRunner.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace lastexecuterecordmstest
{
	TEST_CLASS(CommandRunnerTests)
	{
	public:
		TEST_METHOD(QuoteArg_NoSpaces_NoQuotes_ReturnsSame)
		{
			Assert::AreEqual(std::wstring(L"abc"), ler::quoteArgForWindowsCommandLine(L"abc"));
		}

		TEST_METHOD(QuoteArg_WithSpace_AddsQuotes)
		{
			Assert::AreEqual(std::wstring(L"\"a b\""), ler::quoteArgForWindowsCommandLine(L"a b"));
		}

		TEST_METHOD(QuoteArg_WithQuote_EscapesQuote)
		{
			// Quote should be escaped inside the quoted string
			Assert::AreEqual(std::wstring(L"\"a\\\"b\""), ler::quoteArgForWindowsCommandLine(L"a\"b"));
		}

		TEST_METHOD(QuoteArg_TrailingBackslashes_EscapesCorrectly)
		{
			// Trailing backslashes must be doubled before the terminating quote.
			Assert::AreEqual(std::wstring(L"\"C:\\path\\\\\""), ler::quoteArgForWindowsCommandLine(L"C:\\path\\"));
		}

		TEST_METHOD(QuoteArg_Combo_BackslashesBeforeQuote_EscapesCorrectly)
		{
			// Backslashes before a quote must be doubled, and the quote escaped.
			Assert::AreEqual(std::wstring(L"\"a\\\\\\\"b\""), ler::quoteArgForWindowsCommandLine(L"a\\\"b"));
		}
	};
}
