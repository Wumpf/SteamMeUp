#pragma once

#include <string>

enum class LogLevel
{
	INFO,
	FAIL,
};

void Log(LogLevel level, const std::string& message);

#define LOG(level, message) do { std::stringstream format; format << message << "\n"; Log(level, format.str()); } while(false)


std::string ToUtf8(wchar_t* string);
std::string GetLastErrorAsString();
