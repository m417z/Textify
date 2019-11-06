#include "stdafx.h"
#include "resource.h"

#include "MainDlg.h"
#include "TextDlg.h"
#include "SettingsDlg.h"
#include "version.h"

BOOL CMainDlg::OnInitDialog(CWindow wndFocus, LPARAM lInitParam)
{
	m_hideDialog = (lInitParam != 0);

	// Center the dialog on the screen.
	CenterWindow();

	// Set icons.
	HICON hIcon = AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON));
	SetIcon(hIcon, TRUE);
	HICON hIconSmall = AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON));
	SetIcon(hIconSmall, FALSE);

	// Init dialog.
	CString introText;
	GetDlgItemText(IDC_MAIN_SYSLINK, introText);
	introText.Replace(L"%s", VER_FILE_VERSION_WSTR);
	SetDlgItemText(IDC_MAIN_SYSLINK, introText);

	auto keysComboWnd = CComboBox(GetDlgItem(IDC_COMBO_KEYS));
	keysComboWnd.AddString(L"Left mouse button");
	keysComboWnd.AddString(L"Right mouse button");
	keysComboWnd.AddString(L"Middle mouse button");

	// Load and apply config.
	m_config = std::make_unique<UserConfig>();
	InitMouseAndKeyboardHotKeys();
	ConfigToGui();

	// Init and show tray icon.
	if(!m_config->m_hideTrayIcon)
	{
		InitNotifyIconData();
		Shell_NotifyIcon(NIM_ADD, &m_notifyIconData);
	}

	return TRUE;
}

void CMainDlg::OnDestroy()
{
	UninitMouseAndKeyboardHotKeys();

	if(!m_config->m_hideTrayIcon)
	{
		Shell_NotifyIcon(NIM_DELETE, &m_notifyIconData);
	}
}

void CMainDlg::OnWindowPosChanging(LPWINDOWPOS lpWndPos)
{
	if(m_hideDialog && (lpWndPos->flags & SWP_SHOWWINDOW))
	{
		lpWndPos->flags &= ~SWP_SHOWWINDOW;
		m_hideDialog = false;
	}
}

LRESULT CMainDlg::OnNotify(int idCtrl, LPNMHDR pnmh)
{
	switch(pnmh->idFrom)
	{
	case IDC_MAIN_SYSLINK:
		switch(pnmh->code)
		{
		case NM_CLICK:
		case NM_RETURN:
			if((int)ShellExecute(m_hWnd, L"open", ((PNMLINK)pnmh)->item.szUrl, NULL, NULL, SW_SHOWNORMAL) <= 32)
			{
				CString str = L"An error occurred while trying to open the following website address:\n";
				str += ((PNMLINK)pnmh)->item.szUrl;
				MessageBox(str, NULL, MB_ICONHAND);
			}
			break;
		}
		break;
	}

	SetMsgHandled(FALSE);
	return 0;
}

void CMainDlg::OnHotKey(int nHotKeyID, UINT uModifiers, UINT uVirtKey)
{
	if(nHotKeyID == 1)
	{
		CPoint pt;
		GetCursorPos(&pt);

		CTextDlg dlgText(m_config->m_webButtonInfos, m_config->m_autoCopySelection);
		dlgText.DoModal(NULL, reinterpret_cast<LPARAM>(&pt));
	}
}

