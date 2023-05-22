#include <string>
#include <map>

#include "file.h"
#include "cfg.h"
#include "type.h"
#include "util.h"
#include "uistr.h"

std::map<std::pair<std::string, int>, std::string> ui::strings;

static void addUIString(const std::string& _name, int ind, const std::string& _str)
{
    ui::strings[std::make_pair(_name, ind)] = _str;
}

static void loadTranslationFile(const std::string& path)
{
    if(fs::fileExists(path))
    {
        fs::dataFile lang(path);
        while(lang.readNextLine(true))
        {
            std::string name = lang.getName();
            int ind = lang.getNextValueInt();
            std::string str = lang.getNextValueStr();
            addUIString(name, ind, str);
        }
    }
}

static std::string getFilename(int lang)
{
    std::string filename;
    switch(lang)
    {
        case SetLanguage_JA:
            filename = "ja.txt";
            break;

        case SetLanguage_ENUS:
            filename = "en-US.txt";
            break;

        case SetLanguage_FR:
            filename = "fr.txt";
            break;

        case SetLanguage_DE:
            filename = "de.txt";
            break;

        case SetLanguage_IT:
            filename = "it.txt";
            break;

        case SetLanguage_ES:
            filename = "es.txt";
            break;

        case SetLanguage_ZHCN:
        case SetLanguage_ZHHANS:
            filename = "zh-CN.txt";
            break;

        case SetLanguage_KO:
            filename = "ko.txt";
            break;

        case SetLanguage_NL:
            filename = "nl.txt";
            break;

        case SetLanguage_PT:
            filename = "pt.txt";
            break;

        case SetLanguage_RU:
            filename = "ru.txt";
            break;

        case SetLanguage_ZHTW:
        case SetLanguage_ZHHANT:
            filename += "zh-TW.txt";
            break;

        case SetLanguage_ENGB:
            filename = "en-GB.txt";
            break;

        case SetLanguage_FRCA:
            filename = "fr-CA.txt";
            break;

        case SetLanguage_ES419:
            filename = "es-419.txt";
            break;

        case SetLanguage_PTBR:
            filename = "pt-BR.txt";
            break;
    }
    return filename;
}

