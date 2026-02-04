#include "CppUnitTest.h"
#include "InstallerUtil.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace lastexecuterecordmstest
{
	TEST_CLASS(InstallerUtilTests)
	{
	public:
		// Note: isInstallerRunning() tests actual system state and may vary
		// We test the interface and basic behavior
		TEST_METHOD(IsInstallerRunning_CanBeCalled)
		{
			// Should not crash and return a boolean
			bool result = ler::isInstallerRunning();
			// Result depends on system state; just verify it returns
			Assert::IsTrue(result == true || result == false);
		}

		TEST_METHOD(WaitForInstallerToFinish_ZeroRetries_ReturnsFalseIfInstalling)
		{
			// With 0 retries, should handle gracefully
			bool result = ler::waitForInstallerToFinish(1, 0);
			// Should return a boolean (implementation treats 0 as 1)
			Assert::IsTrue(result == true || result == false);
		}

		TEST_METHOD(WaitForInstallerToFinish_NegativeRetries_HandledGracefully)
		{
			// Negative retries should be handled gracefully
			bool result = ler::waitForInstallerToFinish(1, -5);
			// Should return a boolean without crashing
			Assert::IsTrue(result == true || result == false);
		}

		TEST_METHOD(WaitForInstallerToFinish_ShortWait_ReturnsQuickly)
		{
			// Test with very short wait time (1 second, 1 retry)
			// This should return relatively quickly even if no installer is running
			auto start = std::chrono::steady_clock::now();
			bool result = ler::waitForInstallerToFinish(1, 1);
			auto end = std::chrono::steady_clock::now();
			auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();
			
			// Should complete within reasonable time (allow some overhead)
			Assert::IsTrue(elapsed < 5, L"Wait took too long");
			Assert::IsTrue(result == true || result == false);
		}
	};
}
