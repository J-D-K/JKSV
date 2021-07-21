#pragma once

//Misc stuff for new menu code
typedef void (*funcPtr)(void *);
typedef struct
{
    bool running = false, finished = false;
    Thread *thrdPtr = NULL;
    void *argPtr = NULL;
    Mutex statusLock = 0;
    std::string *status;

    void updateStatus(const std::string& newStatus)
    {
        mutexLock(&statusLock);
        *status = newStatus;
        mutexUnlock(&statusLock);
    }
} threadInfo;
