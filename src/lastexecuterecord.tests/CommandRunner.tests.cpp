#include <gtest/gtest.h>

#include "CommandRunner.h"

namespace {

TEST(CommandRunner, QuoteArg_NoSpaces_NoQuotes_ReturnsSame) {
    EXPECT_EQ(ler::quoteArgForWindowsCommandLine(L"abc"), L"abc");
}

TEST(CommandRunner, QuoteArg_WithSpace_AddsQuotes) {
    EXPECT_EQ(ler::quoteArgForWindowsCommandLine(L"a b"), L"\"a b\"");
}

TEST(CommandRunner, QuoteArg_WithQuote_EscapesQuote) {
    // Quote should be escaped inside the quoted string
    EXPECT_EQ(ler::quoteArgForWindowsCommandLine(L"a\"b"), L"\"a\\\"b\"");
}

TEST(CommandRunner, QuoteArg_TrailingBackslashes_EscapesCorrectly) {
    // Trailing backslashes must be doubled before the terminating quote.
    EXPECT_EQ(ler::quoteArgForWindowsCommandLine(L"C:\\path\\"), L"\"C:\\path\\\\\"");
}

TEST(CommandRunner, QuoteArg_Combo_BackslashesBeforeQuote_EscapesCorrectly) {
    // Backslashes before a quote must be doubled, and the quote escaped.
    EXPECT_EQ(ler::quoteArgForWindowsCommandLine(L"a\\\"b"), L"\"a\\\\\\\"b\"");
}

} // namespace
