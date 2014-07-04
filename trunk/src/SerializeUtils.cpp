// CommitMonitor - simple checker for new commits in svn repositories

// Copyright (C) 2007, 2012-2014 - Stefan Kueng

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

#include "stdafx.h"
#include "SerializeUtils.h"
#include <assert.h>
#include <memory>

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

bool CSerializeUtils::SaveString(FILE * hFile, std::string str)
{
    SerializeTypes type = SerializeType_String;
    if (fwrite(&type, sizeof(type), 1, hFile))
    {
        int length = (int)str.size();
        if (fwrite(&length, sizeof(length), 1, hFile))
        {
            if (fwrite(str.c_str(), sizeof(char), length, hFile)>=(size_t)length)
                return true;
        }
    }
    return false;
}

bool CSerializeUtils::SaveString(FILE * hFile, std::wstring str)
{
    return SaveString(hFile, CUnicodeUtils::StdGetUTF8(str));
}

bool CSerializeUtils::SaveBuffer(FILE * hFile, BYTE * pbData, size_t len)
{
    SerializeTypes type = SerializeType_Buffer;
    if (fwrite(&type, sizeof(type), 1, hFile))
    {
        int writelen = (int)len;
        if (fwrite(&writelen, sizeof(writelen), 1, hFile))
        {
            if (fwrite(pbData, sizeof(BYTE), len, hFile)>=len)
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
            int length = 0;
            if (fread(&length, sizeof(length), 1, hFile))
            {
                if (length)
                {
                    if (length < SERIALIZEBUFFERSIZE)
                    {
                        if (fread(buffer, sizeof(char), length, hFile))
                        {
                            str = std::string(buffer, length);
                            return true;
                        }
                    }
                    std::unique_ptr<char[]> pBuffer(new char[length]);
                    if (fread(pBuffer.get(), sizeof(char), length, hFile))
                    {
                        str = std::string(pBuffer.get(), length);
                        return true;
                    }
                }
                else
                {
                    str = std::string("");
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
        int length = *((int *)buf);
        buf += sizeof(int);
        if (length)
        {
            str = std::string((const char *)buf, length);
            buf += length;
            return true;
        }
        str = std::string("");
        return true;
    }
    return false;
}

bool CSerializeUtils::LoadString(FILE * hFile, std::wstring& str)
{
    std::string tempstr;
    if (LoadString(hFile, tempstr))
    {
        str = CUnicodeUtils::StdGetUnicode(tempstr);
        return true;
    }
    return false;
}

bool CSerializeUtils::LoadString(const unsigned char *& buf, std::wstring& str)
{
    SerializeTypes type = *((SerializeTypes*)buf);
    buf += sizeof(SerializeTypes);

    if (type == SerializeType_String)
    {
        int length = *((int *)buf);
        buf += sizeof(int);
        if (length)
        {
            int size = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)buf, (int)length, NULL, 0);
            if (size < SERIALIZEBUFFERSIZE)
            {
                int ret = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)buf, (int)length, wbuffer, size);
                str = std::wstring(wbuffer, ret);
            }
            else
            {
                std::unique_ptr<wchar_t[]> wide(new wchar_t[size+1]);
                int ret = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)buf, (int)length, wide.get(), size);
                str = std::wstring(wide.get(), ret);
            }
            buf += length;
            return true;
        }
        str = std::wstring(_T(""));
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
        int length = *((int *)buf);
        buf += sizeof(int);
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
