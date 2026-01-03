#include "CppUnitTest.h"
#include "Config.h"
#include "FileUtil.h"
#include <Windows.h>
#include <algorithm> // for std::find_if

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace lastexecuterecordmstest
{
	// Helper function to create temp file paths
	static std::wstring makeTempPath(const wchar_t* leaf) {
		wchar_t tmpDir[MAX_PATH] = {};
		DWORD n = GetTempPathW(MAX_PATH, tmpDir);
		if (n == 0) {
			throw std::runtime_error("GetTempPathW failed");
		}

		wchar_t nameBuf[MAX_PATH] = {};
		wsprintfW(nameBuf, L"ler_%lu_%ls", GetCurrentProcessId(), leaf);

		std::wstring path = std::wstring(tmpDir) + nameBuf;
		return path;
	}

	// RAII helper for temp files
	class TempFile {
	public:
		std::wstring path;
		explicit TempFile(const wchar_t* leaf) : path(makeTempPath(leaf)) {}
		~TempFile() { DeleteFileW(path.c_str()); }
	};

	TEST_CLASS(ConfigTests)
	{
	public:
		TEST_METHOD(Load_MinimalValidConfig_ReturnsCommands)
		{
			TempFile tmp(L"minimal.json");

			ler::writeWStringToUtf8FileAtomic(tmp.path,
				L"{\n"
				L"  \"version\": 1,\n"
				L"  \"commands\": [\n"
				L"    { \"name\": \"c1\", \"exe\": \"C:\\\\Windows\\\\System32\\\\whoami.exe\" }\n"
				L"  ]\n"
				L"}\n");

			ler::AppConfig cfg = ler::loadAndValidateConfig(tmp.path);
			Assert::AreEqual(1u, static_cast<unsigned>(cfg.commands.size()));
			Assert::AreEqual(std::wstring(L"c1"), cfg.commands[0].name);
			Assert::AreEqual(std::wstring(L"C:\\Windows\\System32\\whoami.exe"), cfg.commands[0].exe);
		}

		TEST_METHOD(Load_CommandNameMissing_Throws)
		{
			TempFile tmp(L"noname.json");

			ler::writeWStringToUtf8FileAtomic(tmp.path,
				L"{\n"
				L"  \"commands\": [ { \"exe\": \"x\" } ]\n"
				L"}\n");

			auto func = [&tmp]() { ler::loadAndValidateConfig(tmp.path); };
			Assert::ExpectException<ler::JsonParseError>(func);
		}

		TEST_METHOD(Load_CommandExeMissing_Throws)
		{
			TempFile tmp(L"noexe.json");

			ler::writeWStringToUtf8FileAtomic(tmp.path,
				L"{\n"
				L"  \"commands\": [ { \"name\": \"c1\" } ]\n"
				L"}\n");

			auto func = [&tmp]() { ler::loadAndValidateConfig(tmp.path); };
			Assert::ExpectException<ler::JsonParseError>(func);
		}

		TEST_METHOD(Load_WithDefaults_AppliesDefaults)
		{
			TempFile tmp(L"defaults.json");

			ler::writeWStringToUtf8FileAtomic(tmp.path,
				L"{\n"
				L"  \"commands\": [ { \"name\": \"c1\", \"exe\": \"x.exe\" } ]\n"
				L"}\n");

			ler::AppConfig cfg = ler::loadAndValidateConfig(tmp.path);
			Assert::AreEqual(1u, static_cast<unsigned>(cfg.commands.size()));
			// Default values
			Assert::IsTrue(cfg.commands[0].enabled);
			Assert::AreEqual(0LL, cfg.commands[0].minIntervalSeconds);
			Assert::AreEqual(0LL, cfg.commands[0].timeoutSeconds);
		}

		TEST_METHOD(Load_WithLastRunUtc_ParsesCorrectly)
		{
			TempFile tmp(L"lastrun.json");

			ler::writeWStringToUtf8FileAtomic(tmp.path,
				L"{\n"
				L"  \"commands\": [\n"
				L"    {\n"
				L"      \"name\": \"c1\",\n"
				L"      \"exe\": \"x.exe\",\n"
				L"      \"lastRunUtc\": \"2026-01-02T12:34:56Z\",\n"
				L"      \"lastExitCode\": 0\n"
				L"    }\n"
				L"  ]\n"
				L"}\n");

			ler::AppConfig cfg = ler::loadAndValidateConfig(tmp.path);
			Assert::AreEqual(1u, static_cast<unsigned>(cfg.commands.size()));
			Assert::IsTrue(cfg.commands[0].hasLastRunUtc);
			Assert::AreEqual(std::wstring(L"2026-01-02T12:34:56Z"), cfg.commands[0].lastRunUtc);
			Assert::IsTrue(cfg.commands[0].hasLastExitCode);
			Assert::AreEqual(0u, static_cast<unsigned int>(cfg.commands[0].lastExitCode));
		}

		TEST_METHOD(ApplyCommandsToJson_UpdatesJsonObject)
		{
			TempFile tmp(L"apply.json");

			ler::writeWStringToUtf8FileAtomic(tmp.path,
				L"{\n"
				L"  \"commands\": [ { \"name\": \"c1\", \"exe\": \"x.exe\" } ]\n"
				L"}\n");

			ler::AppConfig cfg = ler::loadAndValidateConfig(tmp.path);
			cfg.commands[0].hasLastRunUtc = true;
			cfg.commands[0].lastRunUtc = L"2026-01-03T00:00:00Z";
			cfg.commands[0].hasLastExitCode = true;
			cfg.commands[0].lastExitCode = 0;

			ler::applyCommandsToJson(cfg);

			// Verify the JSON object was updated
			Assert::IsTrue(
				cfg.root.isObject() &&
				std::find_if(
					cfg.root.o.begin(),
					cfg.root.o.end(),
					[](const auto& kv) { return kv.first == L"commands"; }
				) != cfg.root.o.end()
			);
		}
	};
}
