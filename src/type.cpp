#include <switch.h>
#include <stdio.h>
#include <stdarg.h>

#include "type.h"

void threadStatus::setStatus(const char *fmt, ...)
{
    char tmp[1024];
    va_list args;
    va_start(args, fmt);
    vsprintf(tmp, fmt, args);
    va_end(args);

    mutexLock(&statusLock);
    status = tmp;
    mutexUnlock(&statusLock);
}

void threadStatus::getStatus(std::string& statusOut)
{
    mutexLock(&statusLock);
    statusOut = status;
    mutexUnlock(&statusLock);
}
