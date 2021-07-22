#pragma once

#include <string>
#include <switch.h>

//Misc stuff for new menu code
typedef void (*funcPtr)(void *);

typedef struct
{
    Mutex statusLock = 0;
    std::string status;
    void setStatus(const std::string& newStatus)
    {
        mutexLock(&statusLock);
        status = newStatus;
        mutexUnlock(&statusLock);
    }

    void getStatus(std::string& statusOut)
    {
        mutexLock(&statusLock);
        statusOut = status;
        mutexUnlock(&statusLock);
    }

} threadStatus;

typedef struct
{
    bool running = false, finished = false;
    Thread thrd;
    void *argPtr = NULL;
    funcPtr drawFunc = NULL;//Draw func is passed threadInfo pointer too
    threadStatus *status;
} threadInfo;
