#pragma once
#include "svn_client.h"
#include <string>
#include <vector>

#include <iostream>
#include <fstream>
#include <string>

#pragma warning( push )
#pragma warning( disable : 4512 )
#pragma warning( disable : 4100 )

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include <boost/serialization/base_object.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/map.hpp>

#pragma warning( pop ) 

using namespace std;
using namespace boost;


class CUrlInfo
{
	friend class boost::serialization::access;
public:
	CUrlInfo(void);
	~CUrlInfo(void);

	wstring						username;
	wstring						password;

	wstring						url;
	__time64_t					lastchecked;
	svn_revnum_t				lastcheckedrev;

	int							minutesinterval;
	bool						fetchdiffs;

private:
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version)
	{
		ar & username;
		ar & password;
		ar & url;
		ar & lastchecked;
		ar & lastcheckedrev;
		ar & minutesinterval;
		ar & fetchdiffs;
	}
};

class CUrlInfos
{
	friend class boost::serialization::access;
public:
	CUrlInfos(void);
	~CUrlInfos(void);

	void						Save(LPCWSTR filename);
	void						Load(LPCWSTR filename);

	map<wstring,CUrlInfo>		infos;
private:
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version)
	{
		ar & infos;
	}
};

BOOST_CLASS_TRACKING(CUrlInfos, boost::serialization::track_never)
