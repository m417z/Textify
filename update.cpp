#include "stdafx.h"
#include "update.h"
#include "version.h"
#include "resource.h"

#ifndef TDF_SIZE_TO_CONTENT
#define TDF_SIZE_TO_CONTENT 0x1000000
#endif

typedef struct tagUTASKDIALOGSTRUCT {
	HWND hWnd;
	TASKDIALOGCONFIG* pTaskDialogConfig;
	TASKDIALOG_BUTTON* pButtons;
	TASKDIALOG_BUTTON* pNewButtons;

	WCHAR szSetupPath[MAX_PATH];
	BOOL bRunElevated;

	BOOL bDownloading;
	BOOL bShowingProgressBar;
	WCHAR* pHost;
	WCHAR* pPath;
	WCHAR szTempFile[MAX_PATH];
	DWORD dwFileSizeTotal;
	DWORD dwFileSizeDownloaded;
} UTASKDIALOGSTRUCT;

static HRESULT CALLBACK TaskDialogCallbackProc(HWND hWnd, UINT uNotification, WPARAM wParam, LPARAM lParam, LONG_PTR dwRefData);

#ifdef UPDATE_CHECKER

#include "../update.cpp"

#else

BOOL UpdateCheckInit(HWND hWnd, UINT uMsg)
{
	return FALSE;
}

void UpdateCheckCleanup()
{
}

BOOL UpdateCheckQueue()
{
	return FALSE;
}

void UpdateCheckAbort()
{
}

char* UpdateCheckGetVersion()
{
	return NULL;
}

DWORD UpdateCheckGetVersionLong()
{
	return 0;
}

void UpdateCheckFreeVersion()
{
}

static BOOL UpdateQueue(HWND hWnd, UTASKDIALOGSTRUCT* pUserStruct)
{
	return FALSE;
}

static void UpdateAbort()
{
}

static BOOL UpdateRunInstaller(HWND hWnd, WCHAR* pPath, WCHAR* pSetupPath, BOOL bRunElevated)
{
	return FALSE;
}

static DWORD GetRequestError()
{
	return ERROR_INVALID_FUNCTION;
}

static DWORD GetRequestHttpStatusCode()
{
	return 0;
}

#endif

/*
Source:
http://blog.aaronballman.com/2011/08/how-to-check-access-rights/

Usage:
if(CanAccessFolder(TEXT("C:\\Users\\"), GENERIC_WRITE)) {}
if(CanAccessFolder(TEXT("C:\\"), GENERIC_READ | GENERIC_WRITE)) {}
*/

static BOOL CanAccessFolder(LPCTSTR szFolderName, DWORD dwGenericAccessRights)
{
	BOOL bRet = FALSE;
	DWORD length = 0;

	if(!GetFileSecurity(szFolderName, OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION |
		DACL_SECURITY_INFORMATION, NULL, 0, &length) &&
		ERROR_INSUFFICIENT_BUFFER == GetLastError())
	{
		PSECURITY_DESCRIPTOR security = (PSECURITY_DESCRIPTOR)HeapAlloc(GetProcessHeap(), 0, length);

		if(security && GetFileSecurity(szFolderName, OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION |
			DACL_SECURITY_INFORMATION, security, length, &length))
		{
			HANDLE hToken = NULL;

			if(OpenProcessToken(GetCurrentProcess(), TOKEN_IMPERSONATE | TOKEN_QUERY |
				TOKEN_DUPLICATE | STANDARD_RIGHTS_READ, &hToken))
			{
				HANDLE hImpersonatedToken = NULL;

				if(DuplicateToken(hToken, SecurityImpersonation, &hImpersonatedToken))
				{
					GENERIC_MAPPING mapping = { 0xFFFFFFFF };
					PRIVILEGE_SET privileges = { 0 };
					DWORD grantedAccess = 0, privilegesLength = sizeof(PRIVILEGE_SET);
					BOOL result = FALSE;

					mapping.GenericRead = FILE_GENERIC_READ;
					mapping.GenericWrite = FILE_GENERIC_WRITE;
					mapping.GenericExecute = FILE_GENERIC_EXECUTE;
					mapping.GenericAll = FILE_ALL_ACCESS;

					MapGenericMask(&dwGenericAccessRights, &mapping);

					if(AccessCheck(security, hImpersonatedToken, dwGenericAccessRights,
						&mapping, &privileges, &privilegesLength, &grantedAccess, &result))
					{
						bRet = (result != FALSE);
					}

					CloseHandle(hImpersonatedToken);
				}

				CloseHandle(hToken);
			}

			HeapFree(GetProcessHeap(), 0, security);
		}
	}

	return bRet;
}

