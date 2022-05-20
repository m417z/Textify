#include "stdafx.h"
#include "resource.h"

#include "MainDlg.h"
#include "TextDlg.h"
#include "SettingsDlg.h"
#include "update.h"
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

	// Load and apply config.
	m_config.emplace();
	ApplyUiLanguage();
	ApplyMouseAndKeyboardHotKeys();
	ConfigToGui();

	// Init and show tray icon.
	InitNotifyIconData();

	if(!m_config->m_hideTrayIcon)
	{
		Shell_NotifyIcon(NIM_ADD, &m_notifyIconData);
	}

	// Start timer to check for updates.
	if(m_config->m_checkForUpdates)
	{
		SetTimer(TIMER_UPDATE_CHECK, 1000 * 10, NULL); // 10sec
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

	if(m_config->m_checkForUpdates)
	{
		KillTimer(TIMER_UPDATE_CHECK);
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
				CString title;
				title.LoadString(IDS_ERROR);

				CString text;
				text.LoadString(IDS_ERROR_OPEN_ADDRESS);
				text += L"\n";
				text += ((PNMLINK)pnmh)->item.szUrl;

				MessageBox(text, title, MB_ICONERROR);
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

		CTextDlg dlgText(*m_config);
		dlgText.DoModal(NULL, reinterpret_cast<LPARAM>(&pt));
	}
}

void CMainDlg::OnTimer(UINT_PTR nIDEvent)
{
	if(nIDEvent == TIMER_UPDATE_CHECK)
	{
		KillTimer(TIMER_UPDATE_CHECK);

		if(UpdateCheckInit(m_hWnd, UWM_UPDATE_CHECKED))
		{
			if(UpdateCheckQueue())
			{
				m_checkingForUpdates = true;
			}
			else
			{
				UpdateCheckCleanup();
			}
		}

		if(!m_checkingForUpdates)
		{
			SetTimer(TIMER_UPDATE_CHECK, 1000 * 60 * 60, NULL); // 1h
		}
	}
}

void CMainDlg::OnOK(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	bool ctrlKey = (CButton(GetDlgItem(IDC_CHECK_CTRL)).GetCheck() != BST_UNCHECKED);
	bool altKey = (CButton(GetDlgItem(IDC_CHECK_ALT)).GetCheck() != BST_UNCHECKED);
	bool shiftKey = (CButton(GetDlgItem(IDC_CHECK_SHIFT)).GetCheck() != BST_UNCHECKED);

	if(!ctrlKey && !altKey && !shiftKey)
	{
		CString title;
		title.LoadString(IDS_MAINDLG_WARNING_MODIFIER_TITLE);

		CString text;
		text.LoadString(IDS_MAINDLG_WARNING_MODIFIER_TEXT);

		if(MessageBox(text, title, MB_ICONWARNING | MB_YESNO) != IDYES)
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
	mouseHotKey.shift = shiftKey;
	mouseHotKey.key = mouseKey;

	m_config->SaveToIniFile();

	ApplyMouseAndKeyboardHotKeys();

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
		LANGID oldUiLanguage = m_config->m_uiLanguage;
		bool oldCheckForUpdates = m_config->m_checkForUpdates;
		bool oldHideTrayIcon = m_config->m_hideTrayIcon;

		m_config.emplace();

		LANGID newUiLanguage = m_config->m_uiLanguage;
		bool newCheckForUpdates = m_config->m_checkForUpdates;
		bool newHideTrayIcon = m_config->m_hideTrayIcon;

		if(oldUiLanguage != newUiLanguage)
		{
			ApplyUiLanguage();
		}

		ApplyMouseAndKeyboardHotKeys();
		ConfigToGui();

		if(newHideTrayIcon != oldHideTrayIcon)
		{
			Shell_NotifyIcon(newHideTrayIcon ? NIM_DELETE : NIM_ADD, &m_notifyIconData);
		}

		if(newCheckForUpdates != oldCheckForUpdates && !m_checkingForUpdates)
		{
			if(newCheckForUpdates)
			{
				SetTimer(TIMER_UPDATE_CHECK, 1000 * 10, NULL); // 10sec
			}
			else
			{
				KillTimer(TIMER_UPDATE_CHECK);
			}
		}
	}
}

