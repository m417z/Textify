#include "stdafx.h"
#include "MouseGlobalHook.h"

// This static pointer is used for the mouse procedure.
// As a result, only one instance can be used.
MouseGlobalHook* volatile MouseGlobalHook::m_pThis;

MouseGlobalHook::MouseGlobalHook(CWindow wndNotify, UINT msgNotify,
	int key, bool ctrl, bool alt, bool shift) :
	m_wndNotify{ wndNotify }, m_msgNotify{ msgNotify },
	m_mouseKey{ key }, m_ctrlKey{ ctrl }, m_altKey{ alt }, m_shiftKey{ shift }
{
	if(_InterlockedCompareExchangePointer(
		reinterpret_cast<void* volatile*>(&m_pThis), this, nullptr))
	{
		AtlThrow(E_FAIL);
	}

	CHandle readyEvent{ CreateEvent(NULL, TRUE, FALSE, NULL) };
	if(!readyEvent)
		AtlThrow(AtlHresultFromLastError());

	m_mouseHookThreadReadyEvent = readyEvent;

	m_mouseHookThread = CreateThread(NULL, 0, MouseHookThreadProxy, this, CREATE_SUSPENDED, &m_mouseHookThreadId);
	if(!m_mouseHookThread)
		AtlThrow(AtlHresultFromLastError());

	SetThreadPriority(m_mouseHookThread, THREAD_PRIORITY_TIME_CRITICAL);
	ResumeThread(m_mouseHookThread);

	WaitForSingleObject(readyEvent, INFINITE);
}

MouseGlobalHook::~MouseGlobalHook()
{
	HANDLE hThread = _InterlockedExchangePointer(&m_mouseHookThread, nullptr);
	if(hThread)
	{
		PostThreadMessage(m_mouseHookThreadId, WM_APP, 0, 0);
		WaitForSingleObject(hThread, INFINITE);
		CloseHandle(hThread);
	}

	m_pThis = NULL;
}

void MouseGlobalHook::SetNewHotkey(int key, bool ctrl, bool alt, bool shift)
{
	// Note: the change is not atomic, but that's probably good enough.
	m_mouseKey = key;
	m_ctrlKey = ctrl;
	m_altKey = alt;
	m_shiftKey = shift;
}

DWORD WINAPI MouseGlobalHook::MouseHookThreadProxy(void* pParameter)
{
	auto pThis = reinterpret_cast<MouseGlobalHook*>(pParameter);
	return pThis->MouseHookThread();
}

DWORD MouseGlobalHook::MouseHookThread()
{
	MSG msg;
	PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);
	SetEvent(m_mouseHookThreadReadyEvent);
	m_mouseHookThreadReadyEvent = NULL;

	m_lowLevelMouseHook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProcProxy, GetModuleHandle(NULL), 0);
	if(m_lowLevelMouseHook)
	{
		BOOL bRet;
		while((bRet = GetMessage(&msg, NULL, 0, 0)) != 0)
		{
			if(bRet == -1)
			{
				msg.wParam = 0;
				break;
			}

			if(msg.hwnd == NULL && msg.message == WM_APP)
			{
				PostQuitMessage(0);
				continue;
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		UnhookWindowsHookEx(m_lowLevelMouseHook);
	}
	else
		msg.wParam = 0;

	HANDLE hThread = _InterlockedExchangePointer(&m_mouseHookThread, nullptr);
	if(hThread)
	{
		CloseHandle(hThread);
	}

	return static_cast<DWORD>(msg.wParam);
}

LRESULT CALLBACK MouseGlobalHook::LowLevelMouseProcProxy(int nCode, WPARAM wParam, LPARAM lParam)
{
	return m_pThis->LowLevelMouseProc(nCode, wParam, lParam);
}

LRESULT MouseGlobalHook::LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if(nCode == HC_ACTION)
	{
		bool handleEvent = false;

		switch(wParam)
		{
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
			handleEvent = true;
			break;
		}

		if(handleEvent)
		{
			MSLLHOOKSTRUCT* msllHookStruct = reinterpret_cast<MSLLHOOKSTRUCT*>(lParam);

			bool hotkeyDownDetected = false;
			bool hotkeyUpDetected = false;

			bool bModifiersMatch =
				(GetKeyState(VK_CONTROL) < 0) == m_ctrlKey &&
				(GetKeyState(VK_MENU) < 0) == m_altKey &&
				(GetKeyState(VK_SHIFT) < 0) == m_shiftKey;

			if(bModifiersMatch)
			{
				switch(wParam)
				{
				case WM_LBUTTONDOWN:
					if(m_mouseKey == VK_LBUTTON)
						hotkeyDownDetected = true;
					break;

				case WM_LBUTTONUP:
					if(m_mouseKey == VK_LBUTTON)
						hotkeyUpDetected = true;
					break;

				case WM_RBUTTONDOWN:
					if(m_mouseKey == VK_RBUTTON)
						hotkeyDownDetected = true;
					break;

				case WM_RBUTTONUP:
					if(m_mouseKey == VK_RBUTTON)
						hotkeyUpDetected = true;
					break;

				case WM_MBUTTONDOWN:
					if(m_mouseKey == VK_MBUTTON)
						hotkeyDownDetected = true;
					break;

				case WM_MBUTTONUP:
					if(m_mouseKey == VK_MBUTTON)
						hotkeyUpDetected = true;
					break;
				}
			}

			if(hotkeyDownDetected)
			{
				m_mouseDownEventHooked = true;
				return 1;
			}

			if(hotkeyUpDetected && m_mouseDownEventHooked)
			{
				m_mouseDownEventHooked = false;
				m_wndNotify.PostMessage(m_msgNotify, msllHookStruct->pt.x, msllHookStruct->pt.y);
				return 1;
			}

			m_mouseDownEventHooked = false;
		}
	}

	return CallNextHookEx(m_lowLevelMouseHook, nCode, wParam, lParam);
}
