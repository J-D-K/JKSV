#pragma once

//Misc stuff for new menu code
typedef void (*funcPtr)(void *);
typedef struct
{
    bool running = false, finished = false;
    Thread *thrdPtr = NULL;
    void *argPtr = NULL;
    std::string status = "";
} threadInfo;
