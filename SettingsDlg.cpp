#include "stdafx.h"
#include "SettingsDlg.h"

BOOL CSettingsDlg::OnInitDialog(CWindow wndFocus, LPARAM lInitParam)
{
	// Center the dialog on the screen.
	CenterWindow();

	// Init resizing.
	DlgResize_Init();

	// Init font.
	CStatic configTextCtrl(GetDlgItem(IDC_CONFIG_TEXT));
	CFontHandle font(configTextCtrl.GetFont());
	CLogFont fontAttributes(font);
	wcscpy_s(fontAttributes.lfFaceName, L"Consolas");
	m_fontConfigText = fontAttributes.CreateFontIndirect();
	configTextCtrl.SetFont(m_fontConfigText);

	// Load ini file.
	CEdit configText = CEdit(GetDlgItem(IDC_CONFIG_TEXT));
	configText.SetWindowText(LoadIniFile());
	configText.SetLimitText(0);

	// Other initialization stuff.
	m_wndEdit.SubclassWindow(configText);

	CButton(GetDlgItem(IDOK)).EnableWindow(FALSE);

	return TRUE;
}

void CSettingsDlg::OnDestroy()
{
}

void CSettingsDlg::OnConfigTextChange(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	CButton(GetDlgItem(IDOK)).EnableWindow(TRUE);
}

void CSettingsDlg::OnOK(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	CEdit configText = CEdit(GetDlgItem(IDC_CONFIG_TEXT));
	CString str;
	configText.GetWindowText(str);
	SaveIniFile(str);

	EndDialog(nID);
}

void CSettingsDlg::OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	if(CButton(GetDlgItem(IDOK)).IsWindowEnabled() &&
		MessageBox(L"放弃未保存的更改？", L"请确认", MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON2) != IDYES)
	{
		return;
	}

	EndDialog(nID);
}

UINT CSettingsDlg::OnGetDlgCode(LPMSG lpMsg)
{
	// Prevent selecting all text when focused.
	// https://devblogs.microsoft.com/oldnewthing/20031114-00/?p=41823
	return m_wndEdit.DefWindowProc() & ~DLGC_HASSETSEL;
}

// Private functions

CPath CSettingsDlg::GetIniFilePath()
{
	CPath iniFilePath;
	GetModuleFileName(NULL, iniFilePath.m_strPath.GetBuffer(MAX_PATH), MAX_PATH);
	iniFilePath.m_strPath.ReleaseBuffer();
	iniFilePath.RenameExtension(L".ini");
	return iniFilePath;
}

CString CSettingsDlg::LoadIniFile()
{
	HRESULT hr;

	CPath iniFilePath = GetIniFilePath();

	CAtlFile file;
	hr = file.Create(iniFilePath, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING);
	ATLENSURE_RETURN_VAL(SUCCEEDED(hr), L"");

	ULONGLONG fileSizeUlonglong;
	hr = file.GetSize(fileSizeUlonglong);
	ATLENSURE_RETURN_VAL(SUCCEEDED(hr), L"");

	int fileSize = (int)fileSizeUlonglong;

	CStringA utf8Contents;
	CHAR* utf8ContentsBuffer = utf8Contents.GetBuffer(fileSize + 1);
	hr = file.Read(utf8ContentsBuffer, fileSize);
	ATLENSURE_RETURN_VAL(SUCCEEDED(hr), L"");

	utf8ContentsBuffer[fileSize] = '\0';
	utf8Contents.ReleaseBuffer(fileSize + 1);

	CA2W unicodeContents(utf8ContentsBuffer, CP_ACP);// Force ANSI to fix UTF8 Chinese messy codes in UserConfig.cpp, then the ini file should also be ANSI.
	//CA2W unicodeContents(utf8ContentsBuffer, CP_UTF8);
	CString result = unicodeContents.m_psz;

	const WCHAR* prefixToRemove = L"; 编辑此配置文件后，\r\n; Textify 需要重新启动来应用更改。\r\n";
	size_t prefixToRemoveLength = wcslen(prefixToRemove);
	if(result.Left(prefixToRemoveLength) == prefixToRemove)
	{
		result = result.Right(result.GetLength() - prefixToRemoveLength);
		result.TrimLeft();
	}

	return result;
}

bool CSettingsDlg::SaveIniFile(CString newFileContents)
{
	HRESULT hr;

	CPath iniFilePath = GetIniFilePath();

	CAtlFile file;
	hr = file.Create(iniFilePath, GENERIC_WRITE, FILE_SHARE_READ, CREATE_ALWAYS);
	ATLENSURE_RETURN_VAL(SUCCEEDED(hr), false);

	CW2A utf8Contents(newFileContents, CP_UTF8);

	hr = file.Write(utf8Contents.m_psz, strlen(utf8Contents.m_psz));
	ATLENSURE_RETURN_VAL(SUCCEEDED(hr), false);

	return true;
}
