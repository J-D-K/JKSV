#pragma once
#include <fstream>

namespace logger
{
    void init(void);
    void log(const char *format, ...);   
}
