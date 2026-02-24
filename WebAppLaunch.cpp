#include "stdafx.h"
#include "WebAppLaunch.h"
#include "URLEncode.h"

namespace
{
	enum class PopupBrowserType
	{
		None,
		Chrome,
		Edge,
		Brave,
		Firefox
	};

	PopupBrowserType DetectPopupBrowserType();
	bool ExecuteCommand(const WCHAR* command);
}

bool CommandLaunch(const WCHAR* command, const WCHAR* replacement, int width, int height)
{
	PopupBrowserType popupBrowserType;
	CString formattedCommand;

	if(wcsncmp(L"popup-web!", command, sizeof("popup-web!") - 1) == 0)
	{
		formattedCommand = command + (sizeof("popup-web!") - 1);
		popupBrowserType = DetectPopupBrowserType();
	}
	else if(wcsncmp(L"popup-chrome!", command, sizeof("popup-chrome!") - 1) == 0)
	{
		formattedCommand = command + (sizeof("popup-chrome!") - 1);
		popupBrowserType = PopupBrowserType::Chrome;
	}
	else if(wcsncmp(L"popup-edge!", command, sizeof("popup-edge!") - 1) == 0)
	{
		formattedCommand = command + (sizeof("popup-edge!") - 1);
		popupBrowserType = PopupBrowserType::Edge;
	}
	else if(wcsncmp(L"popup-firefox!", command, sizeof("popup-firefox!") - 1) == 0)
	{
		formattedCommand = command + (sizeof("popup-firefox!") - 1);
		popupBrowserType = PopupBrowserType::Firefox;
	}
	else
	{
		formattedCommand = command;
		popupBrowserType = PopupBrowserType::None;
	}

	CString replacementWithoutQuotes = replacement;
	replacementWithoutQuotes.Replace(L"\"", L"");

	formattedCommand.Replace(L"%s", URLEncoder::Encode(replacement));
	formattedCommand.Replace(L"%ds", URLEncoder::Encode(URLEncoder::Encode(replacement)));
	formattedCommand.Replace(L"%cs", replacementWithoutQuotes);
	formattedCommand.Replace(L"%rs", replacement);

	if(popupBrowserType == PopupBrowserType::Chrome)
	{
		// Prevent argument injection. We can escape this properly,
		// but since it shouldn't be a part of a URL anyway, we just strip it.
		formattedCommand.Replace(L"\"", L"");
		CString chromeCommand = L"chrome.exe --app=\"" + formattedCommand + L"\"";
		return ExecuteCommand(chromeCommand);
	}

	if(popupBrowserType == PopupBrowserType::Edge)
	{
		formattedCommand.Replace(L"\"", L"");
		CString edgeCommand = L"msedge.exe --app=\"" + formattedCommand + L"\"";
		return ExecuteCommand(edgeCommand);
	}

	if(popupBrowserType == PopupBrowserType::Brave)
	{
		formattedCommand.Replace(L"\"", L"");
		CString braveCommand = L"brave.exe --app=\"" + formattedCommand + L"\"";
		return ExecuteCommand(braveCommand);
	}

	if(popupBrowserType == PopupBrowserType::Firefox)
	{
		formattedCommand.Replace(L"\"", L"");
		CString firefoxCommand;
		firefoxCommand.Format(L"firefox.exe -width %d -height %d -new-window \"%s\"",
			width, height, formattedCommand.GetString());
		return ExecuteCommand(firefoxCommand);
	}

	return ExecuteCommand(formattedCommand);
}

namespace
{
	PopupBrowserType DetectPopupBrowserType()
	{
		DWORD dwError;
		CRegKey regKey;

		dwError = regKey.Open(HKEY_CURRENT_USER,
			L"Software\\Microsoft\\Windows\\Shell\\Associations\\UrlAssociations\\http\\UserChoice",
			KEY_QUERY_VALUE | KEY_WOW64_64KEY);
		if(dwError != ERROR_SUCCESS)
			return PopupBrowserType::None;

		WCHAR szProgId[64];
		ULONG nChars = ARRAYSIZE(szProgId);
		dwError = regKey.QueryStringValue(L"ProgId", szProgId, &nChars);
		if(dwError != ERROR_SUCCESS)
			return PopupBrowserType::None;

		// ProgId string reference:
		// https://superuser.com/a/571854

		if(_wcsicmp(szProgId, L"ChromeHTML") == 0 ||
			_wcsicmp(szProgId, L"ChromeBHTML") == 0 ||
			_wcsicmp(szProgId, L"ChromeDHTML") == 0)
		{
			return PopupBrowserType::Chrome;
		}

		if(_wcsicmp(szProgId, L"MSEdgeHTM") == 0 ||
			_wcsicmp(szProgId, L"MSEdgeBHTML") == 0 ||
			_wcsicmp(szProgId, L"MSEdgeDHTML") == 0)
		{
			return PopupBrowserType::Edge;
		}

		if(_wcsicmp(szProgId, L"BraveHTML") == 0 ||
			_wcsicmp(szProgId, L"BraveBHTML") == 0 ||
			_wcsicmp(szProgId, L"BraveDHTML") == 0)
		{
			return PopupBrowserType::Brave;
		}

		if(_wcsicmp(szProgId, L"FirefoxURL") == 0 ||
			_wcsnicmp(szProgId, L"FirefoxURL-", sizeof("FirefoxURL-") - 1) == 0)
		{
			return PopupBrowserType::Firefox;
		}

		return PopupBrowserType::None;
	}

	bool ExecuteCommand(const WCHAR* command)
	{
		CString commandWithoutArgs = command;
		commandWithoutArgs.Trim();

		CString args;

		if(!UrlIsW(commandWithoutArgs, URLIS_APPLIABLE))
		{
			// Get args from command line. See also functions
			// ShellExecCmdLine, SplitParams, at:
			// https://github.com/reactos/reactos/blob/master/dll/win32/shell32/shlexec.cpp

			args = PathGetArgs(commandWithoutArgs);
			if(!args.IsEmpty())
			{
				commandWithoutArgs = commandWithoutArgs.Left(
					commandWithoutArgs.GetLength() - args.GetLength());
				commandWithoutArgs.TrimRight();
			}

			if(commandWithoutArgs.GetAt(0) == L'\"' &&
				commandWithoutArgs.GetAt(commandWithoutArgs.GetLength() - 1) == L'\"')
			{
				commandWithoutArgs = commandWithoutArgs.Mid(1, commandWithoutArgs.GetLength() - 2);
			}
		}

		HINSTANCE hRet = ::ShellExecute(nullptr, nullptr,
			commandWithoutArgs, args.IsEmpty() ? nullptr : args,
			nullptr, SW_SHOWNORMAL);
		return (int)(DWORD_PTR)hRet > 32;
	}
}
