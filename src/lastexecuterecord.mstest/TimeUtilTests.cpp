#include "CppUnitTest.h"
#include "TimeUtil.h"
#include <cmath>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace lastexecuterecordmstest
{
	TEST_CLASS(TimeUtilTests)
	{
	public:
		TEST_METHOD(TryParseIso_ValidZ_ReturnsTrue)
		{
			std::int64_t epoch = -1;
			Assert::IsTrue(ler::tryParseIsoUtcToEpochSeconds(L"2026-01-02T12:34:56Z", epoch));
		}

		TEST_METHOD(TryParseIso_ValidNoZ_TreatedAsUtc_ReturnsTrue)
		{
			std::int64_t epoch = -1;
			Assert::IsTrue(ler::tryParseIsoUtcToEpochSeconds(L"2026-01-02T12:34:56", epoch));
		}

		TEST_METHOD(TryParseIso_InvalidLength_ReturnsFalse)
		{
			std::int64_t epoch = -1;
			Assert::IsFalse(ler::tryParseIsoUtcToEpochSeconds(L"2026-01-02T12:34", epoch));
		}

		TEST_METHOD(TryParseIso_InvalidSeparator_ReturnsFalse)
		{
			std::int64_t epoch = -1;
			Assert::IsFalse(ler::tryParseIsoUtcToEpochSeconds(L"2026/01/02T12:34:56Z", epoch));
		}

		TEST_METHOD(TryParseIso_FractionalSeconds_ReturnsFalse)
		{
			std::int64_t epoch = -1;
			Assert::IsFalse(ler::tryParseIsoUtcToEpochSeconds(L"2026-01-02T12:34:56.123Z", epoch));
		}

		TEST_METHOD(FormatEpoch_Zero_ReturnsUnixEpoch)
		{
			// 0 should be 1970-01-01T00:00:00Z
			Assert::AreEqual(std::wstring(L"1970-01-01T00:00:00Z"), ler::formatEpochSecondsAsIsoUtc(0));
		}

		TEST_METHOD(FormatAndParse_RoundTrip_ReturnsSameEpoch)
		{
			const std::int64_t samples[] = { 0, 1, 60, 3600, 1700000000 };

			for (auto e : samples) {
				std::wstring s = ler::formatEpochSecondsAsIsoUtc(e);
				std::int64_t back = -1;
				Assert::IsTrue(ler::tryParseIsoUtcToEpochSeconds(s, back),
					(L"Failed to parse formatted epoch: " + s).c_str());
				Assert::AreEqual(e, back);
			}
		}
	};
}
