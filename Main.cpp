#include <windows.h>

#include <thread>

#include "resource.h"
#include "Log.h"

#include "SteamMeUp.h"

#ifdef _CONSOLE
int main()
#else
int CALLBACK WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
#endif
{
	if (InitXInputAndFindSteam() && InitTray())
	{
		std::atomic<bool> exited = false;
		std::thread xinputThread(RunXInputMessageLoop, std::ref(exited));

		RunWinApiMessageLoop();

		exited = true;
		xinputThread.join();
	}
	else
		return 1;
	return 0;
}
