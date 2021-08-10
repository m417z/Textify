#include "stdafx.h"
#include "resource.h"
#include "MainDlg.h"
#include "wow64ext/wow64ext.h"

CAppModule _Module;

namespace
{
	int RunApp(HINSTANCE hInstance);
	int CloseRunningApp(HINSTANCE hInstance);
	ATOM RegisterDialogClass(LPCTSTR lpszClassName, HINSTANCE hInstance);
	bool DoesParamExist(const WCHAR* pParam);
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR /*lpstrCmdLine*/, int /*nCmdShow*/)
{
	HRESULT hRes = ::OleInitialize(NULL); //::CoInitialize(NULL);
// If you are running on NT 4.0 or higher you can use the following call instead to 
// make the EXE free threaded. This means that calls come in on a random RPC thread.
//	HRESULT hRes = ::CoInitializeEx(NULL, COINIT_MULTITHREADED);
	ATLASSERT(SUCCEEDED(hRes));

	// this resolves ATL window thunking problem when Microsoft Layer for Unicode (MSLU) is used
	::DefWindowProc(NULL, 0, 0, 0L);

	AtlInitCommonControls(ICC_BAR_CLASSES);	// add flags to support other controls

	hRes = _Module.Init(NULL, hInstance);
	ATLASSERT(SUCCEEDED(hRes));

	int nRet;

	if(DoesParamExist(L"-exit"))
		nRet = CloseRunningApp(hInstance);
	else
		nRet = RunApp(hInstance);

	_Module.Term();
	::OleUninitialize(); //::CoUninitialize();

	return nRet;
}

namespace
{
	int RunApp(HINSTANCE hInstance)
	{
		CHandle hMutex(::CreateMutex(NULL, TRUE, L"textify_app"));
		if(hMutex && GetLastError() == ERROR_ALREADY_EXISTS)
		{
			CWindow wndRunning(::FindWindow(L"Textify", NULL));
			if(wndRunning)
			{
				::AllowSetForegroundWindow(wndRunning.GetWindowProcessID());

				if(DoesParamExist(L"-exit_if_running"))
					wndRunning.PostMessage(CMainDlg::UWM_EXIT);
				else
					wndRunning.PostMessage(CMainDlg::UWM_BRING_TO_FRONT);
			}

			return 0;
		}

		Wow64ExtInitialize();

		RegisterDialogClass(L"Textify", hInstance);
		RegisterDialogClass(L"TextifyEditDlg", hInstance);

		bool startHidden = DoesParamExist(L"-hidewnd");

		int nRet = 0;
		// BLOCK: Run application
		{
			CMainDlg dlgMain;
			nRet = dlgMain.DoModal(NULL, startHidden ? 1 : 0);
		}

		UnregisterClass(L"Textify", hInstance);
		UnregisterClass(L"TextifyEditDlg", hInstance);

		if(hMutex)
			::ReleaseMutex(hMutex);

		return nRet;
	}

	int CloseRunningApp(HINSTANCE hInstance)
	{
		CWindow wndRunning(::FindWindow(L"Textify", NULL));
		if(wndRunning)
		{
			::AllowSetForegroundWindow(wndRunning.GetWindowProcessID());
			wndRunning.PostMessage(CMainDlg::UWM_EXIT);
		}

		return 0;
	}

	ATOM RegisterDialogClass(LPCTSTR lpszClassName, HINSTANCE hInstance)
	{
		WNDCLASS wndcls;
		::GetClassInfo(hInstance, MAKEINTRESOURCE(32770), &wndcls);

		// Set our own class name
		wndcls.lpszClassName = lpszClassName;

		// Just register the class
		return ::RegisterClass(&wndcls);
	}

	bool DoesParamExist(const WCHAR* pParam)
	{
		for(int i = 1; i < __argc; i++)
		{
			if(_wcsicmp(__wargv[i], pParam) == 0)
			{
				return true;
			}
		}

		return false;
	}
}
