#include "stdafx.h"
#include "resource.h"

#include "TextDlg.h"
#include "WebAppLaunch.h"

namespace
{
	void GetAccessibleInfoFromPointMSAA(POINT pt, CWindow& window, CString& outString, CRect& outRc, std::vector<int>& outIndexes);
	bool GetAccessibleInfoFromPoint(POINT pt, CWindow& window, CString& outString, CRect& outRc, std::vector<int>& outIndexes);
	void UnicodeSpacesToAscii(CString& string);
	int MyGetDpiForWindow(HWND hWnd);
	int ScaleForWindow(HWND hWnd, int value);
	int GetSystemMetricsForWindow(HWND hWnd, int nIndex);
	int AdjustWindowRectExForWindow(HWND hWnd, LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle);
	BOOL UnadjustWindowRectExForWindow(HWND hWnd, LPRECT prc, DWORD dwStyle, BOOL fMenu, DWORD dwExStyle);
	BOOL WndAdjustWindowRect(CWindow window, LPRECT prc);
	BOOL WndUnadjustWindowRect(CWindow window, LPRECT prc);
	CSize GetEditControlTextSize(CEdit window, LPCTSTR lpszString, int nMaxWidth = INT_MAX);
	CSize TextSizeToEditClientSize(HWND hWnd, CEdit editWnd, CSize textSize);
	CSize EditClientSizeToTextSize(HWND hWnd, CEdit editWnd, CSize editClientSize);
	BOOL SetClipboardText(const WCHAR* text);
}

BOOL CTextDlg::OnInitDialog(CWindow wndFocus, LPARAM lInitParam)
{
	CPoint& ptEvent = *reinterpret_cast<CPoint*>(lInitParam);

	CWindow wndAcc;
	CString strText;
	CRect rcAccObject;
	if(m_config.m_useLegacyMsaaApi ||
		!GetAccessibleInfoFromPoint(ptEvent, wndAcc, strText, rcAccObject, m_editIndexes))
	{
		GetAccessibleInfoFromPointMSAA(ptEvent, wndAcc, strText, rcAccObject, m_editIndexes);
	}

	// Check whether the target window is another TextDlg.
	if(wndAcc)
	{
		CWindow wndAccRoot{ ::GetAncestor(wndAcc, GA_ROOT) };
		if(wndAccRoot)
		{
			WCHAR szBuffer[32];
			if(::GetClassName(wndAccRoot, szBuffer, ARRAYSIZE(szBuffer)) &&
				wcscmp(szBuffer, L"TextifyEditDlg") == 0)
			{
				wndAccRoot.SendMessage(WM_CLOSE);
				EndDialog(0);
				return FALSE;
			}
		}
	}

	// Allows to steal focus
	INPUT input{};
	SendInput(1, &input, sizeof(INPUT));

	if(!::SetForegroundWindow(m_hWnd))
	{
		//EndDialog(0);
		//return FALSE;
	}

	InitWebAppButtons();

	CString strDefaultText;
	strDefaultText.LoadString(IDS_DEFAULT_TEXT);

	if(m_config.m_unicodeSpacesToAscii)
	{
		UnicodeSpacesToAscii(strText);
	}

	if(strText.IsEmpty())
	{
		strText = strDefaultText;
	}

	CEdit editWnd = GetDlgItem(IDC_EDIT);
	editWnd.SetLimitText(0);
	editWnd.SetWindowText(strText);

	AdjustWindowLocationAndSize(ptEvent, rcAccObject, strText, strDefaultText);

	m_lastSelStart = 0;
	m_lastSelEnd = strText.GetLength();

	if(m_config.m_autoCopySelection)
	{
		SetClipboardText(strText);
	}

	m_wndEdit.SubclassWindow(editWnd);

	return TRUE;
}

HBRUSH CTextDlg::OnCtlColorStatic(CDCHandle dc, CStatic wndStatic)
{
	if(wndStatic.GetDlgCtrlID() == IDC_EDIT)
	{
		return GetSysColorBrush(COLOR_WINDOW);
	}

	SetMsgHandled(FALSE);
	return NULL;
}

void CTextDlg::OnActivate(UINT nState, BOOL bMinimized, CWindow wndOther)
{
	if(nState == WA_INACTIVE && !m_showingModalBrowserHost)
		EndDialog(0);
}

void CTextDlg::OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	EndDialog(nID);
}

