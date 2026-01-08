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

	// RAII helper for temp directories
	class TempDirectory {
	public:
		std::wstring path;
		explicit TempDirectory(const wchar_t* leaf) : path(makeTempPath(leaf)) {}
		~TempDirectory() {
			// Delete all files in directory first
			WIN32_FIND_DATAW findData;
			std::wstring searchPath = path + L"\\*";
			HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);
			if (hFind != INVALID_HANDLE_VALUE) {
				do {
					if (wcscmp(findData.cFileName, L".") != 0 && wcscmp(findData.cFileName, L"..") != 0) {
						std::wstring filePath = path + L"\\" + findData.cFileName;
						DeleteFileW(filePath.c_str());
					}
				} while (FindNextFileW(hFind, &findData));
				FindClose(hFind);
			}
			RemoveDirectoryW(path.c_str());
		}
	};

	TEST_CLASS(ConfigTests)
	{
	public:
		TEST_METHOD(DefaultConfigPath_ReturnsUserProfilePath)
		{
			std::wstring path = ler::defaultConfigPath();
			
			// Should contain .lastexecrecord and config.json
			Assert::IsTrue(path.find(L".lastexecrecord") != std::wstring::npos);
			Assert::IsTrue(path.find(L"config.json") != std::wstring::npos);
			
			// If USERPROFILE is set, path should start with it
			std::wstring userProfile = ler::getEnvVar(L"USERPROFILE");
			if (!userProfile.empty()) {
				Assert::IsTrue(path.find(userProfile) == 0);
			}
		}

		TEST_METHOD(EnsureSampleConfigExists_CreatesDirectoryAndFile)
		{
			TempDirectory tempDir(L"configtest_dir");
			std::wstring configPath = ler::joinPath(tempDir.path, L"config.json");
			
			// Verify directory doesn't exist initially
			DWORD attrs = GetFileAttributesW(tempDir.path.c_str());
			Assert::IsTrue(attrs == INVALID_FILE_ATTRIBUTES);
			
			// Call ensureSampleConfigExists
			ler::ensureSampleConfigExists(configPath);
			
			// Verify directory was created
			attrs = GetFileAttributesW(tempDir.path.c_str());
			Assert::IsTrue(attrs != INVALID_FILE_ATTRIBUTES);
			Assert::IsTrue((attrs & FILE_ATTRIBUTE_DIRECTORY) != 0);
			
			// Verify config file was created
			Assert::IsTrue(ler::fileExists(configPath));
			
			// Verify config file has valid content
			ler::AppConfig cfg = ler::loadAndValidateConfig(configPath);
			Assert::AreEqual(1LL, cfg.version);
		}

		TEST_METHOD(EnsureSampleConfigExists_DoesNotOverwriteExisting)
		{
			TempDirectory tempDir(L"configtest_nooverwrite");
			std::wstring configPath = ler::joinPath(tempDir.path, L"config.json");
			
			// Create directory manually
			CreateDirectoryW(tempDir.path.c_str(), nullptr);
			
			// Write custom config
			std::wstring customContent = 
				L"{\n"
				L"  \"version\": 1,\n"
				L"  \"commands\": [\n"
				L"    { \"name\": \"custom\", \"exe\": \"test.exe\" }\n"
				L"  ]\n"
				L"}\n";
			ler::writeWStringToUtf8FileAtomic(configPath, customContent);
			
			// Call ensureSampleConfigExists
			ler::ensureSampleConfigExists(configPath);
			
			// Verify original content is preserved
			ler::AppConfig cfg = ler::loadAndValidateConfig(configPath);
			Assert::AreEqual(1u, static_cast<unsigned>(cfg.commands.size()));
			Assert::AreEqual(std::wstring(L"custom"), cfg.commands[0].name);
		}

		TEST_METHOD(EnsureSampleConfigExists_CreatesNestedDirectories)
		{
			TempDirectory tempBaseDir(L"configtest_base");
			std::wstring nestedDir = ler::joinPath(tempBaseDir.path, L"level1");
			nestedDir = ler::joinPath(nestedDir, L"level2");
			std::wstring configPath = ler::joinPath(nestedDir, L"config.json");
			
			// Call ensureSampleConfigExists
			ler::ensureSampleConfigExists(configPath);
			
			// Verify all nested directories were created
			DWORD attrs = GetFileAttributesW(nestedDir.c_str());
			Assert::IsTrue(attrs != INVALID_FILE_ATTRIBUTES);
			Assert::IsTrue((attrs & FILE_ATTRIBUTE_DIRECTORY) != 0);
			
			// Verify config file was created
			Assert::IsTrue(ler::fileExists(configPath));
			
			// Cleanup nested directories
			DeleteFileW(configPath.c_str());
			std::wstring level2 = nestedDir;
			std::wstring level1 = ler::getDirectoryName(level2);
			RemoveDirectoryW(level2.c_str());
			RemoveDirectoryW(level1.c_str());
		}

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

		TEST_METHOD(Load_WithNetworkOption_ParsesCorrectly)
		{
			TempFile tmp(L"networkoption.json");

			ler::writeWStringToUtf8FileAtomic(tmp.path,
				L"{\n"
				L"  \"networkOption\": 1,\n"
				L"  \"commands\": [ { \"name\": \"c1\", \"exe\": \"x.exe\" } ]\n"
				L"}\n");

			ler::AppConfig cfg = ler::loadAndValidateConfig(tmp.path);
			Assert::AreEqual(1, static_cast<int>(cfg.networkOption));
		}

		TEST_METHOD(Load_WithoutNetworkOption_DefaultsToAlwaysExecute)
		{
			TempFile tmp(L"nonetworkoption.json");

			ler::writeWStringToUtf8FileAtomic(tmp.path,
				L"{\n"
				L"  \"commands\": [ { \"name\": \"c1\", \"exe\": \"x.exe\" } ]\n"
				L"}\n");

			ler::AppConfig cfg = ler::loadAndValidateConfig(tmp.path);
			Assert::AreEqual(2, static_cast<int>(cfg.networkOption));
		}

		TEST_METHOD(Load_WithInvalidNetworkOption_Throws)
		{
			TempFile tmp(L"invalidnetwork.json");

			ler::writeWStringToUtf8FileAtomic(tmp.path,
				L"{\n"
				L"  \"networkOption\": 5,\n"
				L"  \"commands\": [ { \"name\": \"c1\", \"exe\": \"x.exe\" } ]\n"
				L"}\n");

			auto func = [&tmp]() { ler::loadAndValidateConfig(tmp.path); };
			Assert::ExpectException<ler::JsonParseError>(func);
		}

		TEST_METHOD(Load_WithNegativeNetworkOption_Throws)
		{
			TempFile tmp(L"negativenetwork.json");

			ler::writeWStringToUtf8FileAtomic(tmp.path,
				L"{\n"
				L"  \"networkOption\": -1,\n"
				L"  \"commands\": [ { \"name\": \"c1\", \"exe\": \"x.exe\" } ]\n"
				L"}\n");

			auto func = [&tmp]() { ler::loadAndValidateConfig(tmp.path); };
			Assert::ExpectException<ler::JsonParseError>(func);
		}
	};
}
