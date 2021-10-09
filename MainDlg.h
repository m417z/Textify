#pragma once

#include "MouseGlobalHook.h"
#include "UserConfig.h"

class CMainDlg : public CDialogImpl<CMainDlg>
{
public:
	enum { IDD = IDD_MAINDLG };

	enum
	{
		UWM_MOUSEHOOKCLICKED = WM_APP,
		UWM_NOTIFYICON,
		UWM_BRING_TO_FRONT,
		UWM_EXIT,
	};

	enum
	{
		RCMENU_SHOW = 101,
		RCMENU_EXIT,
	};

	BEGIN_MSG_MAP_EX(CMainDlg)
		MSG_WM_INITDIALOG(OnInitDialog)
		MSG_WM_DESTROY(OnDestroy)
		MSG_WM_WINDOWPOSCHANGING(OnWindowPosChanging)
		MSG_WM_NOTIFY(OnNotify)
		MSG_WM_HOTKEY(OnHotKey)
		COMMAND_ID_HANDLER_EX(IDOK, OnOK)
		COMMAND_ID_HANDLER_EX(IDCANCEL, OnCancel)
		COMMAND_ID_HANDLER_EX(IDC_SHOW_INI, OnShowIni)
		COMMAND_HANDLER_EX(IDC_CHECK_CTRL, BN_CLICKED, OnConfigChanged)
		COMMAND_HANDLER_EX(IDC_CHECK_ALT, BN_CLICKED, OnConfigChanged)
		COMMAND_HANDLER_EX(IDC_CHECK_SHIFT, BN_CLICKED, OnConfigChanged)
		COMMAND_HANDLER_EX(IDC_COMBO_KEYS, CBN_SELCHANGE, OnConfigChanged)
		MESSAGE_HANDLER_EX(UWM_MOUSEHOOKCLICKED, OnMouseHookClicked)
		MESSAGE_HANDLER_EX(m_uTaskbarCreatedMsg, OnTaskbarCreated)
		MESSAGE_HANDLER_EX(m_uTextfiyMsg, OnCustomTextifyMsg)
		MESSAGE_HANDLER_EX(UWM_NOTIFYICON, OnNotifyIcon)
		MESSAGE_HANDLER_EX(UWM_BRING_TO_FRONT, OnBringToFront)
		MESSAGE_HANDLER_EX(UWM_EXIT, OnExit)
	END_MSG_MAP()

	BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);
	void OnDestroy();
	void OnWindowPosChanging(LPWINDOWPOS lpWndPos);
	LRESULT OnNotify(int idCtrl, LPNMHDR pnmh);
	void OnHotKey(int nHotKeyID, UINT uModifiers, UINT uVirtKey);
	void OnOK(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnShowIni(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnConfigChanged(UINT uNotifyCode, int nID, CWindow wndCtl);
	LRESULT OnMouseHookClicked(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnTaskbarCreated(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnCustomTextifyMsg(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnNotifyIcon(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnBringToFront(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnExit(UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
	void InitMouseAndKeyboardHotKeys();
	void UninitMouseAndKeyboardHotKeys();
	bool RegisterConfiguredKeybdHotKey(const HotKey& keybdHotKey);
	void ConfigToGui();
	void InitNotifyIconData();
	void NotifyIconRightClickMenu();
	bool IsCursorOnExcludedProgram(POINT pt);

	std::unique_ptr<UserConfig> m_config;
	std::unique_ptr<MouseGlobalHook> m_mouseGlobalHook;
	UINT m_uTaskbarCreatedMsg = RegisterWindowMessage(L"TaskbarCreated");
	UINT m_uTextfiyMsg = RegisterWindowMessage(L"Textify");
	NOTIFYICONDATA m_notifyIconData = {};
	bool m_hideDialog;
	bool m_registeredHotKey = false;
};