void CMainDlg::OnOK(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	bool ctrlKey = (CButton(GetDlgItem(IDC_CHECK_CTRL)).GetCheck() != BST_UNCHECKED);
	bool altKey = (CButton(GetDlgItem(IDC_CHECK_ALT)).GetCheck() != BST_UNCHECKED);
	bool shiftKey = (CButton(GetDlgItem(IDC_CHECK_SHIFT)).GetCheck() != BST_UNCHECKED);

	if(!ctrlKey && !altKey && !shiftKey)
	{
		if(MessageBox(L"You are about to set a mouse shortcut without modifier keys.\n"
			L"Are you sure that you'd like to proceed?", L"Please confirm", MB_ICONWARNING | MB_YESNO) != IDYES)
		{
			return;
		}
	}

	int mouseKey;
	auto keysComboWnd = CComboBox(GetDlgItem(IDC_COMBO_KEYS));
	switch(keysComboWnd.GetCurSel())
	{
	case 0:
		mouseKey = VK_LBUTTON;
		break;

	case 1:
		mouseKey = VK_RBUTTON;
		break;

	case 2:
		mouseKey = VK_MBUTTON;
		break;
	}

	HotKey& mouseHotKey = m_config->m_mouseHotKey;
	mouseHotKey.ctrl = ctrlKey;
	mouseHotKey.alt = altKey;
	mouseHotKey.shift= shiftKey;
	mouseHotKey.key = mouseKey;

	m_config->SaveToIniFile();

	m_mouseGlobalHook->SetNewHotkey(mouseKey, ctrlKey, altKey, shiftKey);

	CButton(GetDlgItem(IDOK)).EnableWindow(FALSE);
}

void CMainDlg::OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	ShowWindow(SW_HIDE);
}

void CMainDlg::OnShowIni(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	CSettingsDlg settingsDlg;
	INT_PTR nRet = settingsDlg.DoModal();
	if(nRet == IDOK)
	{
		m_config = std::make_unique<UserConfig>();
		UninitMouseAndKeyboardHotKeys();
		InitMouseAndKeyboardHotKeys();
		ConfigToGui();
	}
}

void CMainDlg::OnConfigChanged(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	CButton(GetDlgItem(IDOK)).EnableWindow();
}

LRESULT CMainDlg::OnMouseHookClicked(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	//CPoint ptEvent{ static_cast<int>(wParam), static_cast<int>(lParam) };

	// Instead of using the mouse hook coordinates, we query the cursor position
	// again, because the hooked coordinates end up incorrect for DPI-unaware
	// applications in high DPI environment.

	CPoint pt;
	GetCursorPos(&pt);

	CTextDlg dlgText(m_config->m_webButtonInfos, m_config->m_autoCopySelection);
	return dlgText.DoModal(NULL, reinterpret_cast<LPARAM>(&pt));
}

LRESULT CMainDlg::OnTaskbarCreated(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if(!m_config->m_hideTrayIcon)
	{
		Shell_NotifyIcon(NIM_ADD, &m_notifyIconData);
	}

	return 0;
}

LRESULT CMainDlg::OnCustomTextifyMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(lParam)
	{
	case 1:
		EndDialog(0);
		break;
	}

	return 0;
}

LRESULT CMainDlg::OnNotifyIcon(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if(wParam == 1)
	{
		switch(lParam)
		{
		case WM_LBUTTONUP:
			ShowWindow(SW_SHOWNORMAL);
			::SetForegroundWindow(GetLastActivePopup());
			break;

		case WM_RBUTTONUP:
			if(IsWindowEnabled())
			{
				::SetForegroundWindow(m_hWnd);
				NotifyIconRightClickMenu();
			}
			else
			{
				ShowWindow(SW_SHOWNORMAL);
				::SetForegroundWindow(GetLastActivePopup());
			}
			break;
		}
	}

	return 0;
}

LRESULT CMainDlg::OnBringToFront(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	ShowWindow(SW_SHOWNORMAL);
	::SetForegroundWindow(GetLastActivePopup());
	return 0;
}

LRESULT CMainDlg::OnExit(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	EndDialog(0);
	return 0;
}

