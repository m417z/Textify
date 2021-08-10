#include "stdafx.h"
#include "InjectAllowSetForegroundWindow.h"
#include "wow64ext/wow64ext.h"

static BOOL InjectFromWOW64AllowSetForegroundWindow(HANDLE hProcess, DWORD dwProcessId, DWORD dwTimeout);

BOOL InjectAllowSetForegroundWindow(DWORD dwProcessId, DWORD dwTimeout)
{
#ifdef _WIN64
#error InjectAllowSetForegroundWindow assumes to be running from a 32-bit process
#endif // _WIN64

	HWND hForegroundWnd = GetForegroundWindow();
	if(!hForegroundWnd)
		return AllowSetForegroundWindow(dwProcessId);

	DWORD dwForegroundProcessId = 0;
	if(!GetWindowThreadProcessId(hForegroundWnd, &dwForegroundProcessId) ||
		!dwForegroundProcessId)
		return FALSE;

	if(dwForegroundProcessId == dwProcessId)
		return TRUE;

	if(dwForegroundProcessId == GetCurrentProcessId())
		return AllowSetForegroundWindow(dwProcessId);

	BOOL bTargetProcessWin64 = FALSE;

	HANDLE hProcess = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
		PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, dwForegroundProcessId);
	if(!hProcess)
		return TRUE;

	BOOL bIsWow64;
	if(IsWow64Process(hProcess, &bIsWow64) && !bIsWow64)
	{
		SYSTEM_INFO si;
		GetNativeSystemInfo(&si);
		if(si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
			bTargetProcessWin64 = TRUE;
	}

	BOOL bSucceeded;

	if(bTargetProcessWin64)
	{
		bSucceeded = InjectFromWOW64AllowSetForegroundWindow(hProcess, dwProcessId, dwTimeout);
	}
	else
	{
		bSucceeded = FALSE;

		HMODULE hUser32 = GetModuleHandle(L"user32.dll");
		if(hUser32)
		{
			FARPROC pAllowSetForegroundWindow = GetProcAddress(hUser32, "AllowSetForegroundWindow");
			if(pAllowSetForegroundWindow)
			{
				HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0,
					(LPTHREAD_START_ROUTINE)pAllowSetForegroundWindow,
					(void*)dwProcessId, 0, NULL);
				if(hThread)
				{
					if(WaitForSingleObject(hThread, dwTimeout) == WAIT_OBJECT_0)
						bSucceeded = TRUE;

					CloseHandle(hThread);
				}
			}
		}
	}

	CloseHandle(hProcess);

	return bSucceeded;
}

static BOOL InjectFromWOW64AllowSetForegroundWindow(HANDLE hProcess, DWORD dwProcessId, DWORD dwTimeout)
{
	// Grabbed from the compiled binary.
	static const char pszNaked64AllowSetForegroundWindow[] =
		"\x48\x89\x5C\x24\x08\x48\x89\x6C\x24\x10\x48\x89\x74\x24\x18\x57"
		"\x41\x54\x41\x55\x41\x56\x41\x57\x48\x83\xEC\x20\x65\x48\x8B\x04"
		"\x25\x60\x00\x00\x00\x33\xFF\x48\x8B\xE9\x4C\x8B\x78\x18\x4D\x8B"
		"\x77\x20\x49\x83\xC7\x20\x4D\x3B\xF7\x0F\x84\xE1\x00\x00\x00\x41"
		"\xBC\xFF\xFF\x00\x00\x44\x8D\x6F\x01\x0F\x1F\x80\x00\x00\x00\x00"
		"\x49\x8B\x56\x50\x45\x0F\xB7\x46\x48\x33\xC9\x0F\x1F\x44\x00\x00"
		"\xC1\xC9\x0D\x0F\xB6\x02\x3C\x61\x72\x0A\x0F\xB6\xC0\x83\xE8\x20"
		"\x48\x98\xEB\x03\x0F\xB6\xC0\x48\x03\xC8\x48\xFF\xC2\x66\x45\x03"
		"\xC4\x75\xDD\x81\xF9\x83\x42\xC8\x63\x0F\x85\x85\x00\x00\x00\x4D"
		"\x8B\x5E\x20\x41\x0F\xB7\xDD\x49\x63\x43\x3C\x42\x8B\xB4\x18\x88"
		"\x00\x00\x00\x49\x03\xF3\x44\x8B\x4E\x20\x44\x8B\x56\x24\x44\x8B"
		"\x46\x18\x4D\x03\xCB\x4D\x03\xD3\x45\x85\xC0\x74\x52\x41\x8B\x11"
		"\x49\x03\xD3\x33\xC0\x0F\xB6\x0A\x0F\x1F\x84\x00\x00\x00\x00\x00"
		"\xC1\xC8\x0D\x0F\xBE\xC9\x48\x8D\x52\x01\x03\xC1\x0F\xB6\x0A\x84"
		"\xC9\x75\xED\x3D\xB8\xE4\x6C\x5D\x75\x15\x8B\x46\x1C\x41\x0F\xB7"
		"\x12\x49\x8D\x0C\x03\x8B\x3C\x91\x49\x03\xFB\x66\x41\x03\xDC\x49"
		"\x83\xC1\x04\x49\x83\xC2\x02\x41\xFF\xC8\x66\x85\xDB\x75\xA9\x48"
		"\x85\xFF\x75\x2B\x4D\x8B\x36\x4D\x3B\xF7\x0F\x85\x30\xFF\xFF\xFF"
		"\x33\xC0\x48\x8B\x5C\x24\x50\x48\x8B\x6C\x24\x58\x48\x8B\x74\x24"
		"\x60\x48\x83\xC4\x20\x41\x5F\x41\x5E\x41\x5D\x41\x5C\x5F\xC3\x8B"
		"\xCD\xFF\xD7\xEB\xDD";

	BOOL bSucceeded = FALSE;

	void* pRemoteCodeBuffer = VirtualAllocEx(hProcess, NULL,
		sizeof(pszNaked64AllowSetForegroundWindow) - 1, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if(pRemoteCodeBuffer)
	{
		BOOL bCanFreeMemory = TRUE;

		BOOL bWritten = WriteProcessMemory(hProcess, pRemoteCodeBuffer,
			pszNaked64AllowSetForegroundWindow, sizeof(pszNaked64AllowSetForegroundWindow) - 1, NULL);
		if(bWritten)
		{
			DWORD64 hThread64 = MyCreateRemoteThread64(
				(DWORD64)hProcess, (DWORD64)(LPTHREAD_START_ROUTINE)pRemoteCodeBuffer,
				(DWORD64)(void*)dwProcessId);
			if(hThread64)
			{
				bCanFreeMemory = FALSE; // don't free if we time out or fail

				HANDLE hThread = (HANDLE)hThread64;

				if(WaitForSingleObject(hThread, dwTimeout) == WAIT_OBJECT_0)
				{
					bCanFreeMemory = TRUE; // thread ended, can free the code

					DWORD dwExitCode = 0;
					if(GetExitCodeThread(hThread, &dwExitCode) && dwExitCode)
						bSucceeded = TRUE;
				}

				CloseHandle64(hThread64);
			}
		}

		if(bCanFreeMemory)
			VirtualFreeEx(hProcess, pRemoteCodeBuffer, 0, MEM_RELEASE);
	}

	return bSucceeded;
}
