// CommitMonitor - simple checker for new commits in svn repositories

// Copyright (C) 2007 - Stefan Kueng

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
#include "StdAfx.h"
#include "SerializeUtils.h"
#include <assert.h>

char CSerializeUtils::buffer[SERIALIZEBUFFERSIZE] = {0};
wchar_t CSerializeUtils::wbuffer[SERIALIZEBUFFERSIZE] = {0};

CSerializeUtils::CSerializeUtils(void)
{
}

CSerializeUtils::~CSerializeUtils(void)
{
}

bool CSerializeUtils::SaveNumber(FILE * hFile, unsigned __int64 value)
{
	SerializeTypes type = SerializeType_Number;
    if (fwrite(&type, sizeof(type), 1, hFile))
	{
        if (fwrite(&value, sizeof(value), 1, hFile))
		{
			return true;
		}
	}
	return false;
}

bool CSerializeUtils::LoadNumber(FILE * hFile, unsigned __int64& value)
{
	SerializeTypes type;
    if (fread(&type, sizeof(type), 1, hFile))
	{
		if (type == SerializeType_Number)
		{
            if (fread(&value, sizeof(value), 1, hFile))
			{
				return true;
			}
		}
	}
	return false;
}

bool CSerializeUtils::LoadNumber(const unsigned char *& buf, unsigned __int64& value)
{
	SerializeTypes type = *((SerializeTypes*)buf);
	buf += sizeof(SerializeTypes);

	if (type == SerializeType_Number)
	{
		value = *((unsigned __int64 *)buf);
		buf += sizeof(unsigned __int64);
		return true;
	}
	return false;
}

bool CSerializeUtils::SaveString(FILE * hFile, string str)
{
	SerializeTypes type = SerializeType_String;
    if (fwrite(&type, sizeof(type), 1, hFile))
	{
		size_t length = str.size();
        if (fwrite(&length, sizeof(length), 1, hFile))
		{
            if (fwrite(str.c_str(), sizeof(char), length, hFile)>=0)
				return true;
		}
	}
	return false;
}
bool CSerializeUtils::SaveString(FILE * hFile, wstring str)
{
	return SaveString(hFile, CUnicodeUtils::StdGetUTF8(str));
}

bool CSerializeUtils::SaveBuffer(FILE * hFile, BYTE * pbData, size_t len)
{
	SerializeTypes type = SerializeType_Buffer;
	if (fwrite(&type, sizeof(type), 1, hFile))
	{
		if (fwrite(&len, sizeof(len), 1, hFile))
		{
			if (fwrite(pbData, sizeof(BYTE), len, hFile)>=0)
				return true;
		}
	}
	return false;
}

bool CSerializeUtils::LoadString(FILE * hFile, std::string &str)
{
	SerializeTypes type;
    if (fread(&type, sizeof(type), 1, hFile))
	{
		if (type == SerializeType_String)
		{
			size_t length = 0;
            if (fread(&length, sizeof(length), 1, hFile))
			{
				if (length)
				{
					if (length < SERIALIZEBUFFERSIZE)
					{
						if (fread(buffer, sizeof(char), length, hFile))
						{
							str = string(buffer, length);
							return true;
						}
					}
					char * pBuffer = new char[length];
					if (fread(pBuffer, sizeof(char), length, hFile))
					{
						str = string(pBuffer, length);
						delete [] pBuffer;
						return true;
					}
					delete [] pBuffer;
				}
				else
				{
					str = string("");
					return true;
				}
			}
		}
	}
	return false;
}

bool CSerializeUtils::LoadString(const unsigned char *& buf, std::string &str)
{
	SerializeTypes type = *((SerializeTypes*)buf);
	buf += sizeof(SerializeTypes);

	if (type == SerializeType_String)
	{
		size_t length = *((size_t *)buf);
		buf += sizeof(size_t);
		if (length)
		{
			str = string((const char *)buf, length);
			buf += length;
			return true;
		}
		str = string("");
		return true;
	}
	return false;
}

bool CSerializeUtils::LoadString(FILE * hFile, wstring& str)
{
	string tempstr;
	if (LoadString(hFile, tempstr))
	{
		str = CUnicodeUtils::StdGetUnicode(tempstr);
		return true;
	}
	return false;
}

bool CSerializeUtils::LoadString(const unsigned char *& buf, wstring& str)
{
	SerializeTypes type = *((SerializeTypes*)buf);
	buf += sizeof(SerializeTypes);

	if (type == SerializeType_String)
	{
		size_t length = *((size_t *)buf);
		buf += sizeof(size_t);
		if (length)
		{
			int size = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)buf, length, NULL, 0);
			if (size < SERIALIZEBUFFERSIZE)
			{
				int ret = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)buf, length, wbuffer, size);
				str = wstring(wbuffer, ret);
			}
			else
			{
				wchar_t * wide = new wchar_t[size+1];
				int ret = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)buf, length, wide, size);
				str = wstring(wide, ret);
				delete [] wide;
			}
			buf += length;
			return true;
		}
		str = wstring(_T(""));
		return true;
	}
	return false;
}

bool CSerializeUtils::LoadBuffer(const unsigned char *& buf, BYTE *& pbData, size_t & len)
{
	SerializeTypes type = *((SerializeTypes*)buf);
	buf += sizeof(SerializeTypes);

	if (type == SerializeType_Buffer)
	{
		size_t length = *((size_t *)buf);
		buf += sizeof(size_t);
		if (length)
		{
			pbData = new BYTE[length];
			memcpy(pbData, buf, length);
			len = length;
			buf += length;
			return true;
		}
		len = 0;
		pbData = NULL;
		return true;
	}
	return false;
}
