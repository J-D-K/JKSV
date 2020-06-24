#ifndef UIUPDATE_H
#define UIUPDATE_H

//Contains declarations of ui updating functions

namespace ui
{
    void updateUserMenu(const uint64_t& down, const uint64_t& held);
    void drawUserMenu();

    void updateTitleMenu(const uint64_t& down, const uint64_t& held);
    void drawTitleMenu();

    void updateFolderMenu(const uint64_t& down, const uint64_t& held);
    void drawFolderMenu();

    void updateAdvMode(const uint64_t& down, const uint64_t& held);
    void drawAdvMode();

    //needed here since it uses static menu
    void folderMenuPrepare(data::user& usr, data::titledata& dat);
    void advCopyMenuPrep();
    void advModePrep(const std::string& svDev, const FsSaveDataType& _type, bool commitOnWrite);
}

#endif
