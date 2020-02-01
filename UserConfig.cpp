#include "stdafx.h"
#include "UserConfig.h"

UserConfig::UserConfig(bool loadFromIniFile /*= true*/)
{
	if(loadFromIniFile)
		LoadFromIniFile();
}

bool UserConfig::LoadFromIniFile()
{
	CPath iniFilePath = GetIniFilePath();
	if(!iniFilePath)
		return false;

	int mouseKey = GetPrivateProfileInt(L"mouse", L"key", 0, iniFilePath);
	if(mouseKey == VK_LBUTTON || mouseKey == VK_RBUTTON || mouseKey == VK_MBUTTON)
	{
		m_mouseHotKey.key = mouseKey;

		int ctrlKey = GetPrivateProfileInt(L"mouse", L"ctrl", 0, iniFilePath);
		m_mouseHotKey.ctrl = (ctrlKey != 0);

		int altKey = GetPrivateProfileInt(L"mouse", L"alt", 0, iniFilePath);
		m_mouseHotKey.alt = (altKey != 0);

		int shiftKey = GetPrivateProfileInt(L"mouse", L"shift", 0, iniFilePath);
		m_mouseHotKey.shift = (shiftKey != 0);
	}

	int keybdKey = GetPrivateProfileInt(L"keyboard", L"key", 0, iniFilePath);
	if(keybdKey)
	{
		m_keybdHotKey.key = keybdKey;

		int ctrlKey = GetPrivateProfileInt(L"keyboard", L"ctrl", 0, iniFilePath);
		m_keybdHotKey.ctrl = (ctrlKey != 0);

		int altKey = GetPrivateProfileInt(L"keyboard", L"alt", 0, iniFilePath);
		m_keybdHotKey.alt = (altKey != 0);

		int shiftKey = GetPrivateProfileInt(L"keyboard", L"shift", 0, iniFilePath);
		m_keybdHotKey.shift = (shiftKey != 0);
	}

	int autoCopySelection = GetPrivateProfileInt(L"config", L"auto_copy_selection", 0, iniFilePath);
	m_autoCopySelection = autoCopySelection != 0;

	int hideTrayIcon = GetPrivateProfileInt(L"config", L"hide_tray_icon", 0, iniFilePath);
	m_hideTrayIcon = hideTrayIcon != 0;

	int unicodeSpacesToAscii = GetPrivateProfileInt(L"config", L"unicode_spaces_to_ascii", 0, iniFilePath);
	m_unicodeSpacesToAscii = unicodeSpacesToAscii != 0;

	for(int i = 1; i <= m_maxWebButtons; i++)
	{
		CString iniSectionName;
		iniSectionName.Format(L"web_button_%d", i);

		WebButtonInfo webButtonInfo;
		WCHAR szBuffer[1025];

		GetPrivateProfileString(iniSectionName, L"icon", L"", szBuffer, _countof(szBuffer), iniFilePath);
		if(*szBuffer == L'\0')
			break;

		webButtonInfo.iconPath = RelativeToAbsolutePath(szBuffer);

		GetPrivateProfileString(iniSectionName, L"command", L"", szBuffer, _countof(szBuffer), iniFilePath);
		if(*szBuffer == L'\0')
			break;

		webButtonInfo.command = szBuffer;

		webButtonInfo.width = GetPrivateProfileInt(iniSectionName, L"width", 0, iniFilePath);
		webButtonInfo.height = GetPrivateProfileInt(iniSectionName, L"height", 0, iniFilePath);

		m_webButtonInfos.push_back(webButtonInfo);
	}

	return true;
}

bool UserConfig::SaveToIniFile()
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

	for(int i = 1; i <= m_maxWebButtons; i++)
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

CPath UserConfig::GetIniFilePath()
{
	CPath iniFilePath;
	GetModuleFileName(NULL, iniFilePath.m_strPath.GetBuffer(MAX_PATH), MAX_PATH);
	iniFilePath.m_strPath.ReleaseBuffer();
	iniFilePath.RenameExtension(L".ini");
	return iniFilePath;
}

CPath UserConfig::RelativeToAbsolutePath(CPath relativePath)
{
	CPath moduleFileNamePath;

	GetModuleFileName(NULL, moduleFileNamePath.m_strPath.GetBuffer(MAX_PATH), MAX_PATH);
	moduleFileNamePath.m_strPath.ReleaseBuffer();
	moduleFileNamePath.RemoveFileSpec();

	CPath result;
	result.Combine(moduleFileNamePath, relativePath);

	return result;
}

CPath UserConfig::AbsoluteToRelativePath(CPath absolutePath)
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
