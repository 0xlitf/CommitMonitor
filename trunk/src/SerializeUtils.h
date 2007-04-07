#pragma once
#include <string>

#include "UnicodeUtils.h"

using namespace std;

class CSerializeUtils
{
public:
	CSerializeUtils(void);
	~CSerializeUtils(void);

	static bool SaveNumber(HANDLE hFile, unsigned __int64 value);
	static bool LoadNumber(HANDLE hFile, unsigned __int64& value);

	static bool SaveString(HANDLE hFile, string str);
	static bool SaveString(HANDLE hFile, wstring str);
	static bool LoadString(HANDLE hFile, string& str);
	static bool LoadString(HANDLE hFile, wstring& str);


	enum SerializeTypes
	{
		SerializeType_Number,
		SerializeType_String,
		SerializeType_Map
	};
};
