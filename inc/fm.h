#pragma once

namespace ui
{
    void fmInit();
    void fmExit();
    void fmPrep(const FsSaveDataType& _type, const std::string& _dev, bool _commit);
    void fmUpdate(SDL_Texture *target);
    void fmDraw();
}
