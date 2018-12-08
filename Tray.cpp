#include <Windows.h>
#include "Log.h"
#include "resource.h"

namespace
{
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
}

bool InitTray()
{
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
}