void UpdateTaskDialog(HWND hWnd, char* pVersion)
{
	static BOOL(__stdcall * pTaskDialogIndirect)(
		const TASKDIALOGCONFIG * pTaskConfig,
		int* pnButton,
		int* pnRadioButton,
		BOOL * pfVerificationFlagChecked) = (decltype(pTaskDialogIndirect))-1;
	if(pTaskDialogIndirect == (decltype(pTaskDialogIndirect))-1)
	{
		HMODULE hComctl32 = GetModuleHandle(L"comctl32.dll");
		if(hComctl32)
		{
			pTaskDialogIndirect = (decltype(pTaskDialogIndirect))GetProcAddress(hComctl32, (const char*)345);
		}
		else
		{
			pTaskDialogIndirect = nullptr;
		}
	}

	if(!pTaskDialogIndirect)
	{
		CString title;
		title.LoadString(IDS_UPDATE_TITLE);

		CString text;
		text.LoadString(IDS_UPDATE_TEXT);

		if(MessageBox(hWnd, text, title, MB_ICONINFORMATION | MB_OKCANCEL) == IDOK)
		{
			const WCHAR* url = L"https://ramensoftware.com/textify";

			if((int)(UINT_PTR)ShellExecute(hWnd, NULL, url, NULL, NULL, SW_SHOWNORMAL) <= 32)
			{
				CString title;
				title.LoadString(IDS_ERROR);

				CString text;
				text.LoadString(IDS_ERROR_OPEN_ADDRESS);
				text += L"\n";
				text += url;

				MessageBox(hWnd, text, title, MB_ICONERROR);
			}
		}

		return;
	}

	TASKDIALOGCONFIG tdcTaskDialogConfig;
	TASKDIALOG_BUTTON tbButtons[2];
	TASKDIALOG_BUTTON tbNewButtons[2];
	UTASKDIALOGSTRUCT sUserStruct;

	// Buttons params
	CString strUpdate;
	strUpdate.LoadString(IDS_UPDATE_UPDATE);

	CString strClose;
	strClose.LoadString(IDS_UPDATE_CLOSE);

	tbButtons[0].nButtonID = IDOK;
	tbButtons[0].pszButtonText = strUpdate;
	tbButtons[1].nButtonID = IDCANCEL;
	tbButtons[1].pszButtonText = strClose;

	CString strDownloading;
	strDownloading.LoadString(IDS_UPDATE_DOWNLOADING);

	CString strAbort;
	strAbort.LoadString(IDS_UPDATE_ABORT);

	tbNewButtons[0].nButtonID = IDOK;
	tbNewButtons[0].pszButtonText = strDownloading;
	tbNewButtons[1].nButtonID = IDCANCEL;
	tbNewButtons[1].pszButtonText = strAbort;

	// Dialog params
	CString strTitle;
	strTitle.LoadString(IDS_UPDATE_TITLE);

	CString strMainText;
	strMainText.LoadString(IDS_UPDATE_TEXT);

	ZeroMemory(&tdcTaskDialogConfig, sizeof(TASKDIALOGCONFIG));

	tdcTaskDialogConfig.cbSize = sizeof(TASKDIALOGCONFIG);
	tdcTaskDialogConfig.hwndParent = hWnd;
	tdcTaskDialogConfig.hInstance = GetModuleHandle(NULL);
	tdcTaskDialogConfig.dwFlags = TDF_ENABLE_HYPERLINKS | TDF_EXPAND_FOOTER_AREA | TDF_SIZE_TO_CONTENT;
	tdcTaskDialogConfig.pszWindowTitle = strTitle;
	tdcTaskDialogConfig.pszMainIcon = MAKEINTRESOURCE(IDR_MAINFRAME);
	tdcTaskDialogConfig.pszMainInstruction = strMainText;
	tdcTaskDialogConfig.cButtons = 2;
	tdcTaskDialogConfig.pButtons = tbButtons;
	tdcTaskDialogConfig.pfCallback = TaskDialogCallbackProc;

	if(GetWindowLong(hWnd, GWL_EXSTYLE) & WS_EX_LAYOUTRTL)
		tdcTaskDialogConfig.dwFlags |= TDF_RTL_LAYOUT;

	// User struct params
	sUserStruct.pTaskDialogConfig = &tdcTaskDialogConfig;
	sUserStruct.pButtons = tbButtons;
	sUserStruct.pNewButtons = tbNewButtons;

	int i = GetModuleFileName(NULL, sUserStruct.szSetupPath, MAX_PATH);
	while(i-- && sUserStruct.szSetupPath[i] != L'\\')
		/* loop */;
	sUserStruct.szSetupPath[i + 1] = L'\0';

	sUserStruct.bRunElevated = !CanAccessFolder(sUserStruct.szSetupPath, GENERIC_READ | GENERIC_WRITE);

	sUserStruct.bDownloading = FALSE;
	sUserStruct.pHost = L"ramensoftware.com";
	sUserStruct.pPath = L"/downloads/textify_setup.exe";

	tdcTaskDialogConfig.lpCallbackData = (LONG_PTR)&sUserStruct;

	// Main text
	CString strVersionInfoText;
	{
		strVersionInfoText.LoadString(IDS_UPDATE_VERSION_CURRENT);
		strVersionInfoText += L": " VER_FILE_VERSION_WSTR L"\n";

		CString str;
		str.LoadString(IDS_UPDATE_VERSION_NEW);
		strVersionInfoText += str;
		strVersionInfoText += L": ";
		strVersionInfoText += pVersion;
	}

	tdcTaskDialogConfig.pszContent = strVersionInfoText;

	// Changelog text
	CString strChangelogTitle;
	strChangelogTitle.LoadString(IDS_UPDATE_CHANGELOG);

	CString strChangelog;
	const char* pChangelog = pVersion + lstrlenA(pVersion) + 1;
	if(*pChangelog != '\0')
	{
		CA2W unicodeChangelog(pChangelog, CP_UTF8);
		strChangelog = unicodeChangelog.m_psz;

		tdcTaskDialogConfig.pszExpandedControlText = strChangelogTitle;
		tdcTaskDialogConfig.pszExpandedInformation = strChangelog;
	}

	// Show it
	pTaskDialogIndirect(&tdcTaskDialogConfig, NULL, NULL, NULL);
}

