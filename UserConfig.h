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
	int iconIndex;
	WCHAR acceleratorKey;
	int width;
	int height;
};

enum class TextRetrievalMethod
{
	default,
	msaa,
	uia
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
	TextRetrievalMethod m_textRetrievalMethod;
	int m_webButtonsIconSize;
	int m_webButtonsPerRow;
	std::vector<WebButtonInfo> m_webButtonInfos;
	std::vector<CString> m_excludedPrograms;
};
