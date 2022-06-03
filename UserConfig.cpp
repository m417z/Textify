#include "stdafx.h"
#include "UserConfig.h"

namespace
{
	CPath GetIniFilePath()
	{
		CPath iniFilePath;
		GetModuleFileName(NULL, iniFilePath.m_strPath.GetBuffer(MAX_PATH), MAX_PATH);
		iniFilePath.m_strPath.ReleaseBuffer();
		iniFilePath.RenameExtension(L".ini");
		return iniFilePath;
	}

	// Receives a path with an optional index, e.g.:
	// C:\path\to\awesome.exe,101
	// Removes the index from the string and returns it.
	int ExtractIconIndex(PWSTR pathWithOptionalIndex)
	{
		size_t len = wcslen(pathWithOptionalIndex);
		for(size_t i = len - 1; i > 0; i--)
		{
			WCHAR p = pathWithOptionalIndex[i];
			if(p == ',')
			{
				if(i == len - 1)
					break;

				pathWithOptionalIndex[i] = L'\0';
				return _wtoi(pathWithOptionalIndex + i + 1);
			}

			if(p < L'0' && p > L'9')
				break;
		}

		return 0;
	}

	CPath RelativeToAbsolutePathExpanded(PWSTR relativePath)
	{
		CPath moduleFileNamePath;

		GetModuleFileName(NULL, moduleFileNamePath.m_strPath.GetBuffer(MAX_PATH), MAX_PATH);
		moduleFileNamePath.m_strPath.ReleaseBuffer();
		moduleFileNamePath.RemoveFileSpec();

		CString relativePathExpanded;
		DWORD relativePathExpandedSize = ExpandEnvironmentStrings(relativePath, nullptr, 0);
		ExpandEnvironmentStrings(relativePath, relativePathExpanded.GetBuffer(relativePathExpandedSize), relativePathExpandedSize);
		relativePathExpanded.ReleaseBuffer();

		CPath result;
		result.Combine(moduleFileNamePath, relativePathExpanded);

		return result;
	}
}

UserConfig::UserConfig()
{
	LoadFromIniFile();
}

