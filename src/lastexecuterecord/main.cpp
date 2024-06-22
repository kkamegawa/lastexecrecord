#include <algorithm>
#include <iomanip>
#include <Windows.h>
#include <iostream>
#include <sstream>

/**
 * 
 * @param lpszFileName 
 * @return 
 */
bool IsExistFile(LPCWCHAR lpszFileName) {
    DWORD fileAttributes = GetFileAttributes(lpszFileName);
    if (fileAttributes != INVALID_FILE_ATTRIBUTES && !(fileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
		// file exists and not read only
        if (fileAttributes & FILE_ATTRIBUTE_READONLY) {
            return false;
        }
        return true;
    }
    return false;
}

/**
 * 
 * @param str 
 * @return 
 */
bool IsNumericString(LPCWSTR str) {
	return std::all_of(str, str + wcslen(str), iswdigit);
}


/**
 * 
 * @param lpszFileName 
 * @param lpszStreamName 
 * @param lpszData 
 * @return 
 */
bool updateOrInsertNTFSDataStream(LPCWCHAR lpszFileName, LPCWCHAR lpszStreamName, LPSYSTEMTIME lpSystemTime) {
	
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(4) << lpSystemTime->wYear << "-"
        << std::setw(2) << lpSystemTime->wMonth << "-"
        << std::setw(2) << lpSystemTime->wDay << "T"
        << std::setw(2) << lpSystemTime->wHour << ":"
        << std::setw(2) << lpSystemTime->wMinute << ":"
        << std::setw(2) << lpSystemTime->wSecond;

	HANDLE hFile = CreateFile(lpszFileName, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		return false;
	}
	HANDLE hStream = CreateFile(lpszFileName, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hStream == INVALID_HANDLE_VALUE) {
		CloseHandle(hFile);
		return false;
	}
	DWORD dwWritten;
	WriteFile(hStream, oss.str().c_str(), oss.str().length() * sizeof(WCHAR), &dwWritten, NULL);
	CloseHandle(hStream);
	CloseHandle(hFile);
	return true;
}

bool AddTimefromCurrentTime(LPCWSTR lpszAddSecond, LPSYSTEMTIME nextTime)
{
	SYSTEMTIME st;
	GetSystemTime(&st);
	FILETIME ft;
	SystemTimeToFileTime(&st, &ft);
	ULARGE_INTEGER uli;
	uli.LowPart = ft.dwLowDateTime;
	uli.HighPart = ft.dwHighDateTime;
	uli.QuadPart += _wtoi(lpszAddSecond) * 10000000;
	ft.dwLowDateTime = uli.LowPart;
	ft.dwHighDateTime = uli.HighPart;
	FileTimeToSystemTime(&ft, nextTime);
	return true;
}

/**
 * 
 * @param argc 
 * @param lpszArgs 
 * @return 
 */
int main(int argc, LPCWCHAR  lpszArgs[]) {
	if (argc != 2 || argc !=1) {
		std::wcout << L"Usage: " << lpszArgs << L" <filename>" << std::endl;
		return 1;
	}
    if(IsExistFile(lpszArgs[1]) == false)
    {
		std::wcout << L"File not found or not writeable." << std::endl;
		return 1;
    }
	if (argc == 2 && IsNumericString(lpszArgs[2]) == false)
	{
		std::wcout << L"Invalid argument. second Argument must numeric value" << std::endl;
		return 1;
	}
	SYSTEMTIME systemtime;
	if (argc == 2)
	{
		AddTimefromCurrentTime(lpszArgs[1], &systemtime);
	}else
	{
		// next 12 hours
		AddTimefromCurrentTime(L"43200", &systemtime);
	}

	updateOrInsertNTFSDataStream(lpszArgs[1], L"LASTEXECTIME", &systemtime);
    return 0;
}
