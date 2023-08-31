#include "stdafx.h"
#include "Functions.h"

void UnicodeSpacesToAscii(CString& string)
{
	// Based on:
	// https://stackoverflow.com/a/21797208
	// https://www.compart.com/en/unicode/category/Zs
	// https://www.compart.com/en/unicode/category/Cf
	std::unordered_set<WCHAR> unicodeSpaces = {
		L'\u00A0', // No-Break Space (NBSP)
		//L'\u1680', // Ogham Space Mark
		L'\u2000', // En Quad
		L'\u2001', // Em Quad
		L'\u2002', // En Space
		L'\u2003', // Em Space
		L'\u2004', // Three-Per-Em Space
		L'\u2005', // Four-Per-Em Space
		L'\u2006', // Six-Per-Em Space
		L'\u2007', // Figure Space
		L'\u2008', // Punctuation Space
		L'\u2009', // Thin Space
		L'\u200A', // Hair Space
		L'\u202F', // Narrow No-Break Space (NNBSP)
		L'\u205F', // Medium Mathematical Space (MMSP)
		L'\u3000'  // Ideographic Space
	};
	std::unordered_set<WCHAR> unicodeInvisible = {
		L'\u00AD', // Soft Hyphen (SHY)
		//L'\u0600', // Arabic Number Sign
		//L'\u0601', // Arabic Sign Sanah
		//L'\u0602', // Arabic Footnote Marker
		//L'\u0603', // Arabic Sign Safha
		//L'\u0604', // Arabic Sign Samvat
		//L'\u0605', // Arabic Number Mark Above
		//L'\u061C', // Arabic Letter Mark (ALM)
		//L'\u06DD', // Arabic End of Ayah
		//L'\u070F', // Syriac Abbreviation Mark
		//L'\u08E2', // Arabic Disputed End of Ayah
		//L'\u180E', // Mongolian Vowel Separator (MVS)
		L'\u200B', // Zero Width Space (ZWSP)
		L'\u200C', // Zero Width Non-Joiner (ZWNJ)
		L'\u200D', // Zero Width Joiner (ZWJ)
		L'\u200E', // Left-to-Right Mark (LRM)
		L'\u200F', // Right-to-Left Mark (RLM)
		L'\u202A', // Left-to-Right Embedding (LRE)
		L'\u202B', // Right-to-Left Embedding (RLE)
		L'\u202C', // Pop Directional Formatting (PDF)
		L'\u202D', // Left-to-Right Override (LRO)
		L'\u202E', // Right-to-Left Override (RLO)
		L'\u2060', // Word Joiner (WJ)
		L'\u2061', // Function Application
		L'\u2062', // Invisible Times
		L'\u2063', // Invisible Separator
		L'\u2064', // Invisible Plus
		L'\u2066', // Left-to-Right Isolate (LRI)
		L'\u2067', // Right-to-Left Isolate (RLI)
		L'\u2068', // First Strong Isolate (FSI)
		L'\u2069', // Pop Directional Isolate (PDI)
		L'\u206A', // Inhibit Symmetric Swapping
		L'\u206B', // Activate Symmetric Swapping
		//L'\u206C', // Inhibit Arabic Form Shaping
		//L'\u206D', // Activate Arabic Form Shaping
		L'\u206E', // National Digit Shapes
		L'\u206F', // Nominal Digit Shapes
		L'\uFEFF', // Zero Width No-Break Space (BOM, ZWNBSP)
		L'\uFFF9', // Interlinear Annotation Anchor
		L'\uFFFA', // Interlinear Annotation Separator
		L'\uFFFB'  // Interlinear Annotation Terminator
	};

	int i = 0;
	while(i < string.GetLength())
	{
		if(unicodeInvisible.find(string[i]) != unicodeInvisible.end())
		{
			string.Delete(i, 1);
			continue;
		}

		if(unicodeSpaces.find(string[i]) != unicodeSpaces.end())
		{
			string.SetAt(i, L' ');
		}

		i++;
	}
}

UINT MyGetDpiForWindow(HWND hWnd)
{
	using GetDpiForWindow_t = UINT(WINAPI*)(HWND hwnd);
	static GetDpiForWindow_t pGetDpiForWindow = []()
	{
		HMODULE hUser32 = GetModuleHandle(L"user32.dll");
		if(hUser32)
		{
			return (GetDpiForWindow_t)GetProcAddress(hUser32, "GetDpiForWindow");
		}

		return (GetDpiForWindow_t)nullptr;
	}();

	int iDpi = 96;
	if(pGetDpiForWindow)
	{
		iDpi = pGetDpiForWindow(hWnd);
	}
	else
	{
		CDC hdc = ::GetDC(NULL);
		if(hdc)
		{
			iDpi = hdc.GetDeviceCaps(LOGPIXELSX);
		}
	}

	return iDpi;
}

