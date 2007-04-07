#pragma once

#include <string>
#include "svn_time.h"

using namespace std;

class CAppUtils
{
public:
	CAppUtils(void);
	~CAppUtils(void);

	static wstring					GetAppDataDir();
	static wstring					ConvertDate(apr_time_t time);
};
