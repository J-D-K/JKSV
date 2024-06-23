#pragma once
#include <memory>
#include <switch.h>
#include "appStates/appState.hpp"
#include "system/timer.hpp"
#include "system/task.hpp"

class taskState : public appState
{
    public:
        taskState(sys::taskFunction threadFunction, std::shared_ptr<sys::taskArgs> args);
        ~taskState();

        void update(void);
        void render(void);

        // This is so progressState can inherit this function
        void renderLoadingGlyph(void);

    protected:
        // Task that is created and runs
        std::unique_ptr<sys::task> m_Task;
        // What frame of the glyph array we're on
        int m_GlyphFrame = 0;
        // Timer for updating loading glyph
        std::unique_ptr<sys::timer> m_LoadingGlyphTimer;
        // Bool for adding subtracting from color modifier
        bool m_ColorModifier = true;
        // Color modification for glyph
        uint8_t m_LoadingGlyphColorMod = 0x00;

};

void createAndPushNewTask(sys::taskFunction threadFunction, std::shared_ptr<sys::taskArgs> args);