#pragma once

void UnicodeSpacesToAscii(CString& string);
UINT GetDpiForWindowWithFallback(HWND hWnd);
int ScaleForWindow(HWND hWnd, int value);
int GetSystemMetricsForDpiWithFallback(int nIndex, UINT dpi);
int GetSystemMetricsForWindow(HWND hWnd, int nIndex);
HICON LoadIconWithScaleDownWithFallback(HINSTANCE hInst, PCWSTR pszName, int cx, int cy);
BOOL AdjustWindowRectExForWindow(HWND hWnd, LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle);
BOOL UnadjustWindowRectExForWindow(HWND hWnd, LPRECT prc, DWORD dwStyle, BOOL fMenu, DWORD dwExStyle);
BOOL WndAdjustWindowRect(CWindow window, LPRECT prc);
BOOL WndUnadjustWindowRect(CWindow window, LPRECT prc);
BOOL SetClipboardText(const WCHAR* text);