void CTextDlg::OnCommand(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	int buttonIndex = nID - IDC_WEB_BUTTON_1;
	if(buttonIndex >= 0 && buttonIndex < static_cast<int>(m_config.m_webButtonInfos.size()))
	{
		CString selectedText;
		m_wndEdit.GetWindowText(selectedText);

		int start, end;
		m_wndEdit.GetSel(start, end);
		if(start < end)
		{
			selectedText = selectedText.Mid(start, end - start);
		}

		m_showingModalBrowserHost = true;
		ShowWindow(SW_HIDE);

		const auto& buttonInfo = m_config.m_webButtonInfos[buttonIndex];
		bool succeeded = CommandLaunch(buttonInfo.command, selectedText,
			buttonInfo.width, buttonInfo.height);

		if(!succeeded)
		{
			CString title;
			title.LoadString(IDS_ERROR);

			CString text;
			text.LoadString(IDS_ERROR_EXECUTE);
			text += L"\n";
			text += buttonInfo.command;

			MessageBox(text, title, MB_ICONERROR);
		}

		EndDialog(0);
	}
}

LRESULT CTextDlg::OnNcHitTest(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = ::DefWindowProc(m_hWnd, uMsg, wParam, lParam);
	if(result == HTCLIENT)
		result = HTCAPTION;

	return result;
}

void CTextDlg::OnKeyDown(UINT vk, UINT nRepCnt, UINT nFlags)
{
	if(vk == VK_TAB)
	{
		int start, end;
		m_wndEdit.GetSel(start, end);
		int length = m_wndEdit.SendMessage(WM_GETTEXTLENGTH);

		int newStart, newEnd;

		const int newlineSize = sizeof("\r\n") - 1;

		if(start == 0 && end == length) // all text is selected
		{
			newStart = 0;
			newEnd = m_editIndexes.empty() ? length : (m_editIndexes.front() - newlineSize);
		}
		else
		{
			newStart = 0;
			newEnd = length;

			for(size_t i = 0; i < m_editIndexes.size(); i++)
			{
				int from = (i == 0) ? 0 : m_editIndexes[i - 1];
				int to = m_editIndexes[i] - newlineSize;

				if(from == start && to == end)
				{
					newStart = m_editIndexes[i];
					newEnd = (i + 1 < m_editIndexes.size()) ? (m_editIndexes[i + 1] - newlineSize) : length;

					break;
				}
			}
		}

		m_wndEdit.SetSel(newStart, newEnd, TRUE);
	}
	else
	{
		SetMsgHandled(FALSE);
	}
}

void CTextDlg::OnKeyUp(UINT vk, UINT nRepCnt, UINT nFlags)
{
	OnSelectionMaybeChanged();
	SetMsgHandled(FALSE);
}

void CTextDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
	OnSelectionMaybeChanged();
	SetMsgHandled(FALSE);
}

void CTextDlg::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if(nChar == 1) // Ctrl+A
	{
		m_wndEdit.SetSelAll(TRUE);
	}
	else
	{
		SetMsgHandled(FALSE);
	}
}

void CTextDlg::InitWebAppButtons()
{
	// This id shouldn't be in use yet, we're going to use it for a newly created button.
	ATLASSERT(!GetDlgItem(IDC_WEB_BUTTON_1));

	int numberOfButtons = static_cast<int>(m_config.m_webButtonInfos.size());
	if(numberOfButtons == 0)
	{
		return;
	}

	m_webButtonIcons.resize(numberOfButtons);

	int buttonSize = ScaleForWindow(m_hWnd, m_config.m_webButtonsIconSize);

	CRect buttonRect{
		0,
		0,
		GetSystemMetricsForWindow(m_hWnd, SM_CXEDGE) * 4 + buttonSize,
		GetSystemMetricsForWindow(m_hWnd, SM_CYEDGE) * 4 + buttonSize
	};

	for(int i = 0; i < numberOfButtons; i++)
	{
		CString buttonText;
		if(m_config.m_webButtonInfos[i].acceleratorKey)
			buttonText.Format(L"&%c", m_config.m_webButtonInfos[i].acceleratorKey);
		else if(i + 1 < 10)
			buttonText.Format(L"&%d", i + 1);
		else if(i + 1 == 10)
			buttonText.Format(L"1&0");
		else
			buttonText.Format(L"%d", i + 1);

		CButton button;
		button.Create(m_hWnd, buttonRect, buttonText,
			WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0, IDC_WEB_BUTTON_1 + i);
		button.SetFont(GetFont());

		if(!m_config.m_webButtonInfos[i].name.IsEmpty())
		{
			if(!m_webButtonTooltip)
			{
				m_webButtonTooltip.Create(m_hWnd, NULL, NULL, WS_POPUP | TTS_NOPREFIX, WS_EX_TOPMOST);
			}

			m_webButtonTooltip.AddTool(
				CToolInfo(TTF_SUBCLASS, button, 0, NULL, (PWSTR)m_config.m_webButtonInfos[i].name.GetString()));
		}

		if(!m_config.m_webButtonInfos[i].iconPath.m_strPath.IsEmpty())
		{
			HICON icon = (HICON)::LoadImage(nullptr, m_config.m_webButtonInfos[i].iconPath.m_strPath.GetString(),
				IMAGE_ICON, buttonSize, buttonSize, LR_LOADFROMFILE);
			if(icon)
			{
				button.ModifyStyle(0, BS_ICON);
				button.SetIcon(icon);
				m_webButtonIcons[i] = icon;
			}
		}
	}
}

