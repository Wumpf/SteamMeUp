#pragma once

#include <atomic>

void StartSteam(bool bigPicture);
bool InitXInputAndFindSteam();
bool InitTray();
void RunWinApiMessageLoop();
void RunXInputMessageLoop(std::atomic<bool>& exited);

extern wchar_t g_steamInstallationPath[260];
