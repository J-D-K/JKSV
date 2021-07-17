#include <string>
#include <unordered_map>

#include "file.h"
#include "uistr.h"

//Map to associate external string names to unsigned ints for switch case.
static std::unordered_map<std::string, unsigned> uistrdef =
{
    {"author", 0}, {"userHelp", 1}, {"titleHelp", 2}, {"folderHelp", 3}, {"optHelp", 4},
    {"yt", 5}, {"nt", 6}, {"on", 7}, {"off", 8}, {"confirmBlacklist", 9}, {"confirmOverwrite", 10},
    {"confirmRestore", 11}, {"confirmDelete", 12}, {"confirmCopy", 13}, {"confirmEraseNand", 14},
    {"confirmReset", 15}, {"confirmHead", 16}, {"copyHead", 17}, {"noSavesFound", 18},
    {"advMenu", 19}, {"extMenu", 20}, {"optMenu", 21}, {"optMenuExp", 22}, {"holdingText", 23},
    {"errorConnecting", 24}, {"noUpdate", 25}, {"sortType", 26}, {"saveCreated", 27}, {"saveCreateFailed", 28},
	{"saveDataResetSuccess", 29}, {"saveDataDeleteSuccess", 30}
};

std::string ui::author = "NULL";
std::string ui::userHelp = "[A] Select   [X] User Options";
std::string ui::titleHelp = "[A] Select   [L][R] Jump   [Y] Favorite   [X] Title Options  [B] Back";
std::string ui::folderHelp = "[-] File Mode  [L]/[R]+[A] Auto  [A] Backup  [Y] Restore  [X] Delete Selected  [ZR] Erase  [B] Back";
std::string ui::optHelp = "[A] Toggle   [X] Defaults   [B] Back";
std::string ui::yt = "Yes [A]", ui::nt = "No  [B]";
std::string ui::on = ">On>", ui::off = "Off";
std::string ui::confBlacklist = "Are you sure you want to add #%s# to your blacklist?";
std::string ui::confOverwrite = "Are you sure you want to overwrite #%s#?";
std::string ui::confRestore = "Are you sure you want to restore #%s#?";
std::string ui::confDel = "Are you sure you want to delete #%s#? *This is permanent*!";
std::string ui::confCopy = "Are you sure you want to copy #%s# to #%s#?";
std::string ui::confirmHead = "Confirm";
std::string ui::copyHead = "Copying File...";
std::string ui::confEraseNand = "*WARNING*: This *will* erase the save data for #%s# *from your system*. This is the same as deleting it from #Data Management#! Are you sure you want to continue?";
std::string ui::noSavesFound = "No saves found for #%s#!";
std::string ui::saveCreated = "Save data created for %s!";
std::string ui::saveCreateFailed = "Save data creation failed!";
std::string ui::saveDataReset = "Are you sure want to reset your current save data for %s?";
std::string ui::saveDataResetSuccess = "Save for %s reset!";
std::string ui::saveDataDeleteSuccess = "Save data for %s deleted!";
std::string ui::errorConnecting = "Error Connecting!";
std::string ui::noUpdate = "No updates available!";
std::string ui::advMenuStr[6] = { "Copy to ", "Delete", "Rename", "Make Dir", "Properties", "Close" };
std::string ui::exMenuStr[11] = { "SD to SD Browser", "BIS: PRODINFOF", "BIS: SAFE", "BIS: SYSTEM", "BIS: USER", "Remove Update", "Terminate Process", "Mount System Save", "Rescan Titles", "Mount Process RomFS", "Backup JKSV Folder" };
std::string ui::optMenuStr[14] = { "Include Device Saves: ", "AutoBackup: ", "Overclock: ", "Hold to Delete: ", "Hold to Restore: ", "Hold to Overwrite: ", "Force Mount: ", "Account Sys. Saves: ", "Write to Sys. Saves: ", "Direct FS Cmd: ", "Export to ZIP: ", "Language Override: ", "Sort: ", "Animation Scale: "};
std::string ui::holdingText[3] = { "(Hold) ", "(Keep Holding) ", "(Almost there!) " };
std::string ui::sortString[3] = { "Alphabetical", "Time Played", "Last Played" };
std::string ui::usrOptString[2] = { "Create Save Data", "Delete All User Saves" };
std::string ui::titleOptString[5] = {"Information", "Blacklist", "Reset Save Data", "Delete Save Filesystem", "Extend Save Filesystem"};

