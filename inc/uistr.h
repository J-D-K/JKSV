#ifndef UISTR_H
#define UISTR_H

//Strings since translation support
namespace ui
{
    extern const std::string loadGlyphArray[8];

    extern std::string author, userHelp, titleHelp, folderHelp, optHelp, \
    confBlacklist, confOverwrite, confRestore, confDel, confCopy, \
    confEraseNand, confEraseFolder, yt, nt, on, off, confirmHead, \
    copyHead, noSavesFound, errorConnecting, noUpdate;

    //Strings for file mode menu
    extern std::string advMenuStr[6];
    //Strings for extras menu
    extern std::string exMenuStr[11];
    //Strings for options menu
    extern std::string optMenuStr[15];
    //Strings for options explanations
    extern std::string optMenuExp[15];
    //Strings for the holding thing
    extern std::string holdingText[3];
    //Strings for sort type
    extern std::string sortString[3];
}

#endif // UISTR_H
