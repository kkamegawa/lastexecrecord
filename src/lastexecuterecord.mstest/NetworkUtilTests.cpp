#include "CppUnitTest.h"
#include "NetworkUtil.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace lastexecuterecordmstest
{
	TEST_CLASS(NetworkUtilTests)
	{
	public:
		TEST_METHOD(HasInternetConnection_DoesNotCrash)
		{
			// This test verifies the function doesn't crash or throw
			// Actual return value depends on system state
			bool result = ler::hasInternetConnection();
			(void)result; // Suppress unused variable warning
		}

		TEST_METHOD(IsConnectionMetered_DoesNotCrash)
		{
			// This test verifies the function doesn't crash or throw
			// Actual return value depends on system state
			bool result = ler::isConnectionMetered();
			(void)result; // Suppress unused variable warning
		}

		TEST_METHOD(ShouldExecuteBasedOnNetwork_AlwaysExecute_ReturnsTrue)
		{
			// NetworkOption::AlwaysExecute (2) should always return true
			bool result = ler::shouldExecuteBasedOnNetwork(ler::NetworkOption::AlwaysExecute);
			Assert::IsTrue(result);
		}

		TEST_METHOD(EvaluateNetworkOption_AlwaysExecute_ReturnsKnownTrue)
		{
			ler::NetworkDecision decision = ler::evaluateNetworkOption(ler::NetworkOption::AlwaysExecute);
			Assert::IsTrue(decision.statusKnown);
			Assert::IsTrue(decision.shouldExecute);
			Assert::AreEqual(0u, decision.errorCode);
		}

		TEST_METHOD(EvaluateNetworkOption_Invalid_ReturnsUnknownFalse)
		{
			ler::NetworkDecision decision = ler::evaluateNetworkOption(static_cast<ler::NetworkOption>(99));
			Assert::IsFalse(decision.statusKnown);
			Assert::IsFalse(decision.shouldExecute);
			Assert::AreNotEqual(0u, decision.errorCode);
		}

		TEST_METHOD(ShouldExecuteBasedOnNetwork_ExecuteOnMetered_DoesNotCrash)
		{
			// NetworkOption::ExecuteOnMetered (1) depends on connection state
			// Should not crash or throw
			bool result = ler::shouldExecuteBasedOnNetwork(ler::NetworkOption::ExecuteOnMetered);
			(void)result; // Suppress unused variable warning
		}

		TEST_METHOD(ShouldExecuteBasedOnNetwork_ExecuteWhenConnected_DoesNotCrash)
		{
			// NetworkOption::ExecuteWhenConnected (0) depends on connection state
			// Should not crash or throw
			bool result = ler::shouldExecuteBasedOnNetwork(ler::NetworkOption::ExecuteWhenConnected);
			(void)result; // Suppress unused variable warning
		}

		TEST_METHOD(NetworkOption_EnumValues_AreCorrect)
		{
			// Verify enum values match specification
			Assert::AreEqual(0, static_cast<int>(ler::NetworkOption::ExecuteWhenConnected));
			Assert::AreEqual(1, static_cast<int>(ler::NetworkOption::ExecuteOnMetered));
			Assert::AreEqual(2, static_cast<int>(ler::NetworkOption::AlwaysExecute));
		}
	};
}