void CMainDlg::InitMouseAndKeyboardHotKeys()
{
	_ATLTRY
	{
		const HotKey& mouseHotKey = m_config->m_mouseHotKey;
		m_mouseGlobalHook = std::make_unique<MouseGlobalHook>(m_hWnd, UWM_MOUSEHOOKCLICKED,
			mouseHotKey.key, mouseHotKey.ctrl, mouseHotKey.alt, mouseHotKey.shift);
	}
	_ATLCATCH(e)
	{
		CString str = AtlGetErrorDescription(e);
		MessageBox(
			L"The following error has occurred during the initialization of Textify:\n" + str,
			L"Textify mouse hotkey initialization error", MB_ICONERROR);
	}

	m_registeredHotKey = RegisterConfiguredKeybdHotKey(m_config->m_keybdHotKey);
	if(!m_registeredHotKey)
	{
		CString str = AtlGetErrorDescription(HRESULT_FROM_WIN32(GetLastError()));
		MessageBox(
			L"The following error has occurred during the initialization of Textify:\n" + str,
			L"Textify keyboard hotkey initialization error", MB_ICONERROR);
	}
}

void CMainDlg::UninitMouseAndKeyboardHotKeys()
{
	if(m_registeredHotKey)
		::UnregisterHotKey(m_hWnd, 1);

	m_mouseGlobalHook.reset();
}

bool CMainDlg::RegisterConfiguredKeybdHotKey(const HotKey& keybdHotKey)
{
	UINT hotKeyModifiers = 0;

	if(keybdHotKey.ctrl)
		hotKeyModifiers |= MOD_CONTROL;

	if(keybdHotKey.alt)
		hotKeyModifiers |= MOD_ALT;

	if(keybdHotKey.shift)
		hotKeyModifiers |= MOD_SHIFT;

	return ::RegisterHotKey(m_hWnd, 1, hotKeyModifiers, keybdHotKey.key) != FALSE;
}

void CMainDlg::ConfigToGui()
{
	const HotKey& mouseHotKey = m_config->m_mouseHotKey;

	CButton(GetDlgItem(IDC_CHECK_CTRL)).SetCheck(mouseHotKey.ctrl ? BST_CHECKED : BST_UNCHECKED);
	CButton(GetDlgItem(IDC_CHECK_ALT)).SetCheck(mouseHotKey.alt ? BST_CHECKED : BST_UNCHECKED);
	CButton(GetDlgItem(IDC_CHECK_SHIFT)).SetCheck(mouseHotKey.shift ? BST_CHECKED : BST_UNCHECKED);

	auto keysComboWnd = CComboBox(GetDlgItem(IDC_COMBO_KEYS));
	switch(mouseHotKey.key)
	{
	case VK_LBUTTON:
		keysComboWnd.SetCurSel(0);
		break;

	case VK_RBUTTON:
		keysComboWnd.SetCurSel(1);
		break;

	case VK_MBUTTON:
		keysComboWnd.SetCurSel(2);
		break;
	}

	CButton(GetDlgItem(IDOK)).EnableWindow(FALSE);
}

void CMainDlg::InitNotifyIconData()
{
	m_notifyIconData.cbSize = NOTIFYICONDATA_V1_SIZE;
	m_notifyIconData.hWnd = m_hWnd;
	m_notifyIconData.uID = 1;
	m_notifyIconData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	m_notifyIconData.uCallbackMessage = UWM_NOTIFYICON;
	m_notifyIconData.hIcon = AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON));

	CString sWindowText;
	GetWindowText(sWindowText);
	wcscpy_s(m_notifyIconData.szTip, sWindowText);
}

void CMainDlg::NotifyIconRightClickMenu()
{
	CMenu menu;
	menu.CreatePopupMenu();

	menu.AppendMenu(MF_STRING, RCMENU_SHOW, L"Textify");
	menu.AppendMenu(MF_SEPARATOR);
	menu.AppendMenu(MF_STRING, RCMENU_EXIT, L"Exit");

	CPoint point;
	GetCursorPos(&point);
	int nCmd = menu.TrackPopupMenu(TPM_RIGHTBUTTON | TPM_RETURNCMD, point.x, point.y, m_hWnd);
	switch(nCmd)
	{
	case RCMENU_SHOW:
		ShowWindow(SW_SHOWNORMAL);
		::SetForegroundWindow(GetLastActivePopup());
		break;

	case RCMENU_EXIT:
		EndDialog(0);
		break;
	}
}