void CMainDlg::OnExitButton(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	MyEndDialog();
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

	CTextDlg dlgText(*m_config);
	dlgText.DoModal(NULL, reinterpret_cast<LPARAM>(&pt));

	return 0;
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
		MyEndDialog();
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

LRESULT CMainDlg::OnUpdateChecked(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	UpdateCheckCleanup();

	if(m_config->m_checkForUpdates)
	{
		if(lParam == ERROR_SUCCESS)
		{
			DWORD dwUpdateVersion = UpdateCheckGetVersionLong();
			if(dwUpdateVersion && dwUpdateVersion > VER_FILE_VERSION_LONG)
			{
				HWND hPopup = IsWindowEnabled() ? m_hWnd : GetLastActivePopup();
				UpdateTaskDialog(hPopup, UpdateCheckGetVersion());
			}

			UpdateCheckFreeVersion();

			SetTimer(TIMER_UPDATE_CHECK, 1000 * 60 * 60 * 24, NULL); // 24h
		}
		else
		{
			SetTimer(TIMER_UPDATE_CHECK, 1000 * 60 * 60, NULL); // 1h
		}
	}

	m_checkingForUpdates = false;

	if(m_closeWhenUpdateCheckDone)
	{
		EndDialog(0);
	}

	return 0;
}

LRESULT CMainDlg::OnExit(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	MyEndDialog();
	return 0;
}

void CMainDlg::ApplyUiLanguage()
{
	LANGID uiLanguage = m_config->m_uiLanguage;

	if(!uiLanguage)
	{
		CRegKey regKey;

		DWORD dwError = regKey.Open(HKEY_CURRENT_USER, L"Software\\Textify", KEY_QUERY_VALUE);
		if(dwError == ERROR_SUCCESS)
		{
			DWORD dwLanguage;
			dwError = regKey.QueryDWORDValue(L"language", dwLanguage);
			if(dwError == ERROR_SUCCESS)
			{
				uiLanguage = static_cast<LANGID>(dwLanguage);
			}
		}
	}

	if(uiLanguage)
	{
		OSVERSIONINFO osvi = { sizeof(OSVERSIONINFO) };
		if(GetVersionEx(&osvi) && osvi.dwMajorVersion >= 6)
		{
			::SetThreadUILanguage(uiLanguage);
		}
		else
		{
			::SetThreadLocale(uiLanguage);
		}
	}

	CString str;

	str.LoadString(IDS_MAINDLG_TITLE);
	SetWindowText(str);

	str.LoadString(IDS_MAINDLG_HOMEPAGE);

	CString headerStr;
	headerStr.LoadString(IDS_MAINDLG_HEADER);
	headerStr.Replace(L"%s", VER_FILE_VERSION_WSTR);
	headerStr += L"\n<a href=\"https://ramensoftware.com/textify\">" + str + L"</a>";
	SetDlgItemText(IDC_MAIN_SYSLINK, headerStr);

	str.LoadString(IDS_MAINDLG_MOUSE_SHORTCUT);
	SetDlgItemText(IDC_MOUSE_SHORTCUT, str);

	str.LoadString(IDS_MAINDLG_CTRL);
	SetDlgItemText(IDC_CHECK_CTRL, str);

	str.LoadString(IDS_MAINDLG_ALT);
	SetDlgItemText(IDC_CHECK_ALT, str);

	str.LoadString(IDS_MAINDLG_SHIFT);
	SetDlgItemText(IDC_CHECK_SHIFT, str);

	CComboBox keysComboWnd(GetDlgItem(IDC_COMBO_KEYS));
	int keysComboCurSel = keysComboWnd.GetCurSel();
	keysComboWnd.ResetContent();
	str.LoadString(IDS_MAINDLG_MOUSE_LEFT);
	keysComboWnd.AddString(str);
	str.LoadString(IDS_MAINDLG_MOUSE_RIGHT);
	keysComboWnd.AddString(str);
	str.LoadString(IDS_MAINDLG_MOUSE_MIDDLE);
	keysComboWnd.AddString(str);
	keysComboWnd.SetCurSel(keysComboCurSel);

	str.LoadString(IDS_MAINDLG_APPLY);
	SetDlgItemText(IDOK, str);

	str.LoadString(IDS_MAINDLG_ADVANCED);
	SetDlgItemText(IDC_ADVANCED, str);

	str.LoadString(IDS_MAINDLG_MORE_SETTINGS);
	SetDlgItemText(IDC_SHOW_INI, str);

	str.LoadString(IDS_MAINDLG_EXIT);
	SetDlgItemText(IDC_EXIT, str);
}

void CMainDlg::ApplyMouseAndKeyboardHotKeys()
{
	if(m_config->m_mouseHotKey.key != 0)
	{
		_ATLTRY
		{
			const HotKey & mouseHotKey = m_config->m_mouseHotKey;
			m_mouseGlobalHook.emplace(m_hWnd, UWM_MOUSEHOOKCLICKED,
				mouseHotKey.key, mouseHotKey.ctrl, mouseHotKey.alt, mouseHotKey.shift,
				m_config->m_excludedPrograms);
		}
		_ATLCATCH(e)
		{
			CString str = AtlGetErrorDescription(e);
			MessageBox(
				L"The following error has occurred during the initialization of Textify:\n" + str,
				L"Textify mouse hotkey initialization error", MB_ICONERROR);
		}
	}
	else
	{
		m_mouseGlobalHook.reset();
	}

	if(m_config->m_keybdHotKey.key != 0 && !m_registeredHotKey)
	{
		m_registeredHotKey = RegisterConfiguredKeybdHotKey(m_config->m_keybdHotKey);
		if(!m_registeredHotKey)
		{
			CString str = AtlGetErrorDescription(HRESULT_FROM_WIN32(GetLastError()));
			MessageBox(
				L"The following error has occurred during the initialization of Textify:\n" + str,
				L"Textify keyboard hotkey initialization error", MB_ICONERROR);
		}
	}
	else if(m_config->m_keybdHotKey.key == 0 && m_registeredHotKey)
	{
		::UnregisterHotKey(m_hWnd, 1);
		m_registeredHotKey = false;
	}
}

void CMainDlg::UninitMouseAndKeyboardHotKeys()
{
	if(m_registeredHotKey)
	{
		::UnregisterHotKey(m_hWnd, 1);
		m_registeredHotKey = false;
	}

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

	// Set MOD_NOREPEAT only for Windows 7 and newer.
	OSVERSIONINFO osvi = { sizeof(OSVERSIONINFO) };
	if(GetVersionEx(&osvi))
	{
		if(osvi.dwMajorVersion > 6 || (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion >= 1))
		{
			hotKeyModifiers |= 0x4000 /*MOD_NOREPEAT*/;
		}
	}

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

	default:
		keysComboWnd.SetCurSel(-1);
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

	CString str;

	str.LoadString(IDS_TRAY_TEXTIFY);
	menu.AppendMenu(MF_STRING, RCMENU_SHOW, str);

	menu.AppendMenu(MF_SEPARATOR);

	str.LoadString(IDS_TRAY_EXIT);
	menu.AppendMenu(MF_STRING, RCMENU_EXIT, str);

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
		MyEndDialog();
		break;
	}
}

void CMainDlg::MyEndDialog()
{
	if(m_checkingForUpdates)
	{
		UpdateCheckAbort();
		m_closeWhenUpdateCheckDone = true;
	}
	else
	{
		EndDialog(0);
	}
}
