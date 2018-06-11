#ifndef UTIL_H
#define UTIL_H

#include "data.h"

namespace util
{
	std::string getDateTime();
	void makeTitleDir(data::user& u, data::titledata& t);
	std::string getTitleDir(data::user& u, data::titledata& t);

	//Just returns string with '\n' inserted
	std::string getWrappedString(const std::string& s, const unsigned& sz, const unsigned& maxWidth);
}
#endif // UTIL_H
