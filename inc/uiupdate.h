#ifndef UIUPDATE_H
#define UIUPDATE_H

//Contains declarations of ui updating functions

namespace ui
{
    void updateUserMenu(const uint64_t& down, const uint64_t& held, const touchPosition& p);
    void updateTitleMenu(const uint64_t& down, const uint64_t& held, const touchPosition& p);
    void updateFolderMenu(const uint64_t& down, const uint64_t& held, const touchPosition& p);
    void updateAdvMode(const uint64_t& down, const uint64_t& held, const touchPosition& p);

    //needed here since it uses static menu
    void folderMenuPrepare(data::user& usr, data::titledata& dat);
    void advCopyMenuPrep();
    void advModePrep(const std::string& svDev);
}

#endif // USRSEL_H
