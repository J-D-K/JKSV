#include <switch.h>
#include <vector>

#include "ui.h"
#include "gfx.h"

static const SDL_Color darkenBack = {0x00, 0x00, 0x00, 0xBB};

ui::threadProcMngr::~threadProcMngr()
{
    for(threadInfo *t : threads)
    {
        threadWaitForExit(&t->thrd);
        threadClose(&t->thrd);
        delete t->status;
        delete t;
    }
}

threadInfo *ui::threadProcMngr::newThread(ThreadFunc func, void *args, funcPtr _drawfunc)
{
    threadInfo *t = new threadInfo;
    t->status = new threadStatus;
    t->finished = false;
    t->drawFunc = _drawfunc;
    t->argPtr = args;

    if(R_SUCCEEDED(threadCreate(&t->thrd, func, t, NULL, 0x20000, 0x2B, 1)))
    {
        mutexLock(&threadLock);
        threads.push_back(t);
        mutexUnlock(&threadLock);
        return threads[threads.size() - 1];
    }
    else
    {
        delete t->status;
        delete t;
    }
    return NULL;
}

void ui::threadProcMngr::update()
{
    if(!threads.empty())
    {
        threadInfo *t = threads[0];
        if(t->running == false && t->finished == false)
        {
            t->running = true;
            threadStart(&t->thrd);
        }
        else if(t->running == true && t->finished == true)
        {
            threadWaitForExit(&t->thrd);
            threadClose(&t->thrd);
            delete t->status;
            delete t;
            mutexLock(&threadLock);
            threads.erase(threads.begin());
            mutexUnlock(&threadLock);
        }
    }
}

void ui::threadProcMngr::draw()
{
    if(++frameCount % 4 == 0 && ++lgFrame > 7)
        lgFrame = 0;

    gfx::drawRect(NULL, &darkenBack, 0, 0, 1280, 720);
    gfx::drawTextf(NULL, 32, 56, 673, &ui::loadGlyphClr, loadGlyphArray[lgFrame].c_str());
    if(threads[0]->drawFunc)
        (*(threads[0]->drawFunc))(threads[0]);
    else
    {
        std::string gStatus;
        threads[0]->status->getStatus(gStatus);

        int statX = 640 - (gfx::getTextWidth(gStatus.c_str(), 18) / 2);
        gfx::drawTextf(NULL, 18, statX, 387, &ui::txtCont, gStatus.c_str());
    }
}