bool UserConfig::LoadFromIniFile()
{
	CPath iniFilePath = GetIniFilePath();

	WCHAR szBuffer[1025];

	m_mouseHotKey.key = GetPrivateProfileInt(L"mouse", L"key", 0, iniFilePath);
	m_mouseHotKey.ctrl = GetPrivateProfileInt(L"mouse", L"ctrl", 0, iniFilePath);
	m_mouseHotKey.alt = GetPrivateProfileInt(L"mouse", L"alt", 0, iniFilePath);
	m_mouseHotKey.shift = GetPrivateProfileInt(L"mouse", L"shift", 0, iniFilePath);

	m_keybdHotKey.key = GetPrivateProfileInt(L"keyboard", L"key", 0, iniFilePath);
	m_keybdHotKey.ctrl = GetPrivateProfileInt(L"keyboard", L"ctrl", 0, iniFilePath);
	m_keybdHotKey.alt = GetPrivateProfileInt(L"keyboard", L"alt", 0, iniFilePath);
	m_keybdHotKey.shift = GetPrivateProfileInt(L"keyboard", L"shift", 0, iniFilePath);

	m_uiLanguage = static_cast<LANGID>(GetPrivateProfileInt(L"config", L"ui_language", 0, iniFilePath));
	m_checkForUpdates = GetPrivateProfileInt(L"config", L"check_for_updates", 1, iniFilePath);
	m_autoCopySelection = GetPrivateProfileInt(L"config", L"auto_copy_selection", 0, iniFilePath);
	m_hideWndOnStartup = GetPrivateProfileInt(L"config", L"hide_wnd_on_startup", 0, iniFilePath);
	m_hideTrayIcon = GetPrivateProfileInt(L"config", L"hide_tray_icon", 0, iniFilePath);
	GetPrivateProfileString(L"config", L"font_name", L"", m_fontName.GetBuffer(LF_FACESIZE), LF_FACESIZE, iniFilePath);
	m_fontName.ReleaseBuffer();
	m_fontSize = GetPrivateProfileInt(L"config", L"font_size", 0, iniFilePath);
	m_unicodeSpacesToAscii = GetPrivateProfileInt(L"config", L"unicode_spaces_to_ascii", 0, iniFilePath);

	m_textRetrievalMethod = TextRetrievalMethod::default;
	GetPrivateProfileString(L"config", L"text_retrieval_method", L"", szBuffer, ARRAYSIZE(szBuffer), iniFilePath);
	if(_wcsicmp(szBuffer, L"msaa") == 0)
	{
		m_textRetrievalMethod = TextRetrievalMethod::msaa;
	}
	else if(_wcsicmp(szBuffer, L"uia") == 0)
	{
		m_textRetrievalMethod = TextRetrievalMethod::uia;
	}

	m_webButtonsIconSize = GetPrivateProfileInt(L"web_buttons", L"icon_size", 16, iniFilePath);
	m_webButtonsPerRow = GetPrivateProfileInt(L"web_buttons", L"buttons_per_row", 8, iniFilePath);

	for(int i = 1; ; i++)
	{
		CString iniSectionName;
		iniSectionName.Format(L"web_button_%d", i);

		GetPrivateProfileString(iniSectionName, L"command", L"", szBuffer, ARRAYSIZE(szBuffer), iniFilePath);
		if(*szBuffer == L'\0')
			break;

		WebButtonInfo webButtonInfo;
		webButtonInfo.command = szBuffer;

		GetPrivateProfileString(iniSectionName, L"name", L"", szBuffer, ARRAYSIZE(szBuffer), iniFilePath);
		webButtonInfo.name = szBuffer;

		GetPrivateProfileString(iniSectionName, L"icon", L"", szBuffer, ARRAYSIZE(szBuffer), iniFilePath);
		if(*szBuffer != L'\0')
		{
			webButtonInfo.iconIndex = ExtractIconIndex(szBuffer);
			webButtonInfo.iconPath = RelativeToAbsolutePathExpanded(szBuffer);
		}

		GetPrivateProfileString(iniSectionName, L"key", L"", szBuffer, ARRAYSIZE(szBuffer), iniFilePath);
		webButtonInfo.acceleratorKey = *szBuffer;

		webButtonInfo.width = GetPrivateProfileInt(iniSectionName, L"width", 0, iniFilePath);
		webButtonInfo.height = GetPrivateProfileInt(iniSectionName, L"height", 0, iniFilePath);

		m_webButtonInfos.push_back(webButtonInfo);
	}

	for(int i = 1; ; i++)
	{
		CString iniKeyName;
		iniKeyName.Format(L"%d", i);

		GetPrivateProfileString(L"exclude", iniKeyName, L"", szBuffer, ARRAYSIZE(szBuffer), iniFilePath);
		if(*szBuffer == L'\0')
			break;

		m_excludedPrograms.push_back(szBuffer);
	}

	return true;
}

bool UserConfig::SaveToIniFile() const
{
	CPath iniFilePath = GetIniFilePath();
	if(!iniFilePath)
		return false;

	bool succeeded = true;

	CString str;

	str.Format(L"%d", m_mouseHotKey.key);
	if(!WritePrivateProfileString(L"mouse", L"key", str, iniFilePath))
		succeeded = false;

	str.Format(L"%d", m_mouseHotKey.ctrl ? 1 : 0);
	if(!WritePrivateProfileString(L"mouse", L"ctrl", str, iniFilePath))
		succeeded = false;

	str.Format(L"%d", m_mouseHotKey.alt ? 1 : 0);
	if(!WritePrivateProfileString(L"mouse", L"alt", str, iniFilePath))
		succeeded = false;

	str.Format(L"%d", m_mouseHotKey.shift ? 1 : 0);
	if(!WritePrivateProfileString(L"mouse", L"shift", str, iniFilePath))
		succeeded = false;

	// Don't write the other configuration, as there's no GUI for it.

	return succeeded;
}
