#include <windows.h>
#include <Xinput.h>
#include <cstdio>
#include <string>
#include <sstream>
#include <locale>
#include <codecvt>
#include "resource.h"

// Undocumented button constants
#define XINPUT_GAMEPAD_GUIDE_BUTTON 0x0400
#define XINPUT_GAMEPAD_UNKNOWN		0x0800

typedef struct
{
	unsigned long eventCount;
	WORD          wButtons;
	BYTE          bLeftTrigger;
	BYTE          bRightTrigger;
	SHORT         sThumbLX;
	SHORT         sThumbLY;
	SHORT         sThumbRX;
	SHORT         sThumbRY;
} XINPUT_STATE_EX;
int(__stdcall *XInputGetStateEx) (int, XINPUT_STATE_EX*);

wchar_t steamInstallationPath[MAX_PATH];

enum class LogLevel
{
	INFO,
	FAIL,
};

void Log(LogLevel level, const std::string message)
{
#ifdef NDEBUG
	if (level != LogLevel::FAIL)
		return;
#else
	printf(message.c_str());
#endif
	OutputDebugStringA(message.c_str());
}

#define LOG(level, message) do { std::stringstream format; format << message << "\n"; Log(level, format.str()); } while(false)

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

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

bool Startup()
{
	// Query steam installation location.
	DWORD bufferSize = sizeof(steamInstallationPath);
	if (RegGetValue(HKEY_CURRENT_USER,
					L"Software\\Valve\\Steam",
					L"SteamExe",
					RRF_RT_REG_SZ,
					NULL,
					steamInstallationPath,
					&bufferSize) == ERROR_SUCCESS)
	{
		LOG(LogLevel::FAIL, "Found steam installation at: " << ToUtf8(steamInstallationPath));
	}
	else
	{
		LOG(LogLevel::INFO, "Failed to find steam installation!");
		return false;
	}

	// Query secret gamepad query function that allows to get the guide button.
	// The default XInputGetState will never tell us about the guide button.
	// https://forums.tigsource.com/index.php?topic=26792.msg847843#msg847843
	wchar_t xinputDllPath[MAX_PATH];
	GetSystemDirectory(xinputDllPath, sizeof(xinputDllPath));
	wcscat_s(xinputDllPath, L"\\xinput1_4.dll");
	HINSTANCE xinputDll = LoadLibrary(xinputDllPath);
	if (!xinputDll)
	{
		LOG(LogLevel::FAIL, "Failed to load xinput dll at: " << ToUtf8(xinputDllPath));
		return false;
	}
	XInputGetStateEx = (decltype(XInputGetStateEx))GetProcAddress(xinputDll, (LPCSTR)100);

	// Deprecated.
	//XInputEnable(TRUE);


	// Need a window to be able to set a tray icon.
	WNDCLASSEXW wcex;
	memset(&wcex, 0, sizeof(WNDCLASSEXW));
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.lpfnWndProc = WndProc;
	wcex.hInstance = GetModuleHandle(nullptr);
	wcex.lpszClassName = L"SteamMeUp";
	RegisterClassExW(&wcex);

	HWND hWnd = CreateWindowExW(0, wcex.lpszClassName, L"SteamMeUp", WS_DISABLED, 0, 0, 0, 0, nullptr, nullptr, wcex.hInstance, nullptr);
	if (!hWnd)
	{
		LOG(LogLevel::FAIL, "Failed to create Window: " << GetLastErrorAsString());
		return false;
	}

	NOTIFYICONDATA nid;
	memset(&nid, 0, sizeof(NOTIFYICONDATA));
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = hWnd;
	nid.uID = 1;
	nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nid.uCallbackMessage = WM_USER + 200;
	nid.hIcon = (HICON)LoadImage(wcex.hInstance, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
	lstrcpy(nid.szTip, L"SteamMeUp - Start Steam big picture with the Xbox home button.");
	if (!Shell_NotifyIcon(NIM_ADD, &nid))
	{
		LOG(LogLevel::FAIL, "Failed to display tray icon: " << GetLastErrorAsString());
		return false;
	}

	return true;
}

void StartSteamInBigPictureMode()
{
	wchar_t steamBigPictureStartupCommandLine[MAX_PATH * 2];
	wsprintf(steamBigPictureStartupCommandLine, L"%s -bigpicture", steamInstallationPath);

	LOG(LogLevel::INFO, "Launching steam in big picture mode via \"" << steamBigPictureStartupCommandLine  << "\"...");

	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	if (CreateProcess(NULL, steamBigPictureStartupCommandLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
		LOG(LogLevel::INFO, "Started steam successfully.");
	else
		LOG(LogLevel::FAIL, "Failed to start steam: " << GetLastErrorAsString());
}

void RunLoop()
{
	const int updateFrequencyHz = 10;

	XINPUT_STATE_EX oldState[XUSER_MAX_COUNT];
	ZeroMemory(oldState, sizeof(oldState));

	while (true)
	{
		for (int i = 0; i < XUSER_MAX_COUNT; ++i)
		{
			XINPUT_STATE_EX newState;
			auto result = XInputGetStateEx(i, &newState);
			if (result)
			{
				ZeroMemory(&oldState[i], sizeof(oldState[i]));
				continue;
			}

			// Released guide button - this way turning off doesn't trigger this.
			if ((oldState[i].wButtons & XINPUT_GAMEPAD_GUIDE_BUTTON) != 0 && 
				(newState.wButtons    & XINPUT_GAMEPAD_GUIDE_BUTTON) == 0)
			{
				StartSteamInBigPictureMode();
			}

			oldState[i] = newState;
		}
		Sleep(1000 / updateFrequencyHz);
	}
}

int main()
{
	if (Startup())
		RunLoop();
	else
		return 1;
	return 0;
}
