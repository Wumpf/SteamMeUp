#include <windows.h>

#include <thread>
#include <atomic>

#include "resource.h"
#include "Log.h"

bool Startup();
bool InitTray();
void RunWinApiMessageLoop();
void RunXInputMessageLoop(std::atomic<bool>& exited);

#ifdef _CONSOLE
int main()
#else
int CALLBACK WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
#endif
{
	if (Startup() && InitTray())
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
