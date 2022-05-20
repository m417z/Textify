#pragma once

struct HotKey
{
	int key;
	bool ctrl;
	bool alt;
	bool shift;
};

struct WebButtonInfo
{
	CString command;
	CString name;
	CPath iconPath;
	WCHAR acceleratorKey;
	int width;
	int height;
};

class UserConfig
{
public:
	UserConfig();

	bool LoadFromIniFile();
	bool SaveToIniFile() const;

	HotKey m_mouseHotKey;
	HotKey m_keybdHotKey;
	LANGID m_uiLanguage;
	bool m_checkForUpdates;
	bool m_autoCopySelection;
	bool m_hideTrayIcon;
	bool m_unicodeSpacesToAscii;
	int m_webButtonsIconSize;
	int m_webButtonsPerRow;
	std::vector<WebButtonInfo> m_webButtonInfos;
	std::vector<CString> m_excludedPrograms;
};
