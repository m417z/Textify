#include "stdafx.h"
#include "WebAppLaunch.h"
#include "URLEncode.h"

namespace
{
	const int g_defaultWidth = 400;
	const int g_defaultHeight = 700;

	bool OpenLink(const WCHAR *pLink, const WCHAR* params = NULL);
	bool OpenWebApp(const WCHAR *urlWithParam, int width, int height, CString* errorMessage);
}

bool WebAppLaunch(const WCHAR* url, const WCHAR* params, const WCHAR* replacement, bool externalBrowser, int width, int height, CString* errorMessage)
{
	CString formattedUrl = url;
	formattedUrl.Replace(L"%s", URLEncoder::Encode(replacement));

	if(externalBrowser)
	{
		CString formattedParams = params;
		formattedParams.Replace(L"%s", replacement);

		if(!OpenLink(formattedUrl, formattedParams.IsEmpty() ? nullptr : formattedParams))
		{
			if(errorMessage)
			{
				*errorMessage =
					L"Could not open link.\n"
					L"\n"
					L"In order to use web links, you need to have a web browser (e.g. Internet Explorer).";
			}

			return false;
		}

		return true;
	}

	if(width <= 0 || height <= 0)
	{
		width = g_defaultWidth;
		height = g_defaultHeight;
	}

	CDC hdc = ::GetDC(NULL);
	if(hdc)
	{
		width = MulDiv(width, hdc.GetDeviceCaps(LOGPIXELSX), 96);
		height = MulDiv(height, hdc.GetDeviceCaps(LOGPIXELSX), 96);
		hdc.DeleteDC();
	}

	return OpenWebApp(formattedUrl, width, height, errorMessage);
}

namespace
{
	bool OpenLink(const WCHAR* link, const WCHAR* params /*= NULL*/)
	{
		HINSTANCE hRet = ::ShellExecute(nullptr, L"open", link, params, NULL, SW_SHOWNORMAL);
		return (int)hRet > 32;
	}

	bool OpenWebApp(const WCHAR *urlWithParam, int width, int height, CString* errorMessage)
	{
		CPoint pt;
		GetCursorPos(&pt);

		CRect rcWindow(pt.x - width / 2, pt.y - height / 2, pt.x + width / 2, pt.y + height / 2);

		HMONITOR hMonitor = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
		MONITORINFO monitorinfo = { sizeof(MONITORINFO) };
		if(GetMonitorInfo(hMonitor, &monitorinfo))
		{
			CRect rcMonitor{ monitorinfo.rcMonitor };

			if(rcWindow.Height() > rcMonitor.Height())
			{
				rcWindow.top = 0;
				rcWindow.bottom = rcMonitor.Height();
			}

			if(rcWindow.Width() > rcMonitor.Width())
			{
				rcWindow.left = 0;
				rcWindow.right = rcMonitor.Width();
			}

			if(rcWindow.left < rcMonitor.left)
			{
				rcWindow.MoveToX(rcMonitor.left);
			}
			else if(rcWindow.right > rcMonitor.right)
			{
				rcWindow.MoveToX(rcMonitor.right - rcWindow.Width());
			}

			if(rcWindow.top < rcMonitor.top)
			{
				rcWindow.MoveToY(rcMonitor.top);
			}
			else if(rcWindow.bottom > rcMonitor.bottom)
			{
				rcWindow.MoveToY(rcMonitor.bottom - rcWindow.Height());
			}
		}

		static BOOL(__stdcall * pShowModalBrowserHost)(const WCHAR*, int, const RECT*) = (decltype(pShowModalBrowserHost))-1;
		if(pShowModalBrowserHost == (decltype(pShowModalBrowserHost))-1)
		{
			HMODULE hWebApp = LoadLibrary(L"WebApp.dll");
			if(hWebApp)
			{
				pShowModalBrowserHost = (decltype(pShowModalBrowserHost))GetProcAddress(hWebApp, "ShowModalBrowserHost");
			}
			else
			{
				pShowModalBrowserHost = nullptr;
			}
		}

		if(!pShowModalBrowserHost)
		{
			if(errorMessage)
				*errorMessage = L"Could not load WebApp.dll";

			return false;
		}

		if(!pShowModalBrowserHost(urlWithParam, SW_SHOWNORMAL, &rcWindow))
		{
			if(errorMessage)
				*errorMessage = L"Could not show the browser applet";

			return false;
		}

		return true;
	}
}