void CTextDlg::AdjustWindowLocationAndSize(CPoint ptEvent, CRect rcAccObject, CString strText, CString strDefaultText)
{
	CEdit editWnd = GetDlgItem(IDC_EDIT);

	// A dirty, partial workaround.
	// http://stackoverflow.com/questions/35673347
	strText.Replace(L"!", L"! ");
	strText.Replace(L"|", L"| ");
	strText.Replace(L"?", L"? ");
	strText.Replace(L"-", L"- ");
	strText.Replace(L"}", L"} ");
	strText.Replace(L"{", L" {");
	strText.Replace(L"[", L" [");
	strText.Replace(L"(", L" (");
	strText.Replace(L"+", L" +");
	strText.Replace(L"%", L" %");
	strText.Replace(L"$", L" $");
	strText.Replace(L"\\", L" \\");

	CSize defTextSize = GetEditControlTextSize(editWnd, strDefaultText);
	CSize defTextSizeClient = TextSizeToEditClientSize(m_hWnd, editWnd, defTextSize);

	int nMaxClientWidth = defTextSizeClient.cx > rcAccObject.Width() ? defTextSizeClient.cx : rcAccObject.Width();

	CSize textSize = GetEditControlTextSize(editWnd, strText, nMaxClientWidth);
	CSize textSizeClient = TextSizeToEditClientSize(m_hWnd, editWnd, textSize);

	if(textSizeClient.cx < rcAccObject.Width())
	{
		// Perhaps it will look better if we won't shrink the control,
		// as it will fit perfectly above the control.
		// Let's see if the shrinking is small.

		int nMinClientWidth = ScaleForWindow(m_hWnd, 200);

		if(rcAccObject.Width() <= nMinClientWidth || textSizeClient.cx * 1.5 >= rcAccObject.Width())
		{
			int delta = rcAccObject.Width() - textSizeClient.cx;
			textSizeClient.cx = rcAccObject.Width();
			textSize.cx += delta;

			// Recalculate the height, which might be smaller now.
			//CSize newTextSize = GetEditControlTextSize(editWnd, strText, textSize.cx);
			//textSize.cy = newTextSize.cy;
		}
	}

	CRect rcClient{ rcAccObject.TopLeft(), textSizeClient };
	if(rcAccObject.IsRectEmpty() || !rcClient.PtInRect(ptEvent))
	{
		CPoint ptWindowLocation{ ptEvent };
		ptWindowLocation.Offset(-rcClient.Width() / 2, -rcClient.Height() / 2);
		rcClient.MoveToXY(ptWindowLocation);
	}

	int numberOfWebAppButtons = static_cast<int>(m_config.m_webButtonInfos.size());
	int numberOfWebAppButtonsX = numberOfWebAppButtons;
	int numberOfWebAppButtonsY = 1;
	CSize webAppButtonSize;
	if(numberOfWebAppButtons > 0)
	{
		int numberOfWebAppButtonsPerRow = m_config.m_webButtonsPerRow;
		if(numberOfWebAppButtonsPerRow > 0)
		{
			numberOfWebAppButtonsX = numberOfWebAppButtons > numberOfWebAppButtonsPerRow ? numberOfWebAppButtonsPerRow : numberOfWebAppButtons;
			numberOfWebAppButtonsY = (numberOfWebAppButtons + numberOfWebAppButtonsPerRow - 1) / numberOfWebAppButtonsPerRow;
		}

		CButton webAppButton = GetDlgItem(IDC_WEB_BUTTON_1);
		CRect webAppButtonRect;
		webAppButton.GetWindowRect(webAppButtonRect);
		webAppButtonSize = webAppButtonRect.Size();

		if(rcClient.Width() < webAppButtonSize.cx * numberOfWebAppButtonsX)
			rcClient.right = rcClient.left + webAppButtonSize.cx * numberOfWebAppButtonsX;

		rcClient.bottom += webAppButtonSize.cy * numberOfWebAppButtonsY;
	}

	CRect rcWindow{ rcClient };
	WndAdjustWindowRect(m_hWnd, &rcWindow);

	HMONITOR hMonitor = MonitorFromPoint(ptEvent, MONITOR_DEFAULTTONEAREST);
	MONITORINFO monitorinfo = { sizeof(MONITORINFO) };
	if(GetMonitorInfo(hMonitor, &monitorinfo))
	{
		CRect rcMonitor{ monitorinfo.rcMonitor };
		CRect rcWindowPrev{ rcWindow };

		if(rcWindow.Width() > rcMonitor.Width() ||
			rcWindow.Height() > rcMonitor.Height())
		{
			if(rcWindow.Height() > rcMonitor.Height())
			{
				rcWindow.top = 0;
				rcWindow.bottom = rcMonitor.Height();
			}

			editWnd.ShowScrollBar(SB_VERT);
			rcWindow.right += GetSystemMetricsForWindow(m_hWnd, SM_CXVSCROLL);
			if(rcWindow.Width() > rcMonitor.Width())
			{
				rcWindow.left = 0;
				rcWindow.right = rcMonitor.Width();
			}
		}

		if(rcWindow.left < rcMonitor.left)
		{
			rcWindow.MoveToX(rcMonitor.left);
		}
		else if(rcWindow.right > rcMonitor.right)
		{
			rcWindow.MoveToX(rcMonitor.right - rcWindow.Width());
		}

		if(rcWindow.top < rcMonitor.top)
		{
			rcWindow.MoveToY(rcMonitor.top);
		}
		else if(rcWindow.bottom > rcMonitor.bottom)
		{
			rcWindow.MoveToY(rcMonitor.bottom - rcWindow.Height());
		}

		if(rcWindowPrev != rcWindow)
		{
			rcClient = rcWindow;
			WndUnadjustWindowRect(m_hWnd, &rcClient);
		}
	}

	if(numberOfWebAppButtons > 0)
	{
		if(rcClient.bottom - webAppButtonSize.cy * numberOfWebAppButtonsY > rcClient.top)
		{
			rcClient.bottom -= webAppButtonSize.cy * numberOfWebAppButtonsY;

			CPoint ptButton{ 0, rcClient.Height() };

			for(int i = 0; i < numberOfWebAppButtons; i++)
			{
				if(i > 0 && i % numberOfWebAppButtonsX == 0)
				{
					ptButton.x = 0;
					ptButton.y += webAppButtonSize.cy;
				}

				CButton webAppButton = GetDlgItem(IDC_WEB_BUTTON_1 + i);
				webAppButton.SetWindowPos(NULL, ptButton.x, ptButton.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

				ptButton.x += webAppButtonSize.cx;
			}
		}
		else
		{
			// Hide WebApp buttons.
			for(int i = 0; i < numberOfWebAppButtons; i++)
			{
				CButton webAppButton = GetDlgItem(IDC_WEB_BUTTON_1 + i);
				webAppButton.ShowWindow(SW_HIDE);
			}
		}
	}

	SetWindowPos(NULL, &rcWindow, SWP_NOZORDER | SWP_NOACTIVATE);
	editWnd.SetWindowPos(NULL, 0, 0, rcClient.Width(), rcClient.Height(), SWP_NOZORDER | SWP_NOACTIVATE);
}

void CTextDlg::OnSelectionMaybeChanged()
{
	if(m_config.m_autoCopySelection)
	{
		int start, end;
		m_wndEdit.GetSel(start, end);
		if(start < end && (start != m_lastSelStart || end != m_lastSelEnd))
		{
			CString str;
			m_wndEdit.GetWindowText(str);

			SetClipboardText(str.Mid(start, end - start));

			m_lastSelStart = start;
			m_lastSelEnd = end;
		}
	}
}

namespace
{
	void GetAccessibleInfoFromPointMSAA(POINT pt, CWindow& window, CString& outString, CRect& outRc, std::vector<int>& outIndexes)
	{
		outString.Empty();
		outRc = CRect{ pt, CSize{ 0, 0 } };
		outIndexes = std::vector<int>();

		HRESULT hr;

		CComPtr<IAccessible> pAcc;
		CComVariant vtChild;
		hr = AccessibleObjectFromPoint(pt, &pAcc, &vtChild);
		if(FAILED(hr))
		{
			return;
		}

		hr = WindowFromAccessibleObject(pAcc, &window.m_hWnd);
		if(FAILED(hr))
		{
			return;
		}

		DWORD processId = 0;
		GetWindowThreadProcessId(window.m_hWnd, &processId);

		while(true)
		{
			CString string;
			std::vector<int> indexes;

			CComBSTR bsName;
			hr = pAcc->get_accName(vtChild, &bsName);
			if(SUCCEEDED(hr) && bsName.Length() > 0)
			{
				string = bsName;
			}

			CComBSTR bsValue;
			hr = pAcc->get_accValue(vtChild, &bsValue);
			if(SUCCEEDED(hr) && bsValue.Length() > 0 &&
				bsValue != bsName)
			{
				if(!string.IsEmpty())
				{
					string += L"\r\n";
					indexes.push_back(string.GetLength());
				}

				string += bsValue;
			}

			CComVariant vtRole;
			hr = pAcc->get_accRole(CComVariant(CHILDID_SELF), &vtRole);
			if(FAILED(hr) || vtRole.lVal != ROLE_SYSTEM_TITLEBAR) // ignore description for the system title bar
			{
				CComBSTR bsDescription;
				hr = pAcc->get_accDescription(vtChild, &bsDescription);
				if(SUCCEEDED(hr) && bsDescription.Length() > 0 &&
					bsDescription != bsName && bsDescription != bsValue)
				{
					if(!string.IsEmpty())
					{
						string += L"\r\n";
						indexes.push_back(string.GetLength());
					}

					string += bsDescription;
				}
			}

			if(!string.IsEmpty())
			{
				// Normalize newlines.
				string.Replace(L"\r\n", L"\n");
				string.Replace(L"\r", L"\n");
				string.Replace(L"\n", L"\r\n");

				string.TrimRight();

				if(!string.IsEmpty())
				{
					outString = string;
					outIndexes = indexes;
					break;
				}
			}

			if(vtChild.lVal == CHILDID_SELF)
			{
				CComPtr<IDispatch> pDispParent;
				HRESULT hr = pAcc->get_accParent(&pDispParent);
				if(FAILED(hr) || !pDispParent)
				{
					break;
				}

				CComQIPtr<IAccessible> pAccParent(pDispParent);
				pAcc.Attach(pAccParent.Detach());

				HWND hWnd;
				hr = WindowFromAccessibleObject(pAcc, &hWnd);
				if(FAILED(hr))
				{
					break;
				}

				DWORD compareProcessId = 0;
				GetWindowThreadProcessId(hWnd, &compareProcessId);
				if(compareProcessId != processId)
				{
					break;
				}
			}
			else
			{
				vtChild.lVal = CHILDID_SELF;
			}
		}

		long pxLeft, pyTop, pcxWidth, pcyHeight;
		hr = pAcc->accLocation(&pxLeft, &pyTop, &pcxWidth, &pcyHeight, vtChild);
		if(SUCCEEDED(hr))
		{
			outRc = CRect{ CPoint{ pxLeft, pyTop }, CSize{ pcxWidth, pcyHeight } };
		}
	}

	bool GetAccessibleInfoFromPoint(POINT pt, CWindow& window, CString& outString, CRect& outRc, std::vector<int>& outIndexes)
	{
		outString.Empty();
		outRc = CRect{ pt, CSize{ 0, 0 } };
		outIndexes = std::vector<int>();

		HRESULT hr;

		CComPtr<IUIAutomation> uia;
		hr = uia.CoCreateInstance(CLSID_CUIAutomation);
		if(FAILED(hr) || !uia)
		{
			return false;
		}

		CComPtr<IUIAutomationElement> element;
		hr = uia->ElementFromPoint(pt, &element);
		if(FAILED(hr) || !element)
		{
			return true;
		}

		int processId = 0;
		hr = element->get_CurrentProcessId(&processId);
		if(FAILED(hr))
		{
			return true;
		}

		while(true)
		{
			CString string;
			std::vector<int> indexes;

			CComBSTR bsName;
			hr = element->get_CurrentName(&bsName);
			if(SUCCEEDED(hr) && bsName.Length() > 0)
			{
				string = bsName;
			}

			CComVariant varValue;
			hr = element->GetCurrentPropertyValue(UIA_ValueValuePropertyId, &varValue);
			if(SUCCEEDED(hr) && varValue.vt == VT_BSTR)
			{
				CComBSTR bsValue = varValue.bstrVal;
				if(bsValue.Length() > 0 && bsValue != bsName)
				{
					if(!string.IsEmpty())
					{
						string += L"\r\n";
						indexes.push_back(string.GetLength());
					}

					string += bsValue;
				}
			}

			if(!string.IsEmpty())
			{
				// Normalize newlines.
				string.Replace(L"\r\n", L"\n");
				string.Replace(L"\r", L"\n");
				string.Replace(L"\n", L"\r\n");

				string.TrimRight();

				if(!string.IsEmpty())
				{
					outString = string;
					outIndexes = indexes;
					break;
				}
			}

			CComPtr<IUIAutomationCondition> trueCondition;
			hr = uia->CreateTrueCondition(&trueCondition);
			if(FAILED(hr) || !trueCondition)
			{
				break;
			}

			CComPtr<IUIAutomationTreeWalker> treeWalker;
			hr = uia->CreateTreeWalker(trueCondition, &treeWalker);
			if(FAILED(hr) || !treeWalker)
			{
				break;
			}

			CComPtr<IUIAutomationElement> parentElement;
			hr = treeWalker->GetParentElement(element, &parentElement);
			if(FAILED(hr) || !parentElement)
			{
				break;
			}

			element.Attach(parentElement.Detach());

			int compareProcessId = 0;
			hr = element->get_CurrentProcessId(&compareProcessId);
			if(FAILED(hr) || compareProcessId != processId)
			{
				break;
			}
		}

		hr = element->get_CurrentNativeWindowHandle((UIA_HWND*)&window.m_hWnd);
		if(SUCCEEDED(hr) && !window)
		{
			CComPtr<IUIAutomationCondition> trueCondition;
			hr = uia->CreateTrueCondition(&trueCondition);
			if(SUCCEEDED(hr) && trueCondition)
			{
				CComPtr<IUIAutomationTreeWalker> treeWalker;
				hr = uia->CreateTreeWalker(trueCondition, &treeWalker);
				if(SUCCEEDED(hr) && treeWalker)
				{
					CComPtr<IUIAutomationElement> parentElement;
					hr = treeWalker->GetParentElement(element, &parentElement);
					if(SUCCEEDED(hr) && parentElement)
					{
						while(true)
						{
							hr = parentElement->get_CurrentNativeWindowHandle((UIA_HWND*)&window.m_hWnd);
							if(FAILED(hr) || window)
							{
								break;
							}

							CComPtr<IUIAutomationElement> nextParentElement;
							hr = treeWalker->GetParentElement(parentElement, &nextParentElement);
							if(FAILED(hr) || !parentElement)
							{
								break;
							}

							parentElement.Attach(nextParentElement.Detach());
						}
					}
				}
			}
		}

		CRect boundingRc;
		hr = element->get_CurrentBoundingRectangle(&boundingRc);
		if(SUCCEEDED(hr))
		{
			outRc = boundingRc;
		}

		return true;
	}

	void UnicodeSpacesToAscii(CString& string)
	{
		// Based on:
		// https://stackoverflow.com/a/21797208
		// https://www.compart.com/en/unicode/category/Zs
		// https://www.compart.com/en/unicode/category/Cf
		std::unordered_set<WCHAR> unicodeSpaces = {
			L'\u00A0', // No-Break Space (NBSP)
			//L'\u1680', // Ogham Space Mark
			L'\u2000', // En Quad
			L'\u2001', // Em Quad
			L'\u2002', // En Space
			L'\u2003', // Em Space
			L'\u2004', // Three-Per-Em Space
			L'\u2005', // Four-Per-Em Space
			L'\u2006', // Six-Per-Em Space
			L'\u2007', // Figure Space
			L'\u2008', // Punctuation Space
			L'\u2009', // Thin Space
			L'\u200A', // Hair Space
			L'\u202F', // Narrow No-Break Space (NNBSP)
			L'\u205F', // Medium Mathematical Space (MMSP)
			L'\u3000'  // Ideographic Space
		};
		std::unordered_set<WCHAR> unicodeInvisible = {
			L'\u00AD', // Soft Hyphen (SHY)
			//L'\u0600', // Arabic Number Sign
			//L'\u0601', // Arabic Sign Sanah
			//L'\u0602', // Arabic Footnote Marker
			//L'\u0603', // Arabic Sign Safha
			//L'\u0604', // Arabic Sign Samvat
			//L'\u0605', // Arabic Number Mark Above
			//L'\u061C', // Arabic Letter Mark (ALM)
			//L'\u06DD', // Arabic End of Ayah
			//L'\u070F', // Syriac Abbreviation Mark
			//L'\u08E2', // Arabic Disputed End of Ayah
			//L'\u180E', // Mongolian Vowel Separator (MVS)
			L'\u200B', // Zero Width Space (ZWSP)
			L'\u200C', // Zero Width Non-Joiner (ZWNJ)
			L'\u200D', // Zero Width Joiner (ZWJ)
			L'\u200E', // Left-to-Right Mark (LRM)
			L'\u200F', // Right-to-Left Mark (RLM)
			L'\u202A', // Left-to-Right Embedding (LRE)
			L'\u202B', // Right-to-Left Embedding (RLE)
			L'\u202C', // Pop Directional Formatting (PDF)
			L'\u202D', // Left-to-Right Override (LRO)
			L'\u202E', // Right-to-Left Override (RLO)
			L'\u2060', // Word Joiner (WJ)
			L'\u2061', // Function Application
			L'\u2062', // Invisible Times
			L'\u2063', // Invisible Separator
			L'\u2064', // Invisible Plus
			L'\u2066', // Left-to-Right Isolate (LRI)
			L'\u2067', // Right-to-Left Isolate (RLI)
			L'\u2068', // First Strong Isolate (FSI)
			L'\u2069', // Pop Directional Isolate (PDI)
			L'\u206A', // Inhibit Symmetric Swapping
			L'\u206B', // Activate Symmetric Swapping
			//L'\u206C', // Inhibit Arabic Form Shaping
			//L'\u206D', // Activate Arabic Form Shaping
			L'\u206E', // National Digit Shapes
			L'\u206F', // Nominal Digit Shapes
			L'\uFEFF', // Zero Width No-Break Space (BOM, ZWNBSP)
			L'\uFFF9', // Interlinear Annotation Anchor
			L'\uFFFA', // Interlinear Annotation Separator
			L'\uFFFB'  // Interlinear Annotation Terminator
		};

		int i = 0;
		while(i < string.GetLength())
		{
			if(unicodeInvisible.find(string[i]) != unicodeInvisible.end())
			{
				string.Delete(i, 1);
				continue;
			}

			if(unicodeSpaces.find(string[i]) != unicodeSpaces.end())
			{
				string.SetAt(i, L' ');
			}

			i++;
		}
	}

	int MyGetDpiForWindow(HWND hWnd)
	{
		using GetDpiForWindow_t = UINT(WINAPI*)(HWND hwnd);
		static GetDpiForWindow_t pGetDpiForWindow = []()
		{
			HMODULE hUser32 = GetModuleHandle(L"user32.dll");
			if(hUser32)
			{
				return (GetDpiForWindow_t)GetProcAddress(hUser32, "GetDpiForWindow");
			}

			return (GetDpiForWindow_t)nullptr;
		}();

		int iDpi = 96;
		if(pGetDpiForWindow)
		{
			iDpi = pGetDpiForWindow(hWnd);
		}
		else
		{
			CDC hdc = ::GetDC(NULL);
			if(hdc)
			{
				iDpi = hdc.GetDeviceCaps(LOGPIXELSX);
			}
		}

		return iDpi;
	}

	int ScaleForWindow(HWND hWnd, int value)
	{
		return MulDiv(value, MyGetDpiForWindow(hWnd), 96);
	}

	int GetSystemMetricsForWindow(HWND hWnd, int nIndex)
	{
		using GetSystemMetricsForDpi_t = int(WINAPI*)(int nIndex, UINT dpi);
		static GetSystemMetricsForDpi_t pGetSystemMetricsForDpi = []()
		{
			HMODULE hUser32 = GetModuleHandle(L"user32.dll");
			if(hUser32)
			{
				return (GetSystemMetricsForDpi_t)GetProcAddress(hUser32, "GetSystemMetricsForDpi");
			}

			return (GetSystemMetricsForDpi_t)nullptr;
		}();

		if(pGetSystemMetricsForDpi)
		{
			return pGetSystemMetricsForDpi(nIndex, MyGetDpiForWindow(hWnd));
		}
		else
		{
			return GetSystemMetrics(nIndex);
		}
	}

	int AdjustWindowRectExForWindow(HWND hWnd, LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle)
	{
		using AdjustWindowRectExForDpi_t = BOOL(WINAPI*)(LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle, UINT dpi);
		static AdjustWindowRectExForDpi_t pAdjustWindowRectExForDpi = []()
		{
			HMODULE hUser32 = GetModuleHandle(L"user32.dll");
			if(hUser32)
			{
				return (AdjustWindowRectExForDpi_t)GetProcAddress(hUser32, "AdjustWindowRectExForDpi");
			}

			return (AdjustWindowRectExForDpi_t)nullptr;
		}();

		if(pAdjustWindowRectExForDpi)
		{
			return pAdjustWindowRectExForDpi(lpRect, dwStyle, bMenu, dwExStyle, MyGetDpiForWindow(hWnd));
		}
		else
		{
			return AdjustWindowRectEx(lpRect, dwStyle, bMenu, dwExStyle);
		}
	}

	BOOL UnadjustWindowRectExForWindow(HWND hWnd, LPRECT prc, DWORD dwStyle, BOOL fMenu, DWORD dwExStyle)
	{
		RECT rc;
		SetRectEmpty(&rc);

		BOOL fRc = AdjustWindowRectExForWindow(hWnd, &rc, dwStyle, fMenu, dwExStyle);
		if(fRc)
		{
			prc->left -= rc.left;
			prc->top -= rc.top;
			prc->right -= rc.right;
			prc->bottom -= rc.bottom;
		}

		return fRc;
	}

	BOOL WndAdjustWindowRect(CWindow window, LPRECT prc)
	{
		DWORD dwStyle = window.GetStyle();
		DWORD dwExStyle = window.GetExStyle();
		BOOL bMenu = (!(dwStyle & WS_CHILD) && (window.GetMenu() != NULL));

		return AdjustWindowRectExForWindow(window, prc, dwStyle, bMenu, dwExStyle);
	}

	BOOL WndUnadjustWindowRect(CWindow window, LPRECT prc)
	{
		DWORD dwStyle = window.GetStyle();
		DWORD dwExStyle = window.GetExStyle();
		BOOL bMenu = (!(dwStyle & WS_CHILD) && (window.GetMenu() != NULL));

		return UnadjustWindowRectExForWindow(window, prc, dwStyle, bMenu, dwExStyle);
	}

	CSize GetEditControlTextSize(CEdit window, LPCTSTR lpszString, int nMaxWidth /*= INT_MAX*/)
	{
		CFontHandle pEdtFont = window.GetFont();
		if(!pEdtFont)
			return CSize{};

		CClientDC oDC{ window };
		CFontHandle pOldFont = oDC.SelectFont(pEdtFont);

		CRect rc{ 0, 0, nMaxWidth, 0 };
		oDC.DrawTextEx((LPTSTR)lpszString, -1, &rc, DT_CALCRECT | DT_EDITCONTROL | DT_WORDBREAK | DT_NOPREFIX | DT_EXPANDTABS | DT_TABSTOP);

		oDC.SelectFont(pOldFont);

		return rc.Size();
	}

	CSize TextSizeToEditClientSize(HWND hWnd, CEdit editWnd, CSize textSize)
	{
		CRect rc{ CPoint{ 0, 0 }, textSize };
		WndAdjustWindowRect(editWnd, &rc);

		UINT nLeftMargin, nRightMargin;
		editWnd.GetMargins(nLeftMargin, nRightMargin);

		CSize editClientSize;

		// Experiments show that this works kinda ok.
		editClientSize.cx = rc.Width() +
			nLeftMargin + nRightMargin +
			2 * GetSystemMetricsForWindow(hWnd, SM_CXBORDER) + 2 * GetSystemMetricsForWindow(hWnd, SM_CXDLGFRAME);

		editClientSize.cy = rc.Height() +
			2 * GetSystemMetricsForWindow(hWnd, SM_CYBORDER)/* +
			2 * GetSystemMetricsForWindow(hWnd, SM_CYDLGFRAME)*/;

		return editClientSize;
	}

	CSize EditClientSizeToTextSize(HWND hWnd, CEdit editWnd, CSize editClientSize)
	{
		UINT nLeftMargin, nRightMargin;
		editWnd.GetMargins(nLeftMargin, nRightMargin);

		editClientSize.cx -=
			nLeftMargin + nRightMargin +
			2 * GetSystemMetricsForWindow(hWnd, SM_CXBORDER) + 2 * GetSystemMetricsForWindow(hWnd, SM_CXDLGFRAME);

		editClientSize.cy -=
			2 * GetSystemMetricsForWindow(hWnd, SM_CYBORDER)/* +
			2 * GetSystemMetricsForWindow(hWnd, SM_CYDLGFRAME)*/;

		CRect rc{ CPoint{ 0, 0 }, editClientSize };
		WndUnadjustWindowRect(editWnd, &rc);

		return rc.Size();
	}

	BOOL SetClipboardText(const WCHAR* text)
	{
		if(!OpenClipboard(NULL))
			return FALSE;

		BOOL bSucceeded = FALSE;

		size_t size = sizeof(WCHAR) * (wcslen(text) + 1);
		HANDLE handle = GlobalAlloc(GHND, size);
		if(handle)
		{
			WCHAR* clipboardText = (WCHAR*)GlobalLock(handle);
			if(clipboardText)
			{
				memcpy(clipboardText, text, size);
				GlobalUnlock(handle);
				bSucceeded = EmptyClipboard() &&
					SetClipboardData(CF_UNICODETEXT, handle);
			}

			if(!bSucceeded)
				GlobalFree(handle);
		}

		CloseClipboard();
		return bSucceeded;
	}
}
