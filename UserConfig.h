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
	CPath iconPath;
	CString url;
	CString params;
	bool externalBrowser;
	int width;
	int height;
};

class UserConfig
{
public:
	UserConfig(bool loadFromIniFile = true);

	bool LoadFromIniFile();
	bool SaveToIniFile();

	HotKey m_mouseHotKey = HotKey{ VK_MBUTTON, false, false, true };
	HotKey m_keybdHotKey = HotKey{ 'T', true, true, true };
	bool m_autoCopySelection = false;
	bool m_hideTrayIcon = false;
	std::vector<WebButtonInfo> m_webButtonInfos;

	const int m_maxWebButtons = 20;

private:
	CPath GetIniFilePath();
	CPath RelativeToAbsolutePath(CPath relativePath);
	CPath AbsoluteToRelativePath(CPath absolutePath);
};
