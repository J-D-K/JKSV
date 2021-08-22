#pragma once

namespace ui
{
    void fmInit();
    void fmExit();
    void fmPrep(const FsSaveDataType& _type, const std::string& _dev, bool _commit);
    void fmUpdate();
    void fmDraw(SDL_Texture *target);
}
