#ifndef UTIL_H
#define UTIL_H

#include "data.h"

namespace util
{
	//Returns string with date + time
	std::string getDateTime();

	//Creates Dir 'JKSV/[user]/[title]
	void makeTitleDir(data::user& u, data::titledata& t);

	//Returns 'JKSV/[user]/[title]/'
	std::string getTitleDir(data::user& u, data::titledata& t);

	//Just returns string with '\n' inserted. Doesn't work as intended yet.
	std::string getWrappedString(const std::string& s, const unsigned& sz, const unsigned& maxWidth);
}
#endif // UTIL_H
