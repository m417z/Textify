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

	// Load strings.
	CString str;
	str.LoadString(IDS_SETTINGSDLG_TITLE);
	SetWindowText(str);
	str.LoadString(IDS_SETTINGSDLG_OK);
	SetDlgItemText(IDOK, str);
	str.LoadString(IDS_SETTINGSDLG_CANCEL);
	SetDlgItemText(IDCANCEL, str);

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
	CString title;
	title.LoadString(IDS_SETTINGSDLG_WARNING_DISCARD_TITLE);

	CString text;
	text.LoadString(IDS_SETTINGSDLG_WARNING_DISCARD_TEXT);

	if(CButton(GetDlgItem(IDOK)).IsWindowEnabled() &&
		MessageBox(text, title, MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON2) != IDYES)
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
	int bytesToRead = fileSize;

	bool unicode = false;

	if(fileSize >= 2 && fileSize % 2 == 0)
	{
		BYTE twoBytes[2];
		hr = file.Read(twoBytes, 2);
		ATLENSURE_RETURN_VAL(SUCCEEDED(hr), L"");

		// UTF16LE BOM.
		if(twoBytes[0] == 0xFF && twoBytes[1] == 0xFE)
		{
			bytesToRead -= 2; // skip BOM
			unicode = true;
		}
		else
		{
			file.Seek(0, FILE_BEGIN);
		}
	}

	if(bytesToRead == 0)
		return L"";

	CString result;

	if(unicode)
	{
		int charsToRead = bytesToRead / sizeof(WCHAR);
		WCHAR* buffer = result.GetBuffer(charsToRead);
		hr = file.Read(buffer, bytesToRead);
		ATLENSURE_RETURN_VAL(SUCCEEDED(hr), L"");

		result.ReleaseBuffer(charsToRead);
	}
	else
	{
		CStringA ansiContents;
		char* buffer = ansiContents.GetBuffer(bytesToRead);
		hr = file.Read(buffer, bytesToRead);
		ATLENSURE_RETURN_VAL(SUCCEEDED(hr), L"");

		ansiContents.ReleaseBuffer(bytesToRead);

		CA2W unicodeContents(buffer, CP_ACP);
		result = unicodeContents.m_psz;
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

	// UTF16LE BOM.
	hr = file.Write("\xFF\xFE", 2);
	ATLENSURE_RETURN_VAL(SUCCEEDED(hr), false);

	hr = file.Write(newFileContents.GetString(), newFileContents.GetLength() * sizeof(WCHAR));
	ATLENSURE_RETURN_VAL(SUCCEEDED(hr), false);

	return true;
}
