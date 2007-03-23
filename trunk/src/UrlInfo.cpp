#include "StdAfx.h"
#include "UrlInfo.h"

CUrlInfo::CUrlInfo(void) : lastchecked(0)
	, lastcheckedrev(0)
	, minutesinterval(0)
	, fetchdiffs(false)
{
}

CUrlInfo::~CUrlInfo(void)
{
}


CUrlInfos::CUrlInfos(void)
{
}

CUrlInfos::~CUrlInfos(void)
{
}

void CUrlInfos::Save(LPCWSTR filename)
{
	// create and open a character archive for output
	std::ofstream ofs(filename);

	boost::archive::text_oarchive oa(ofs);
	// write class instance to archive
	oa << *this;
}

void CUrlInfos::Load(LPCWSTR filename)
{
	// create and open an archive for input
	std::ifstream ifs(filename, std::ios::binary);
	boost::archive::text_iarchive ia(ifs);
	// read class state from archive
	ia >> *this;
}