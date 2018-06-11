#include <string>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>

#include "data.h"

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

    const std::string getTitleDir(data::user& u, data::titledata& t)
    {
        return std::string(u.getUsername() + "/" + t.getTitleSafe() + "/");
    }
}
