#pragma once

#include <string>
#include <switch.h>

//Misc stuff for new menu code
typedef void (*funcPtr)(void *);

class threadStatus
{
    public:
        void setStatus(const char *fmt, ...);
        void getStatus(std::string& statusOut);

    private:
        Mutex statusLock = 0;
        std::string status;
};

typedef struct
{
    bool running = false, finished = false;
    Thread thrd;
    ThreadFunc thrdFunc;
    void *argPtr = NULL;
    funcPtr drawFunc = NULL;//Draw func is passed threadInfo pointer too
    threadStatus *status;
} threadInfo;
