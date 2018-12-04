#ifndef CLSUI_H
#define CLSUI_H

#include "data.h"

namespace ui
{
    void clsUserPrep();
    void clsTitlePrep(data::user& u);

    void classicUserMenuUpdate(const uint64_t& down, const uint64_t& held, const touchPosition& p);
    void classicTitleMenuUpdate(const uint64_t& down, const uint64_t& held, const touchPosition& p);
    //I don't think this matched very well
    void classicFolderMenuUpdate(const uint64_t& down, const uint64_t& held, const touchPosition& p);

    void updateExMenu(const uint64_t& down, const uint64_t& held, const touchPosition& p);
    void exMenuPrep();
}

#endif // CLSUI_H
