#include <Windows.h>
#include "Log.h"
#include "resource.h"

#define TRAY_CALLBACK_MESSAGE (WM_USER + 200)
#define TRAY_CONTEXMENU_EXIT (WM_USER + 400)

namespace
{
	HMENU hPopupMenu;
	HWND hWnd;
	NOTIFYICONDATA notifyIconData;

	LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch (message)
		{
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
			case TRAY_CONTEXMENU_EXIT:
				DestroyWindow(hWnd);
				break;
			}
			break;

		case WM_DESTROY:
			Shell_NotifyIcon(NIM_DELETE, &notifyIconData);
			PostQuitMessage(0);
			break;

		case TRAY_CALLBACK_MESSAGE:
			switch (LOWORD(lParam))
			{
			case WM_RBUTTONUP:
			{
				POINT pt = {};
				if (GetCursorPos(&pt))
				{
					SetForegroundWindow(hWnd);
					TrackPopupMenu(hPopupMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, NULL);
				}
			}
			}
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

	hWnd = CreateWindowExW(0, wcex.lpszClassName, L"SteamMeUp", WS_DISABLED, 0, 0, 0, 0, nullptr, nullptr, wcex.hInstance, nullptr);
	if (!hWnd)
	{
		LOG(LogLevel::FAIL, "Failed to create Window: " << GetLastErrorAsString());
		return false;
	}

	memset(&notifyIconData, 0, sizeof(NOTIFYICONDATA));
	notifyIconData.cbSize = sizeof(NOTIFYICONDATA);
	notifyIconData.hWnd = hWnd;
	notifyIconData.uID = 1;
	notifyIconData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	notifyIconData.uCallbackMessage = TRAY_CALLBACK_MESSAGE;
	notifyIconData.hIcon = (HICON)LoadImage(wcex.hInstance, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
	lstrcpy(notifyIconData.szTip, L"SteamMeUp - Start Steam big picture with the Xbox home button.");
	if (!Shell_NotifyIcon(NIM_ADD, &notifyIconData))
	{
		LOG(LogLevel::FAIL, "Failed to display tray icon: " << GetLastErrorAsString());
		return false;
	}

	hPopupMenu = CreatePopupMenu();
	//AppendMenuW(hPopupMenu, MF_STRING | MF_GRAYED | MF_DISABLED, TRAY_CONTEXMENU_EXIT, L"Some text");
	//AppendMenuW(hPopupMenu, MF_SEPARATOR, 0, nullptr);
	AppendMenuW(hPopupMenu, MF_STRING, TRAY_CONTEXMENU_EXIT, L"Exit");

	return true;
}

void RunWinApiMessageLoop()
{
	MSG msg;
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}