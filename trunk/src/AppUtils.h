#pragma once

#include <string>

using namespace std;

class CAppUtils
{
public:
	CAppUtils(void);
	~CAppUtils(void);

	static wstring					GetAppDataDir();
};
