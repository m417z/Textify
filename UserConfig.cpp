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

	CPath RelativeToAbsolutePath(CPath relativePath)
	{
		CPath moduleFileNamePath;

		GetModuleFileName(NULL, moduleFileNamePath.m_strPath.GetBuffer(MAX_PATH), MAX_PATH);
		moduleFileNamePath.m_strPath.ReleaseBuffer();
		moduleFileNamePath.RemoveFileSpec();

		CPath result;
		result.Combine(moduleFileNamePath, relativePath);

		return result;
	}

	CPath AbsoluteToRelativePath(CPath absolutePath)
	{
		CPath moduleFileNamePath;

		GetModuleFileName(NULL, moduleFileNamePath.m_strPath.GetBuffer(MAX_PATH), MAX_PATH);
		moduleFileNamePath.m_strPath.ReleaseBuffer();
		moduleFileNamePath.RemoveFileSpec();

		CPath result;
		result.RelativePathTo(moduleFileNamePath, FILE_ATTRIBUTE_DIRECTORY, absolutePath, FILE_ATTRIBUTE_NORMAL);

		if(result.m_strPath.GetLength() > 2 &&
			result.m_strPath[0] == L'.' &&
			result.m_strPath[1] == L'\\')
		{
			result.m_strPath = result.m_strPath.Right(result.m_strPath.GetLength() - 2);
		}

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
	m_hideTrayIcon = GetPrivateProfileInt(L"config", L"hide_tray_icon", 0, iniFilePath);
	m_unicodeSpacesToAscii = GetPrivateProfileInt(L"config", L"unicode_spaces_to_ascii", 0, iniFilePath);

	m_webButtonsIconSize = GetPrivateProfileInt(L"web_buttons", L"icon_size", 16, iniFilePath);
	m_webButtonsPerRow = GetPrivateProfileInt(L"web_buttons", L"buttons_per_row", 8, iniFilePath);

	for(int i = 1; ; i++)
	{
		CString iniSectionName;
		iniSectionName.Format(L"web_button_%d", i);

		WCHAR szBuffer[1025];

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
			webButtonInfo.iconPath = RelativeToAbsolutePath(szBuffer);
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

		WCHAR szBuffer[1025];

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

	/*str.Format(L"%d", m_keybdHotKey.key);
	if(!WritePrivateProfileString(L"keyboard", L"key", str, iniFilePath))
		succeeded = false;

	str.Format(L"%d", m_keybdHotKey.ctrl ? 1 : 0);
	if(!WritePrivateProfileString(L"keyboard", L"ctrl", str, iniFilePath))
		succeeded = false;

	str.Format(L"%d", m_keybdHotKey.alt ? 1 : 0);
	if(!WritePrivateProfileString(L"keyboard", L"alt", str, iniFilePath))
		succeeded = false;

	str.Format(L"%d", m_keybdHotKey.shift ? 1 : 0);
	if(!WritePrivateProfileString(L"keyboard", L"shift", str, iniFilePath))
		succeeded = false;

	for(int i = 1; ; i++)
	{
		CString iniSectionName;
		iniSectionName.Format(L"web_button_%d", i);

		if(static_cast<int>(m_webButtonInfos.size()) < i)
		{
			if(!WritePrivateProfileString(iniSectionName, nullptr, nullptr, iniFilePath))
				succeeded = false;

			continue;
		}

		WebButtonInfo& webButtonInfo = m_webButtonInfos[i - 1];

		CPath relativeIconPath = AbsoluteToRelativePath(webButtonInfo.iconPath);
		if(!WritePrivateProfileString(iniSectionName, L"icon", relativeIconPath.m_strPath, iniFilePath))
			succeeded = false;

		if(!WritePrivateProfileString(iniSectionName, L"url", webButtonInfo.url, iniFilePath))
			succeeded = false;

		str.Format(L"%d", webButtonInfo.externalBrowser ? 1 : 0);
		if(!WritePrivateProfileString(iniSectionName, L"external_browser", str, iniFilePath))
			succeeded = false;

		str.Format(L"%d", webButtonInfo.width);
		if(!WritePrivateProfileString(iniSectionName, L"width", str, iniFilePath))
			succeeded = false;

		str.Format(L"%d", webButtonInfo.height);
		if(!WritePrivateProfileString(iniSectionName, L"height", str, iniFilePath))
			succeeded = false;
	}*/

	return succeeded;
}
