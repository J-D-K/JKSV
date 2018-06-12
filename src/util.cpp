#include <string>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>

#include "data.h"
#include "gfx.h"
#include "ui.h"

namespace util
{
	const std::string getDateTime()
	{
		char ret[48];

		time_t raw;
		time(&raw);
		tm *Time = localtime(&raw);

		sprintf(ret, "%04d-%02d-%02d@%02d-%02d-%02d", Time->tm_year + 1900, Time->tm_mon + 1, Time->tm_mday, Time->tm_hour, Time->tm_min, Time->tm_sec);

		return std::string(ret);
	}

	void makeTitleDir(data::user& u, data::titledata& t)
	{
		std::string path = u.getUsername() + "/" + t.getTitleSafe();
		mkdir(path.c_str(), 777);
	}

	std::string getTitleDir(data::user& u, data::titledata& t)
	{
		return std::string(u.getUsername() + "/" + t.getTitleSafe() + "/");
	}

	std::string getWrappedString(const std::string& s, const unsigned& sz, const unsigned& maxWidth)
	{
		if(gfx::getTextWidth(s, sz) < maxWidth)
			return s;

		std::string ret = "", tmp = "";
		unsigned first = 0, lastSpace = 0;

		for(unsigned i = 0; i < s.length(); i++)
		{
			tmp += s[i];

			if(s[i] == ' ')
				lastSpace = i;

			if(gfx::getTextWidth(tmp, sz) >= maxWidth)
			{
				tmp.assign(s, first, lastSpace - first);

				first = lastSpace + 1;
				ret += tmp + "\n";

				tmp.assign(s, first, i);
			}
			else if(i == s.length() - 1 && gfx::getTextWidth(tmp, sz) < maxWidth)
				ret += tmp;
		}

		return ret;
	}
}
