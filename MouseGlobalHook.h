#pragma once

class MouseGlobalHook
{
public:
	MouseGlobalHook(CWindow wndNotify, UINT msgNotify,
		int key, bool ctrl, bool alt, bool shift,
		std::vector<CString> excludedPrograms);
	~MouseGlobalHook();

private:
	static DWORD WINAPI MouseHookThreadProxy(void* pParameter);
	DWORD MouseHookThread();

	static LRESULT CALLBACK LowLevelMouseProcProxy(int nCode, WPARAM wParam, LPARAM lParam);
	LRESULT LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam);
	bool IsCursorOnExcludedProgram(POINT pt);

	static MouseGlobalHook* volatile m_pThis; // for the mouse procedure

	CWindow m_wndNotify;
	UINT m_msgNotify;
	int m_mouseKey;
	bool m_ctrlKey, m_altKey, m_shiftKey;
	std::vector<CString> m_excludedPrograms;

	volatile HANDLE m_mouseHookThread;
	DWORD m_mouseHookThreadId;
	HANDLE m_mouseHookThreadReadyEvent;
	HHOOK m_lowLevelMouseHook;

	bool m_mouseDownEventHooked = false;
};