void ui::initStrings()
{
    addUIString("author", 0, "NULL");
    addUIString("helpUser", 0, "[A] Select   [Y] Dump All Saves   [X] User Options");
    addUIString("helpTitle", 0, "[A] Select   [L][R] Jump   [Y] Favorite   [X] Title Options  [B] Back");
    addUIString("helpFolder", 0, "[A] Select  [Y] Restore  [X] Delete   [ZR] Upload  [B] Close");
    addUIString("helpSettings", 0, "[A] Toggle   [X] Defaults   [B] Back");

    //Y/N On/Off
    addUIString("dialogYes", 0, "Yes [A]");
    addUIString("dialogNo", 0, "No [B]");
    addUIString("dialogOK", 0, "OK [A]");
    addUIString("settingsOn", 0, ">On>");
    addUIString("settingsOff", 0, "Off");
    addUIString("holdingText", 0, "(Hold) ");
    addUIString("holdingText", 1, "(Keep Holding) ");
    addUIString("holdingText", 2, "(Almost There!) ");

    //Confirmation Strings
    addUIString("confirmBlacklist", 0, "Are you sure you want to add #%s# to your blacklist?");
    addUIString("confirmOverwrite", 0, "Are you sure you want to overwrite #%s#?");
    addUIString("confirmRestore", 0, "Are you sure you want to restore #%s#?");
    addUIString("confirmDelete", 0, "Are you sure you want to delete #%s#? *This is permanent*!");
    addUIString("confirmCopy", 0, "Are you sure you want to copy #%s# to #%s#?");
    addUIString("confirmDeleteSaveData", 0, "*WARNING*: This *will* erase the save data for #%s# *from your system*. Are you sure you want to do this?");
    addUIString("confirmResetSaveData", 0, "*WARNING*: This *will* reset the save data for this game as if it was never ran before. Are you sure you want to do this?");
    addUIString("confirmCreateAllSaveData", 0, "Are you sure you would like to create all save data on this system for #%s#? This can take a while depending on how many titles are found.");
    addUIString("confirmDeleteBackupsTitle", 0, "Are you sure you would like to delete all save backups for #%s#?");
    addUIString("confirmDeleteBackupsAll", 0, "Are you sure you would like to delete *all* of your save backups for all of your games?");
    addUIString("confirmDriveOverwrite", 0, "Downloading this backup from drive will overwrite the one on your SD card. Continue?");

    //Save Data related strings
    addUIString("saveDataNoneFound", 0, "No saves found for #%s#!");
    addUIString("saveDataCreatedForUser", 0, "Save data created for %s!");
    addUIString("saveDataCreationFailed", 0, "Save data creation failed!");
    addUIString("saveDataResetSuccess", 0, "Save for #%s# reset!");
    addUIString("saveDataDeleteSuccess", 0, "Save data for #%s# deleted!");
    addUIString("saveDataExtendSuccess", 0, "Save data for #%s# extended!");
    addUIString("saveDataExtendFailed", 0, "Failed to extend save data.");
    addUIString("saveDataDeleteAllUser", 0, "*ARE YOU SURE YOU WANT TO DELETE ALL SAVE DATA FOR %s?*");
    addUIString("saveDataBackupDeleted", 0, "#%s# has been deleted.");
    addUIString("saveDataBackupMovedToTrash", 0, "#%s# has been moved to trash.");
    addUIString("saveTypeMainMenu", 0, "Device");
    addUIString("saveTypeMainMenu", 1, "BCAT");
    addUIString("saveTypeMainMenu", 2, "Cache");
    addUIString("saveTypeMainMenu", 3, "System");
    addUIString("saveTypeMainMenu", 4, "System BCAT");
    addUIString("saveTypeMainMenu", 5, "Temporary");
    //This is redundant. Need to merge and use one or the other...
    addUIString("saveDataTypeText", 0, "System");
    addUIString("saveDataTypeText", 1, "Account");
    addUIString("saveDataTypeText", 2, "BCAT");
    addUIString("saveDataTypeText", 3, "Device");
    addUIString("saveDataTypeText", 4, "Temporary");
    addUIString("saveDataTypeText", 5, "Cache");
    addUIString("saveDataTypeText", 6, "System BCAT");

    //Internet Related
    addUIString("onlineErrorConnecting", 0, "Error Connecting!");
    addUIString("onlineNoUpdates", 0, "No Updates Available.");

    //File mode menu strings
    addUIString("fileModeMenu", 0, "Copy To ");
    addUIString("fileModeMenu", 1, "Delete");
    addUIString("fileModeMenu", 2, "Rename");
    addUIString("fileModeMenu", 3, "Make Dir");
    addUIString("fileModeMenu", 4, "Properties");
    addUIString("fileModeMenu", 5, "Close");
    addUIString("fileModeMenu", 6, "Add to Path Filters");
    addUIString("fileModeMenuMkDir", 0, "New");

    //New folder pop menu strings
    addUIString("folderMenuNew", 0, "New Backup");

    //File mode properties string
    addUIString("fileModeFileProperties", 0, "Path: %s\nSize: %s");
    addUIString("fileModeFolderProperties", 0, "Path: %s\nSub Folders: %u\nFile Count: %u\nTotal Size: %s");

    //Settings menu
    addUIString("settingsMenu", 0, "Empty Trash Bin");
    addUIString("settingsMenu", 1, "Check for Updates");
    addUIString("settingsMenu", 2, "Set JKSV Save Output Folder");
    addUIString("settingsMenu", 3, "Edit Blacklisted Titles");
    addUIString("settingsMenu", 4, "Delete All Save Backups");
    addUIString("settingsMenu", 5, "Include Device Saves With Users: ");
    addUIString("settingsMenu", 6, "Auto Backup On Restore: ");
    addUIString("settingsMenu", 7, "Auto-Name Backups: ");
    addUIString("settingsMenu", 8, "Overclock/CPU Boost: ");
    addUIString("settingsMenu", 9, "Hold To Delete: ");
    addUIString("settingsMenu", 10, "Hold To Restore: ");
    addUIString("settingsMenu", 11, "Hold To Overwrite: ");
    addUIString("settingsMenu", 12, "Force Mount: ");
    addUIString("settingsMenu", 13, "Account System Saves: ");
    addUIString("settingsMenu", 14, "Enable Writing to System Saves: ");
    addUIString("settingsMenu", 15, "Use FS Commands Directly: ");
    addUIString("settingsMenu", 16, "Export Saves to ZIP: ");
    addUIString("settingsMenu", 17, "Force English To Be Used: ");
    addUIString("settingsMenu", 18, "Enable Trash Bin: ");
    addUIString("settingsMenu", 19, "Title Sorting Type: ");
    addUIString("settingsMenu", 20, "Animation Scale: ");
    addUIString("settingsMenu", 21, "Auto-upload to Drive: ");

    //Main menu
    addUIString("mainMenuSettings", 0, "Settings");
    addUIString("mainMenuExtras", 0, "Extras");

    // Translator in main page
    addUIString("translationMainPage", 0, "Translation: ");

    //Loading page
    addUIString("loadingStartPage", 0, "Loading...");

    //Sort Strings for ^
    addUIString("sortType", 0, "Alphabetical");
    addUIString("sortType", 1, "Time Played");
    addUIString("sortType", 2, "Last Played");

    //Extras
    addUIString("extrasMenu", 0, "SD to SD Browser");
    addUIString("extrasMenu", 1, "BIS: ProdInfoF");
    addUIString("extrasMenu", 2, "BIS: Safe");
    addUIString("extrasMenu", 3, "BIS: System");
    addUIString("extrasMenu", 4, "BIS: User");
    addUIString("extrasMenu", 5, "Remove Pending Update");
    addUIString("extrasMenu", 6, "Terminate Process");
    addUIString("extrasMenu", 7, "Mount System Save");
    addUIString("extrasMenu", 8, "Rescan Titles");
    addUIString("extrasMenu", 9, "Mount Process RomFS");
    addUIString("extrasMenu", 10, "Backup JKSV Folder");
    addUIString("extrasMenu", 11, "*[DEV]* Output en-US");

    //User Options
    addUIString("userOptions", 0, "Dump All For ");
    addUIString("userOptions", 1, "Create Save Data");
    addUIString("userOptions", 2, "Create All Save Data");
    addUIString("userOptions", 3, "Delete All User Saves");

    //Title Options
    addUIString("titleOptions", 0, "Information");
    addUIString("titleOptions", 1, "Blacklist");
    addUIString("titleOptions", 2, "Change Output Folder");
    addUIString("titleOptions", 3, "Open in File Mode");
    addUIString("titleOptions", 4, "Delete All Save Backups");
    addUIString("titleOptions", 5, "Reset Save Data");
    addUIString("titleOptions", 6, "Delete Save Data");
    addUIString("titleOptions", 7, "Extend Save Data");
    addUIString("titleOptions", 8, "Export SVI");

    //Thread Status Strings
    addUIString("threadStatusCreatingSaveData", 0, "Creating save data for #%s#...");
    addUIString("threadStatusCopyingFile", 0, "Copying '#%s#'...");
    addUIString("threadStatusDeletingFile", 0, "Deleting...");
    addUIString("threadStatusOpeningFolder", 0, "Opening '#%s#'...");
    addUIString("threadStatusAddingFileToZip", 0, "Adding '#%s#' to ZIP...");
    addUIString("threadStatusDecompressingFile", 0, "Decompressing '#%s#'...");
    addUIString("threadStatusDeletingSaveData", 0, "Deleting Save Data for #%s#...");
    addUIString("threadStatusExtendingSaveData", 0, "Extending Save Data for #%s#...");
    addUIString("threadStatusCreatingSaveData", 0, "Creating Save Data for #%s#...");
    addUIString("threadStatusResettingSaveData", 0, "Resetting save data...");
    addUIString("threadStatusDeletingUpdate", 0, "Deleting pending update...");
    addUIString("threadStatusCheckingForUpdate", 0, "Checking for updates...");
    addUIString("threadStatusDownloadingUpdate", 0, "Downloading update...");
    addUIString("threadStatusGetDirProps", 0, "Getting Folder Properties...");
    addUIString("threadStatusPackingJKSV", 0, "Writing JKSV folder contents to ZIP...");
    addUIString("threadStatusSavingTranslations", 0, "Saving the file master...");
    addUIString("threadStatusCalculatingSaveSize", 0, "Calculating save data size...");
    addUIString("threadStatusUploadingFile", 0, "Uploading #%s#...");
    addUIString("threadStatusDownloadingFile", 0, "Downloading #%s#...");
    addUIString("threadStatusCompressingSaveForUpload", 0, "Compressing #%s# for upload...");

    //Random leftover pop-ups
    addUIString("popCPUBoostEnabled", 0, "CPU Boost Enabled for ZIP.");
    addUIString("popErrorCommittingFile", 0, "Error committing file to save!");
    addUIString("popZipIsEmpty", 0, "ZIP file is empty!");
    addUIString("popFolderIsEmpty", 0, "Folder is empty!");
    addUIString("popSaveIsEmpty", 0, "Save data is empty!");
    addUIString("popProcessShutdown", 0, "#%s# successfully shutdown.");
    addUIString("popAddedToPathFilter", 0, "'#%s#' added to path filters.");
    addUIString("popChangeOutputFolder", 0, "#%s# changed to #%s#");
    addUIString("popChangeOutputError", 0, "#%s# contains illegal or non-ASCII characters.");
    addUIString("popTrashEmptied", 0, "Trash emptied");
    addUIString("popSVIExported", 0, "SVI Exported.");
    addUIString("popDriveStarted", 0, "Google Drive started successfully.");
    addUIString("popDriveFailed", 0, "Failed to start Google Drive.");
    addUIString("popDriveNotActive", 0, "Google Drive is not available");

    //Keyboard hints
    addUIString("swkbdEnterName", 0, "Enter a new name");
    addUIString("swkbdSaveIndex", 0, "Enter Cache Index");
    addUIString("swkbdSetWorkDir", 0, "Enter a new Output Path");
    addUIString("swkbdProcessID", 0, "Enter Process ID");
    addUIString("swkbdSysSavID", 0, "Enter System Save ID");
    addUIString("swkbdRename", 0, "Enter a new name for item");
    addUIString("swkbdMkDir", 0, "Enter a folder name");
    addUIString("swkbdNewSafeTitle", 0, "Input New Output Folder");
    addUIString("swkbdExpandSize", 0, "Enter New Size in MB");

    //Status informations
    addUIString("infoStatus", 0, "TID: %016lX");
    addUIString("infoStatus", 1, "SID: %016lX");
    addUIString("infoStatus", 2, "Play Time: %02d:%02d");
    addUIString("infoStatus", 3, "Total Launches: %u");
    addUIString("infoStatus", 4, "Publisher: %s");
    addUIString("infoStatus", 5, "Save Type: %s");
    addUIString("infoStatus", 6, "Cache Index: %u");
    addUIString("infoStatus", 7, "User: %s");

    addUIString("debugStatus", 0, "User Count: ");
    addUIString("debugStatus", 1, "Current User: ");
    addUIString("debugStatus", 2, "Current Title: ");
    addUIString("debugStatus", 3, "Safe Title: ");
    addUIString("debugStatus", 4, "Sort Type: ");

    addUIString("appletModeWarning", 0, "*WARNING*: You are running JKSV in applet mode. Certain functions may not work.");
}

