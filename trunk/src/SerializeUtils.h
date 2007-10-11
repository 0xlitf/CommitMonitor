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

	static bool SaveString(FILE * hFile, string str);
	static bool SaveString(FILE * hFile, wstring str);
	static bool LoadString(FILE * hFile, string& str);
	static bool LoadString(FILE * hFile, wstring& str);


	enum SerializeTypes
	{
		SerializeType_Number,
		SerializeType_String,
		SerializeType_Map
	};
};
