#ifndef TXTUI_H
#define TXTUI_H

#include "data.h"

namespace ui
{
    void textUserPrep();
    void textTitlePrep(data::user& u);

    void textUserMenuUpdate(const uint64_t& down, const uint64_t& held);
    void drawTextUserMenu();

    void textTitleMenuUpdate(const uint64_t& down, const uint64_t& held);
    void drawTextTitleMenu();

    void textFolderMenuUpdate(const uint64_t& down, const uint64_t& held);
    void drawTextFolderMenu();

    void updateExMenu(const uint64_t& down, const uint64_t& held);
    void drawExMenu();

    void exMenuPrep();

    //Options
    void optMenuInit();
    void updateOptMenu(const uint64_t& down, const uint64_t& held);
    void drawOptMenu();
}

#endif
