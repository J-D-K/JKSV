#pragma once

//Strings since translation support
namespace ui
{
    void loadTrans();

    extern std::string author, userHelp, titleHelp, folderHelp, optHelp, \
    confBlacklist, confOverwrite, confRestore, confDel, confCopy, \
    confEraseNand, confEraseFolder, yt, nt, on, off, confirmHead, \
    copyHead, noSavesFound, errorConnecting, noUpdate, saveCreated, saveCreateFailed, \
    saveDataReset, saveDataResetSuccess, saveDataDeleteSuccess;

    //Strings for file mode menu
    extern std::string advMenuStr[6];
    //Strings for extras menu
    extern std::string exMenuStr[11];
    //Strings for options menu
    extern std::string optMenuStr[15];
    //Strings for the holding thing
    extern std::string holdingText[3];
    //Strings for sort type
    extern std::string sortString[3];
    //Strings for user options
    extern std::string usrOptString[1];
    //Strings for title options
    extern std::string titleOptString[4];
}