void ui::loadTrans()
{
    std::string transTestFile = fs::getWorkDir() + "trans.txt";
    std::string translationFile = "romfs:/lang/" + getFilename(data::sysLang);
    bool transFile = fs::fileExists(transTestFile);
    if(!transFile && (data::sysLang == SetLanguage_ENUS || data::sysLang == SetLanguage_ENGB || cfg::config["langOverride"]))
        ui::initStrings();
    else if(transFile)
        loadTranslationFile(transTestFile);
    else
        loadTranslationFile(translationFile);

    util::replaceButtonsInString(ui::strings[std::make_pair("helpUser", 0)]);
    util::replaceButtonsInString(ui::strings[std::make_pair("helpTitle", 0)]);
    util::replaceButtonsInString(ui::strings[std::make_pair("helpFolder", 0)]);
    util::replaceButtonsInString(ui::strings[std::make_pair("helpSettings", 0)]);
    util::replaceButtonsInString(ui::strings[std::make_pair("dialogYes", 0)]);
    util::replaceButtonsInString(ui::strings[std::make_pair("dialogNo", 0)]);
    util::replaceButtonsInString(ui::strings[std::make_pair("dialogOK", 0)]);
    util::replaceButtonsInString(ui::strings[std::make_pair("appletModeWarning", 0)]);
}

