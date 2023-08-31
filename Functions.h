#pragma once

void UnicodeSpacesToAscii(CString& string);
UINT MyGetDpiForWindow(HWND hWnd);
int ScaleForWindow(HWND hWnd, int value);
int GetSystemMetricsForWindow(HWND hWnd, int nIndex);
BOOL AdjustWindowRectExForWindow(HWND hWnd, LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle);
BOOL UnadjustWindowRectExForWindow(HWND hWnd, LPRECT prc, DWORD dwStyle, BOOL fMenu, DWORD dwExStyle);
BOOL WndAdjustWindowRect(CWindow window, LPRECT prc);
BOOL WndUnadjustWindowRect(CWindow window, LPRECT prc);
BOOL SetClipboardText(const WCHAR* text);
