#pragma once

BOOL UpdateCheckInit(HWND hWnd, UINT uMsg);
void UpdateCheckCleanup();
BOOL UpdateCheckQueue();
void UpdateCheckAbort();
char* UpdateCheckGetVersion();
DWORD UpdateCheckGetVersionLong();
void UpdateCheckFreeVersion();
void UpdateTaskDialog(HWND hWnd, char* pVersion);
