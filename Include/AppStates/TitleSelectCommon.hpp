#pragma once
#include "AppStates/AppState.hpp"

class TitleSelectCommon : public AppState
{
    public:
        TitleSelectCommon(void);
        virtual ~TitleSelectCommon() {};

        virtual void Update(void) = 0;
        virtual void Render(void) = 0;

        virtual void Refresh(void) = 0;

        void RenderControlGuide(void);

    protected:
        // X coordinate for control guide. Shared between all instances. Only should be calculated once.
        static inline int m_TitleControlsX = 0;
};
