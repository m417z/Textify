#include "stdafx.h"
#include "URLEncode.h"

namespace URLEncoder
{
	CString Encode(const CString& str)
	{
		CW2A utf8str(str, CP_UTF8);

		CString result;

		for(int i = 0; utf8str[i] != L'\0'; i++)
		{
			char c = utf8str[i];

			if(
				(c >= 'a' && c <= 'z') ||
				(c >= 'A' && c <= 'Z') ||
				(c >= '0' && c <= '9') ||
				c == '-' || c == '_' || c == '.' || c == '~'
				)
			{
				result += c;
			}
			else
			{
				result.AppendFormat(L"%%%02X", (DWORD)(BYTE)c);
			}
		}

		return result;
	}
}
