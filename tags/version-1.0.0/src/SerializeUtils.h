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
#pragma once
#include <string>

#include "UnicodeUtils.h"

using namespace std;

class CSerializeUtils
{
public:
	CSerializeUtils(void);
	~CSerializeUtils(void);

	static bool SaveNumber(FILE * hFile, unsigned __int64 value);
	static bool LoadNumber(FILE * hFile, unsigned __int64& value);
	static bool LoadNumber(const unsigned char *& buf, unsigned __int64& value);

	static bool SaveString(FILE * hFile, string str);
	static bool SaveString(FILE * hFile, wstring str);
	static bool LoadString(FILE * hFile, string& str);
	static bool LoadString(const unsigned char *& buf, string& str);
	static bool LoadString(FILE * hFile, wstring& str);
	static bool LoadString(const unsigned char *& buf, wstring& str);

	enum SerializeTypes
	{
		SerializeType_Number,
		SerializeType_String,
		SerializeType_Map
	};
private:
    static char buffer[4096];
};
