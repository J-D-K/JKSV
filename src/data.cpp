#include <vector>
#include <string>
#include <cstring>
#include <fstream>
#include <algorithm>
#include <cstdio>
#include <switch.h>

#include "data.h"
#include "sys.h"
#include "file.h"
#include "util.h"

//Sorts titles sort-of alphabetically
static struct
{
	bool operator()(data::titledata& a, data::titledata& b)
	{
		for(unsigned i = 0; i < a.getTitle().length(); i++)
		{
			int charA = tolower(a.getTitle().c_str()[i]), charB = tolower(b.getTitle().c_str()[i]);
			if(charA != charB)
				return charA < charB;
		}

		return false;
	}
} sortTitles;

//Returns -1 for new
static int getUserIndex(const u128& id)
{
	for(unsigned i = 0; i < data::users.size(); i++)
	{
		if(data::users[i].getUID() == id)
			return i;
	}

	return -1;
}

namespace data
{
	titledata curData;
	user      curUser;
	std::vector<user> users;

	bool init()
	{
		Result res = 0;

		res = accountInitialize();
		if(res)
		{
			printf("accountInitialize failed!");
			return false;
		}

		return true;
	}

	bool fini()
	{
		accountExit();

		return true;
	}

	void loadDataInfo()
	{
		Result res = 0;
		FsSaveDataIterator saveIt;
		size_t total = 0;
		FsSaveDataInfo info;

		res = fsOpenSaveDataIterator(&saveIt, FsSaveDataSpaceId_All);
		if(R_FAILED(res))
		{
			printf("SaveDataIterator Failed\n");
			return;
		}

		while(true)
		{
			res = fsSaveDataIteratorRead(&saveIt, &info, 1, &total);
			if(R_FAILED(res) || total == 0)
				break;

			if((info.SaveDataType == FsSaveDataType_SaveData) || sys::sysSave)
			{
				int u = getUserIndex(info.userID);
				if(u == -1)
				{
					user newUser;
					if(newUser.init(info.userID) || (sys::sysSave && newUser.initNoChk(info.userID)))
					{
						users.push_back(newUser);

						u = getUserIndex(info.userID);
						titledata newData;
						if(newData.init(info) && (!sys::forceMountable || newData.isMountable(newUser.getUID())))
							users[u].titles.push_back(newData);
					}
				}
				else
				{
					titledata newData;
					if(newData.init(info) && (!sys::forceMountable || newData.isMountable(users[u].getUID())))
					{
						users[u].titles.push_back(newData);
					}
				}
			}
		}

		fsSaveDataIteratorClose(&saveIt);

		for(unsigned i = 0; i < users.size(); i++)
			std::sort(users[i].titles.begin(), users[i].titles.end(), sortTitles);

	}

	bool titledata::init(const FsSaveDataInfo& inf)
	{
		Result res = 0;
		NsApplicationControlData *dat = new NsApplicationControlData;
		std::memset(dat, 0, sizeof(NsApplicationControlData));
		NacpLanguageEntry *ent = NULL;

		if(inf.SaveDataType == FsSaveDataType_SaveData)
			id = inf.titleID;
		else if(inf.SaveDataType == FsSaveDataType_SystemSaveData)
			id = inf.saveID;

		uID = inf.userID;
		type = (FsSaveDataType)inf.SaveDataType;
		size_t outSz = 0;

		res = nsGetApplicationControlData(1, id, dat, sizeof(NsApplicationControlData), &outSz);
		if(R_FAILED(res) || outSz < sizeof(dat->nacp))
		{
			if(!sys::sysSave)
				printf("nsGetAppCtrlData Failed: 0x%08X\n", (unsigned)res);
			delete dat;
		}

		if(R_SUCCEEDED(res))
		{
			res = nacpGetLanguageEntry(&dat->nacp, &ent);
			if(R_FAILED(res) || ent == NULL)
			{
				printf("nacpGetLanguageEntry Failed\n");
				delete dat;
			}
		}

		if(R_SUCCEEDED(res))
		{
			title.assign(ent->name);
			titleSafe = util::safeString(ent->name);
			delete dat;
		}
		else
		{
			char tmp[32];
			sprintf(tmp, "%016lX", (u64)id);
			title.assign(tmp);
			titleSafe = util::safeString(tmp);
		}

		return true;
	}

	bool titledata::isMountable(const u128& uID)
	{
		data::user tmpUser;
		tmpUser.initNoChk(uID);
		if(fs::mountSave(tmpUser, *this))
		{
			fsdevUnmountDevice("sv");
			return true;
		}
		return false;
	}

	std::string titledata::getTitle()
	{
		return title;
	}

	std::string titledata::getTitleSafe()
	{
		return titleSafe;
	}

	uint64_t titledata::getID()
	{
		return id;
	}

	FsSaveDataType titledata::getType()
	{
		return type;
	}

	bool user::init(const u128& _id)
	{
		Result res = 0;
		userID = _id;

		AccountProfile prof;
		AccountProfileBase base;

		res = accountGetProfile(&prof, userID);
		if(R_FAILED(res))
			return false;

		res = accountProfileGet(&prof, NULL, &base);
		if(R_FAILED(res))
			return false;

		username.assign(base.username);
		if(username.empty())
			username = "Unknown";
		userSafe = util::safeString(username);

		accountProfileClose(&prof);

		return true;
	}

	bool user::initNoChk(const u128& _id)
	{
		Result res = 0;
		userID = _id;

		AccountProfile prof;
		AccountProfileBase base;

		res = accountGetProfile(&prof, userID);
		if(R_SUCCEEDED(res))
		{
			res = accountProfileGet(&prof, NULL, &base);
		}

		if(R_SUCCEEDED(res))
		{
			username.assign(base.username);
			userSafe = util::safeString(username);
			accountProfileClose(&prof);
		}
		else
		{
			username = "Unknown";
			userSafe = "Unknown";
		}

		return true;
	}

	u128 user::getUID()
	{
		return userID;
	}

	std::string user::getUsername()
	{
		return username;
	}

	std::string user::getUsernameSafe()
	{
		return userSafe;
	}
}