void ui::saveTranslationFiles(void *a)
{
    threadInfo *t = (threadInfo *)a;
    t->status->setStatus(ui::getUICString("infoStatus", 9));

    std::string outputFolder = fs::getWorkDir() + "lang", outputPath, romfsPath;
    fs::mkDir(outputFolder);

    //Save en-us first
    ui::initStrings();
    outputPath = fs::getWorkDir() + "lang/" + getFilename(SetLanguage_ENUS);
    FILE *out = fopen(outputPath.c_str(), "w");
    for(auto& s : ui::strings)
    {
        std::string stringOut = s.second;
            util::replaceStr(stringOut, "\n", "\\n");
            fprintf(out, "%s = %i, \"%s\"\n", s.first.first.c_str(), s.first.second, stringOut.c_str());
    }
    fclose(out);

    romfsInit();
    for(int i = 0; i < SetLanguage_Total; i++)
    {
        if(i == SetLanguage_ENUS)
            continue;

        outputPath = fs::getWorkDir() + "lang/" + getFilename(i);
        romfsPath = "romfs:/lang/" + getFilename(i);

        //Init original US English strings, then load translation over it. Result will be mixed file.
        ui::initStrings();
        loadTranslationFile(romfsPath);

        out = fopen(outputPath.c_str(), "w");
        for(auto& s : ui::strings)
        {
            std::string stringOut = s.second;
            util::replaceStr(stringOut, "\n", "\\n");
            fprintf(out, "%s = %i, \"%s\"\n", s.first.first.c_str(), s.first.second, stringOut.c_str());
        }
        fclose(out);
    }
    loadTrans();
    romfsExit();
    t->finished = true;
}
