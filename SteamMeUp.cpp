#include <windows.h>
#include <Xinput.h>

#include <thread>
#include <atomic>

#include "SteamMeUp.h"
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

wchar_t g_steamInstallationPath[MAX_PATH] = L"";


void StartSteam(bool bigPicture)
{
	wchar_t steamStartupCommandLine[MAX_PATH * 2];
	if (bigPicture)
		wsprintf(steamStartupCommandLine, L"%s -bigpicture", g_steamInstallationPath);
	else
		lstrcpyW(steamStartupCommandLine, g_steamInstallationPath);

	LOG(LogLevel::INFO, "Launching steam in big picture mode via \"" << steamStartupCommandLine << "\"...");

	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	if (CreateProcess(NULL, steamStartupCommandLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
		LOG(LogLevel::INFO, "Started steam successfully.");
	else
		LOG(LogLevel::FAIL, "Failed to start steam: " << GetLastErrorAsString());
}

bool InitXInputAndFindSteam()
{
	// Query steam installation location.
	DWORD bufferSize = sizeof(g_steamInstallationPath);
	if (RegGetValue(HKEY_CURRENT_USER,
					L"Software\\Valve\\Steam",
					L"SteamExe",
					RRF_RT_REG_SZ,
					NULL,
					g_steamInstallationPath,
					&bufferSize) == ERROR_SUCCESS)
	{
		LOG(LogLevel::INFO, "Found steam installation at: " << ToUtf8(g_steamInstallationPath));
	}
	else
	{
		LOG(LogLevel::FAIL, "Failed to find steam installation!");
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

	return true;
}

void RunXInputMessageLoop(std::atomic<bool>& exited)
{
	// Requires to press the button a bit longer but in return we keep the process more idle.
	// General issue is that there seems to be no way to run XInput event based :(
	const int updateFrequencyHz = 4;
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_IDLE);

	XINPUT_STATE_EX oldState[XUSER_MAX_COUNT];
	ZeroMemory(oldState, sizeof(oldState));

	//HANDLE waitableTimer = CreateWaitableTimerW(nullptr, FALSE, nullptr);

	while(!exited)
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
				StartSteam(true);
			}

			oldState[i] = newState;
		}

		Sleep(1000 / updateFrequencyHz);
		//WaitForSingleObject(waitableTimer, 1000 / updateFrequencyHz);
	}

	//CloseHandle(waitableTimer);
}
