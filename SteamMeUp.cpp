#include <windows.h>
#include <Xinput.h>
#include <cstdio>
#include <string>
#include <sstream>
#include "resource.h"
#include "Log.h"

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

bool InitTray();

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


	return InitTray();
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
