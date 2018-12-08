#include "Log.h"

#include <Windows.h>
#include <locale>
#include <codecvt>

void Log(LogLevel level, const std::string& message)
{
#ifdef NDEBUG
	if (level != LogLevel::FAIL)
		return;
#else
	printf(message.c_str());
#endif
	OutputDebugStringA(message.c_str());

	if (level == LogLevel::FAIL)
		MessageBoxA(NULL, message.c_str(), NULL, MB_OK | MB_ICONERROR);
}

std::string ToUtf8(wchar_t* string)
{
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> utf16conv;
	return utf16conv.to_bytes(string);
}

std::string GetLastErrorAsString()
{
	DWORD errorMessageID = ::GetLastError();
	if (errorMessageID == 0)
		return std::string();

	LPSTR messageBuffer = nullptr;
	size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

	std::string message(messageBuffer, size);
	LocalFree(messageBuffer);
	return message;
}