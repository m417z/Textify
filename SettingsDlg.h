#pragma once

#include "resource.h"

class CSettingsDlg : public CDialogImpl<CSettingsDlg>,
	public CDialogResize<CSettingsDlg>
{
public:
	enum { IDD = IDD_SETTINGSDLG };

	BEGIN_DLGRESIZE_MAP(CSettingsDlg)
		DLGRESIZE_CONTROL(IDC_CONFIG_TEXT, DLSZ_SIZE_X | DLSZ_SIZE_Y)
		DLGRESIZE_CONTROL(IDOK, DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDCANCEL, DLSZ_MOVE_X | DLSZ_MOVE_Y)
	END_DLGRESIZE_MAP()

	BEGIN_MSG_MAP_EX(CSettingsDlg)
		CHAIN_MSG_MAP(CDialogResize<CSettingsDlg>)
		MSG_WM_INITDIALOG(OnInitDialog)
		MSG_WM_DESTROY(OnDestroy)
		COMMAND_HANDLER_EX(IDC_CONFIG_TEXT, EN_CHANGE, OnConfigTextChange)
		COMMAND_ID_HANDLER_EX(IDOK, OnOK)
		COMMAND_ID_HANDLER_EX(IDCANCEL, OnCancel)
	ALT_MSG_MAP(1)
		MSG_WM_GETDLGCODE(OnGetDlgCode)
	END_MSG_MAP()

	CSettingsDlg() : m_wndEdit(this, 1) {}

	BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);
	void OnDestroy();
	void OnConfigTextChange(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnOK(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl);
	UINT OnGetDlgCode(LPMSG lpMsg);

private:
	CContainedWindowT<CEdit> m_wndEdit;
	CFont m_fontConfigText;

	static CPath GetIniFilePath();
	static CString LoadIniFile();
	static bool SaveIniFile(CString newFileContents);
};
