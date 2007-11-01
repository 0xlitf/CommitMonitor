#include "StdAfx.h"
#include "SerializeUtils.h"
#include <assert.h>

char CSerializeUtils::buffer[4096] = {0};

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
		assert(length < (1024*100));
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
				if (length < (1024*100))
				{
                    if (length)
                    {
                        if (length < 4096)
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
		if (length < (1024*100))
		{
			if (length)
			{
				str = string((const char *)buf, length);
				buf += length;
				return true;
			}
			str = string("");
			return true;
		}
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
		if (length < (1024*100))
		{
			if (length)
			{
				int size = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)buf, length, NULL, 0);
				if (size < 4096)
				{
					TCHAR wide[4096];
					int ret = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)buf, length, wide, size);
					str = wstring(wide, ret);
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
	}
	return false;
}