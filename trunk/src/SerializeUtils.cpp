#include "StdAfx.h"
#include "SerializeUtils.h"
#include <assert.h>


char * CSerializeUtils::buffer = NULL;
size_t CSerializeUtils::lenbuffer = 0;

CSerializeUtils::CSerializeUtils(void)
{
}

CSerializeUtils::~CSerializeUtils(void)
{
}

void CSerializeUtils::InitializeStatic()
{
    buffer = new char[1024];
    lenbuffer = 1024;
}

void CSerializeUtils::CleanupStatic()
{
    if (buffer)
        delete [] buffer;
    lenbuffer = 0;
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
                        if (length > lenbuffer)
                        {
                            delete [] buffer;
                            buffer = new char[length];
                            lenbuffer = length;
                        }
                        if (fread(buffer, sizeof(char), length, hFile))
                        {
                            str = string(buffer, length);
                            return true;
                        }
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