#include "stdafx.h"
#include "URLEncode.h"

namespace
{
	CStringA UTF16toUTF8(const CStringW& utf16);
}

namespace URLEncoder
{
	CStringW Encode(const CStringW& str)
	{
		const CStringA utf8str = UTF16toUTF8(str);
		int utf8len = utf8str.GetLength();

		CStringW result;
		result.Preallocate(utf8len * 4);

		for(int i = 0; i < utf8len; i++)
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

		result.FreeExtra();
		return result;
	}
}

namespace
{
	CStringA UTF16toUTF8(const CStringW& utf16)
	{
		CStringA utf8;

		int len = WideCharToMultiByte(CP_UTF8, 0, utf16, utf16.GetLength(), NULL, 0, NULL, NULL);
		if(len > 1)
		{
			char* ptr = utf8.GetBuffer(len);
			WideCharToMultiByte(CP_UTF8, 0, utf16, utf16.GetLength(), ptr, len, NULL, NULL);
			utf8.ReleaseBuffer(len);
		}

		return utf8;
	}
}
