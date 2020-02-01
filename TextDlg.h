#pragma once

#include "UserConfig.h"

class CTextDlg : public CDialogImpl<CTextDlg>
{
public:
	enum { IDD = IDD_TEXTDLG };

	BEGIN_MSG_MAP_EX(CTextDlg)
		MSG_WM_INITDIALOG(OnInitDialog)
		MSG_WM_CTLCOLORSTATIC(OnCtlColorStatic)
		MSG_WM_ACTIVATE(OnActivate)
		COMMAND_ID_HANDLER_EX(IDCANCEL, OnCancel)
		MSG_WM_COMMAND(OnCommand)
		MESSAGE_HANDLER_EX(WM_NCHITTEST, OnNcHitTest)
	ALT_MSG_MAP(1)
		MSG_WM_KEYDOWN(OnKeyDown)
		MSG_WM_KEYUP(OnKeyUp)
		MSG_WM_LBUTTONUP(OnLButtonUp)
		MSG_WM_CHAR(OnChar)
	END_MSG_MAP()

	CTextDlg(std::vector<WebButtonInfo> webButtonInfos, bool autoCopySelection = false, bool unicodeSpacesToAscii = false) :
		m_wndEdit(this, 1),
		m_webButtonInfos(std::move(webButtonInfos)),
		m_autoCopySelection(autoCopySelection),
		m_unicodeSpacesToAscii(unicodeSpacesToAscii) {}

	BOOL OnIdle();

	BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);
	HBRUSH OnCtlColorStatic(CDCHandle dc, CStatic wndStatic);
	void OnActivate(UINT nState, BOOL bMinimized, CWindow wndOther);
	void OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnCommand(UINT uNotifyCode, int nID, CWindow wndCtl);
	LRESULT OnNcHitTest(UINT uMsg, WPARAM wParam, LPARAM lParam);
	void OnKeyDown(UINT vk, UINT nRepCnt, UINT nFlags);
	void OnKeyUp(UINT vk, UINT nRepCnt, UINT nFlags);
	void OnLButtonUp(UINT nFlags, CPoint point);
	void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);

private:
	CContainedWindowT<CEdit> m_wndEdit;
	bool m_autoCopySelection, m_unicodeSpacesToAscii;
	std::vector<int> m_editIndexes;
	int m_lastSelStart = 0, m_lastSelEnd = 0;
	std::vector<WebButtonInfo> m_webButtonInfos;
	std::vector<CIcon> m_webButtonIcons;
	bool m_showingModalBrowserHost = false;

	void InitWebAppButtons();
	void AdjustWindowLocationAndSize(CPoint ptEvent, CRect rcAccObject, CString strText);
	void OnSelectionMaybeChanged();
};