int ScaleForWindow(HWND hWnd, int value)
{
	return MulDiv(value, MyGetDpiForWindow(hWnd), 96);
}

int GetSystemMetricsForWindow(HWND hWnd, int nIndex)
{
	using GetSystemMetricsForDpi_t = int(WINAPI*)(int nIndex, UINT dpi);
	static GetSystemMetricsForDpi_t pGetSystemMetricsForDpi = []()
	{
		HMODULE hUser32 = GetModuleHandle(L"user32.dll");
		if(hUser32)
		{
			return (GetSystemMetricsForDpi_t)GetProcAddress(hUser32, "GetSystemMetricsForDpi");
		}

		return (GetSystemMetricsForDpi_t)nullptr;
	}();

	if(pGetSystemMetricsForDpi)
	{
		return pGetSystemMetricsForDpi(nIndex, MyGetDpiForWindow(hWnd));
	}
	else
	{
		return GetSystemMetrics(nIndex);
	}
}

BOOL AdjustWindowRectExForWindow(HWND hWnd, LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle)
{
	using AdjustWindowRectExForDpi_t = BOOL(WINAPI*)(LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle, UINT dpi);
	static AdjustWindowRectExForDpi_t pAdjustWindowRectExForDpi = []()
	{
		HMODULE hUser32 = GetModuleHandle(L"user32.dll");
		if(hUser32)
		{
			return (AdjustWindowRectExForDpi_t)GetProcAddress(hUser32, "AdjustWindowRectExForDpi");
		}

		return (AdjustWindowRectExForDpi_t)nullptr;
	}();

	if(pAdjustWindowRectExForDpi)
	{
		return pAdjustWindowRectExForDpi(lpRect, dwStyle, bMenu, dwExStyle, MyGetDpiForWindow(hWnd));
	}
	else
	{
		return AdjustWindowRectEx(lpRect, dwStyle, bMenu, dwExStyle);
	}
}

BOOL UnadjustWindowRectExForWindow(HWND hWnd, LPRECT prc, DWORD dwStyle, BOOL fMenu, DWORD dwExStyle)
{
	RECT rc;
	SetRectEmpty(&rc);

	BOOL fRc = AdjustWindowRectExForWindow(hWnd, &rc, dwStyle, fMenu, dwExStyle);
	if(fRc)
	{
		prc->left -= rc.left;
		prc->top -= rc.top;
		prc->right -= rc.right;
		prc->bottom -= rc.bottom;
	}

	return fRc;
}

BOOL WndAdjustWindowRect(CWindow window, LPRECT prc)
{
	DWORD dwStyle = window.GetStyle();
	DWORD dwExStyle = window.GetExStyle();
	BOOL bMenu = (!(dwStyle & WS_CHILD) && (window.GetMenu() != NULL));

	return AdjustWindowRectExForWindow(window, prc, dwStyle, bMenu, dwExStyle);
}

BOOL WndUnadjustWindowRect(CWindow window, LPRECT prc)
{
	DWORD dwStyle = window.GetStyle();
	DWORD dwExStyle = window.GetExStyle();
	BOOL bMenu = (!(dwStyle & WS_CHILD) && (window.GetMenu() != NULL));

	return UnadjustWindowRectExForWindow(window, prc, dwStyle, bMenu, dwExStyle);
}

BOOL SetClipboardText(const WCHAR* text)
{
	if(!OpenClipboard(NULL))
		return FALSE;

	BOOL bSucceeded = FALSE;

	size_t size = sizeof(WCHAR) * (wcslen(text) + 1);
	HANDLE handle = GlobalAlloc(GHND, size);
	if(handle)
	{
		WCHAR* clipboardText = (WCHAR*)GlobalLock(handle);
		if(clipboardText)
		{
			memcpy(clipboardText, text, size);
			GlobalUnlock(handle);
			bSucceeded = EmptyClipboard() &&
				SetClipboardData(CF_UNICODETEXT, handle);
		}

		if(!bSucceeded)
			GlobalFree(handle);
	}

	CloseClipboard();
	return bSucceeded;
}
