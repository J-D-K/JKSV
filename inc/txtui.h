#ifndef TXTUI_H
#define TXTUI_H

#include "data.h"

namespace ui
{
    void textUserPrep();
    void textTitlePrep(data::user& u);

    void textUserMenuUpdate(const uint64_t& down, const uint64_t& held, const touchPosition& p);
    void textTitleMenuUpdate(const uint64_t& down, const uint64_t& held, const touchPosition& p);
    //I don't think this matched very well
    void textFolderMenuUpdate(const uint64_t& down, const uint64_t& held, const touchPosition& p);

    void updateExMenu(const uint64_t& down, const uint64_t& held, const touchPosition& p);
    void exMenuPrep();

    //Options
    void optMenuInit();
    void updateOptMenu(const uint64_t& down, const uint64_t& held, const touchPosition& p);
}

#endif
