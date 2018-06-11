#ifndef UTIL_H
#define UTIL_H

#include "data.h"

namespace util
{
    const std::string getDateTime();
    void makeTitleDir(data::user& u, data::titledata& t);
    const std::string getTitleDir(data::user& u, data::titledata& t);
}
#endif // UTIL_H