void ui::loadTrans()
{
    bool transFile = fs::fileExists(fs::getWorkDir() + "trans.txt");
    if(!transFile && data::sysLang == SetLanguage_ENUS)
        return;//Don't bother loading from file. It serves as a translation guide

    std::string file;
    if(transFile)
        file = fs::getWorkDir() + "trans.txt";
    else
    {
        file = "romfs:/lang/";
        switch(data::sysLang)
        {
            case SetLanguage_ZHCN:
            case SetLanguage_ZHHANS:
                file += "zh-CN.txt";
                break;

            case SetLanguage_ZHTW:
            case SetLanguage_ZHHANT:
                file += "zh-TW.txt";
                break;

            default:
                return;
                break;
        }
    }

    fs::dataFile lang(file);
    while(lang.readNextLine(true))
    {
        switch(uistrdef[lang.getName()])
        {
            case 0:
                ui::author = lang.getNextValueStr();
                break;

            case 1:
                ui::userHelp = lang.getNextValueStr();
                break;

            case 2:
                ui::titleHelp = lang.getNextValueStr();
                break;

            case 3:
                ui::folderHelp = lang.getNextValueStr();
                break;

            case 4:
                ui::optHelp = lang.getNextValueStr();
                break;

            case 5:
                ui::yt = lang.getNextValueStr();
                break;

            case 6:
                ui::nt = lang.getNextValueStr();
                break;

            case 7:
                ui::on = lang.getNextValueStr();
                break;

            case 8:
                ui::off = lang.getNextValueStr();
                break;

            case 9:
                ui::confBlacklist = lang.getNextValueStr();
                break;

            case 10:
                ui::confOverwrite = lang.getNextValueStr();
                break;

            case 11:
                ui::confRestore = lang.getNextValueStr();
                break;

            case 12:
                ui::confDel = lang.getNextValueStr();
                break;

            case 13:
                ui::confCopy = lang.getNextValueStr();
                break;

            case 14:
                ui::confEraseNand = lang.getNextValueStr();
                break;

            case 15:
                ui::saveDataReset = lang.getNextValueStr();
                break;

            case 16:
                ui::confirmHead = lang.getNextValueStr();
                break;

            case 17:
                ui::copyHead = lang.getNextValueStr();
                break;

            case 18:
                ui::noSavesFound = lang.getNextValueStr();
                break;

            case 19:
                {
                    int ind = lang.getNextValueInt();
                    ui::advMenuStr[ind] = lang.getNextValueStr();
                }
                break;

            case 20:
                {
                    int ind = lang.getNextValueInt();
                    ui::exMenuStr[ind] = lang.getNextValueStr();
                }
                break;

            case 21:
                {
                    int ind = lang.getNextValueInt();
                    ui::optMenuStr[ind] = lang.getNextValueStr();
                }
                break;

            case 22:
                break;

            case 23:
                {
                    int ind = lang.getNextValueInt();
                    ui::holdingText[ind] = lang.getNextValueStr();
                }
                break;

            case 24:
                ui::errorConnecting = lang.getNextValueStr();
                break;

            case 25:
                ui::noUpdate = lang.getNextValueStr();
                break;

            case 26:
                {
                    int ind = lang.getNextValueInt();
                    ui::sortString[ind] = lang.getNextValueStr();
                }
                break;

            default:
                ui::showMessage("*Translation File Error:*", "On Line: %s\n*%s* is not a known or valid string name.", lang.getLine(), lang.getName());
                break;
        }
    }
}

