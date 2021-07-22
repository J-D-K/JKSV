#pragma once

#include "type.h"

namespace ui
{
    class threadProcMngr
    {
        public:
            ~threadProcMngr();
            //Draw function is used and called to draw on overlay
            threadInfo *newThread(ThreadFunc func, void *args, funcPtr _drawFunc);
            void update();
            void draw();
            bool empty() { return threads.empty(); }

        private:
            std::vector<threadInfo *> threads;
            uint8_t lgFrame = 0;
            unsigned frameCount = 0;
            Mutex threadLock = 0;
    };
}