static HRESULT CALLBACK TaskDialogCallbackProc(HWND hWnd, UINT uNotification, WPARAM wParam, LPARAM lParam, LONG_PTR dwRefData)
{
	UTASKDIALOGSTRUCT* pUserStruct;

	pUserStruct = (UTASKDIALOGSTRUCT*)dwRefData;

	switch(uNotification)
	{
	case TDN_DIALOG_CONSTRUCTED:
		pUserStruct->hWnd = hWnd;

		SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

		if(pUserStruct->bRunElevated)
			SendMessage(hWnd, TDM_SET_BUTTON_ELEVATION_REQUIRED_STATE, IDOK, TRUE);
		break;

	case TDN_HYPERLINK_CLICKED:
		if((int)(UINT_PTR)ShellExecute(hWnd, NULL, (WCHAR*)lParam, NULL, NULL, SW_SHOWNORMAL) <= 32)
		{
			CString title;
			title.LoadString(IDS_ERROR);

			CString text;
			text.LoadString(IDS_ERROR_OPEN_ADDRESS);
			text += L"\n";
			text += (WCHAR*)lParam;

			MessageBox(hWnd, text, title, MB_ICONERROR);
		}
		break;

	case TDN_BUTTON_CLICKED:
		switch(wParam)
		{
		case IDOK:
			if(!UpdateQueue(hWnd, pUserStruct))
			{
				CString title;
				title.LoadString(IDS_ERROR);

				CString text;
				text.LoadString(IDS_UPDATE_ERROR_DOWNLOAD);

				MessageBox(hWnd, text, title, MB_ICONERROR);
			}
			return S_FALSE;

		case IDCANCEL:
			if(pUserStruct->bDownloading)
			{
				UpdateAbort();
				return S_FALSE;
			}
			break;
		}
		break;

	case TDN_NAVIGATED:
		if(pUserStruct->bRunElevated)
			SendMessage(hWnd, TDM_SET_BUTTON_ELEVATION_REQUIRED_STATE, IDOK, TRUE);

		if(!pUserStruct->bShowingProgressBar)
		{
			pUserStruct->bShowingProgressBar = TRUE;
			SendMessage(hWnd, TDM_ENABLE_BUTTON, IDOK, FALSE);
			SendMessage(hWnd, TDM_SET_PROGRESS_BAR_MARQUEE, TRUE, 0);
		}
		else
		{
			switch(GetRequestError())
			{
			case ERROR_SUCCESS:
				if(GetRequestHttpStatusCode() != 200)
				{
					WCHAR* pTextFormat = L"The server has reported an error (%u). Please try again later.";
					WCHAR* pText = (WCHAR*)HeapAlloc(GetProcessHeap(), 0,
						(lstrlen(pTextFormat) + (sizeof("1234567890") - 1) + 1) * sizeof(WCHAR));
					if(pText)
					{
						wsprintf(pText, pTextFormat, GetRequestHttpStatusCode());
						MessageBox(hWnd, pText, NULL, MB_ICONERROR);
						HeapFree(GetProcessHeap(), 0, pText);
					}
					else
						MessageBox(hWnd, L"(Allocation failed)", NULL, MB_ICONERROR);
				}
				else if(UpdateRunInstaller(pUserStruct->hWnd, pUserStruct->szTempFile, pUserStruct->szSetupPath, pUserStruct->bRunElevated))
					SendMessage(hWnd, TDM_CLICK_BUTTON, IDCANCEL, 0);
				else
					MessageBox(hWnd, L"Failed to launch installer", NULL, MB_ICONERROR);
				break;

			case ERROR_OPERATION_ABORTED:
				break;

			default:
			{
				CString title;
				title.LoadString(IDS_ERROR);

				CString text;
				text.LoadString(IDS_UPDATE_ERROR_DOWNLOAD);

				MessageBox(hWnd, text, title, MB_ICONERROR);

				break;
			}
			}
		}
		break;

	case TDN_VERIFICATION_CLICKED:
		SendMessage(hWnd, TDM_ENABLE_BUTTON, IDOK, !wParam);
		break;
	}

	return S_OK;
}
