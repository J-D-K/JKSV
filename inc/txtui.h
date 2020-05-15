#ifndef TXTUI_H
#define TXTUI_H

#include "data.h"

namespace ui
{
    void textUserPrep();
    void textTitlePrep(data::user& u);

    void textUserMenuUpdate(const uint64_t& down, const uint64_t& held);
    void textTitleMenuUpdate(const uint64_t& down, const uint64_t& held);
    void textFolderMenuUpdate(const uint64_t& down, const uint64_t& held);
    void updateExMenu(const uint64_t& down, const uint64_t& held);
    void exMenuPrep();

    //Options
    void optMenuInit();
    void updateOptMenu(const uint64_t& down, const uint64_t& held);
}

#endif
