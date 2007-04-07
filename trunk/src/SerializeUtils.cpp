#include "StdAfx.h"
#include "SerializeUtils.h"
#include <assert.h>

CSerializeUtils::CSerializeUtils(void)
{
}

CSerializeUtils::~CSerializeUtils(void)
{
}

bool CSerializeUtils::SaveNumber(HANDLE hFile, unsigned __int64 value)
{
	DWORD written = 0;
	SerializeTypes type = SerializeType_Number;
	if (WriteFile(hFile, &type, sizeof(type), &written, NULL))
	{
		if (WriteFile(hFile, &value, sizeof(value), &written, NULL))
		{
			return true;
		}
	}
	return false;
}

bool CSerializeUtils::LoadNumber(HANDLE hFile, unsigned __int64& value)
{
	DWORD read = 0;
	SerializeTypes type;
	if (ReadFile(hFile, &type, sizeof(type), &read, NULL))
	{
		if (type == SerializeType_Number)
		{
			if (ReadFile(hFile, &value, sizeof(value), &read, NULL))
			{
				return true;
			}
		}
	}
	return false;
}

bool CSerializeUtils::SaveString(HANDLE hFile, string str)
{
	DWORD written = 0;
	SerializeTypes type = SerializeType_String;
	if (WriteFile(hFile, &type, sizeof(type), &written, NULL))
	{
		size_t length = str.size();
		assert(length < (1024*100));
		if (WriteFile(hFile, &length, sizeof(length), &written, NULL))
		{
			if (WriteFile(hFile, str.c_str(), length, &written, NULL))
				return true;
		}
	}
	return false;
}
bool CSerializeUtils::SaveString(HANDLE hFile, wstring str)
{
	return SaveString(hFile, CUnicodeUtils::StdGetUTF8(str));
}

bool CSerializeUtils::LoadString(HANDLE hFile, std::string &str)
{
	DWORD read = 0;
	SerializeTypes type;
	if (ReadFile(hFile, &type, sizeof(type), &read, NULL))
	{
		if (type == SerializeType_String)
		{
			size_t length = 0;
			if (ReadFile(hFile, &length, sizeof(length), &read, NULL))
			{
				if (length < (1024*100))
				{
					char * buffer = new char[length];
					if (ReadFile(hFile, buffer, length, &read, NULL))
					{
						str = string(buffer, length);
						delete [] buffer;
						return true;
					}
					delete [] buffer;
				}
			}
		}
	}
	return false;
}

bool CSerializeUtils::LoadString(HANDLE hFile, wstring& str)
{
	string tempstr;
	if (LoadString(hFile, tempstr))
	{
		str = CUnicodeUtils::StdGetUnicode(tempstr);
		return true;
	}
	return false;
}