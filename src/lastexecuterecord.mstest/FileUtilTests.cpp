#include "CppUnitTest.h"
#include "FileUtil.h"
#include <Windows.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace lastexecuterecordmstest
{
	// Helper to create temp file path
	static std::wstring makeTempPath(const wchar_t* leaf) {
		wchar_t tmpDir[MAX_PATH] = {};
		DWORD n = GetTempPathW(MAX_PATH, tmpDir);
		if (n == 0) {
			throw std::runtime_error("GetTempPathW failed");
		}

		wchar_t nameBuf[MAX_PATH] = {};
		wsprintfW(nameBuf, L"ler_%lu_%ls", GetCurrentProcessId(), leaf);

		return std::wstring(tmpDir) + nameBuf;
	}

	class TempFile {
	public:
		std::wstring path;
		explicit TempFile(const wchar_t* leaf) : path(makeTempPath(leaf)) {}
		~TempFile() { DeleteFileW(path.c_str()); }
	};

	TEST_CLASS(FileUtilTests)
	{
	public:
		TEST_METHOD(ReadWrite_Utf8WithoutBom_RoundTrip)
		{
			TempFile tmp(L"utf8.txt");
			std::wstring content = L"Hello, 世界! こんにちは";

			ler::writeWStringToUtf8FileAtomic(tmp.path, content);
			std::wstring read = ler::readUtf8FileToWString(tmp.path);

			Assert::AreEqual(content, read);
		}

		TEST_METHOD(Write_AtomicReplace_OverwritesExisting)
		{
			TempFile tmp(L"replace.txt");

			// Write initial content
			ler::writeWStringToUtf8FileAtomic(tmp.path, L"original");
			std::wstring read1 = ler::readUtf8FileToWString(tmp.path);
			Assert::AreEqual(std::wstring(L"original"), read1);

			// Overwrite with new content
			ler::writeWStringToUtf8FileAtomic(tmp.path, L"replaced");
			std::wstring read2 = ler::readUtf8FileToWString(tmp.path);
			Assert::AreEqual(std::wstring(L"replaced"), read2);
		}

		TEST_METHOD(Read_EmptyFile_ReturnsEmpty)
		{
			TempFile tmp(L"empty.txt");

			ler::writeWStringToUtf8FileAtomic(tmp.path, L"");
			std::wstring read = ler::readUtf8FileToWString(tmp.path);

			Assert::AreEqual(std::wstring(L""), read);
		}

		TEST_METHOD(Read_NonExistentFile_Throws)
		{
			std::wstring nonExistent = makeTempPath(L"doesnotexist.txt");

			auto func = [&nonExistent]() {
				ler::readUtf8FileToWString(nonExistent);
			};

			Assert::ExpectException<std::runtime_error>(func);
		}

		TEST_METHOD(Lock_AcquireLock_Succeeds)
		{
			TempFile tmp(L"lock.txt");

			// First lock should succeed
			ler::FileLock lock = ler::acquireLockFile(tmp.path);
			Assert::IsTrue(lock.h != INVALID_HANDLE_VALUE);
		}

		TEST_METHOD(Lock_DoubleLock_SecondFails)
		{
			TempFile tmp(L"doublelock.txt");

			// First lock succeeds
			ler::FileLock lock1 = ler::acquireLockFile(tmp.path);
			Assert::IsTrue(lock1.h != INVALID_HANDLE_VALUE);

			// Second lock should fail (throw)
			auto func = [&tmp]() {
				ler::FileLock lock2 = ler::acquireLockFile(tmp.path);
			};

			Assert::ExpectException<std::runtime_error>(func);
		}

		TEST_METHOD(PathJoin_CombinesPaths)
		{
			std::wstring result = ler::joinPath(L"C:\\temp", L"file.txt");
			Assert::AreEqual(std::wstring(L"C:\\temp\\file.txt"), result);
		}

		TEST_METHOD(ChangeExtension_ChangesExtension)
		{
			std::wstring result = ler::changeExtension(L"C:\\temp\\file.txt", L".json");
			Assert::AreEqual(std::wstring(L"C:\\temp\\file.json"), result);
		}

		TEST_METHOD(GetDirectoryName_ExtractsDirectory)
		{
			std::wstring result = ler::getDirectoryName(L"C:\\temp\\file.txt");
			Assert::AreEqual(std::wstring(L"C:\\temp"), result);
		}
	};
}
