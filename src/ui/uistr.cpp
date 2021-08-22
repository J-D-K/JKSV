#include <string>
#include <map>

#include "file.h"
#include "cfg.h"
#include "type.h"
#include "uistr.h"

std::map<std::pair<std::string, int>, std::string> ui::strings;

static void addUIString(const std::string& _name, int ind, const std::string& _str)
{
    ui::strings[std::make_pair(_name, ind)] = _str;
}

void ui::initStrings()
{
    addUIString("author", 0, "NULL");
    addUIString("helpUser", 0, "[A] Select   [X] User Options");
    addUIString("helpTitle", 0, "[A] Select   [L][R] Jump   [Y] Favorite   [X] Title Options  [B] Back");
    addUIString("helpFolder", 0, "[A] Select  [Y] Restore  [X] Delete  [B] Close");
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

    //File mode properties string
    addUIString("fileModeFileProperties", 0, "Path: %s\nSize: %s");
    addUIString("fileModeFolderProperties", 0, "Path: %s\nSub Folders: %u\nFile Count: %u\nTotal Size: %s");

    //Settings menu
    addUIString("settingsMenu", 0, "Empty Trash Bin");
    addUIString("settingsMenu", 1, "Check for Updates");
    addUIString("settingsMenu", 2, "Set JKSV Save Output Folder");
    addUIString("settingsMenu", 3, "Delete All Save Backups");
    addUIString("settingsMenu", 4, "Include Device Saves With Users: ");
    addUIString("settingsMenu", 5, "Auto Backup On Restore: ");
    addUIString("settingsMenu", 6, "Auto-Name Backups: ");
    addUIString("settingsMenu", 7, "Overclock/CPU Boost: ");
    addUIString("settingsMenu", 8, "Hold To Delete: ");
    addUIString("settingsMenu", 9, "Hold To Restore: ");
    addUIString("settingsMenu", 10, "Hold To Overwrite: ");
    addUIString("settingsMenu", 11, "Force Mount: ");
    addUIString("settingsMenu", 12, "Account System Saves: ");
    addUIString("settingsMenu", 13, "Enable Writing to System Saves: ");
    addUIString("settingsMenu", 14, "Use FS Commands Directly: ");
    addUIString("settingsMenu", 15, "Export Saves to ZIP: ");
    addUIString("settingsMenu", 16, "Force English To Be Used: ");
    addUIString("settingsMenu", 17, "Enable Trash Bin: ");
    addUIString("settingsMenu", 18, "Title Sorting Type: ");
    addUIString("settingsMenu", 19, "Animation Scale: ");

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

    //Thread Status Strings
    addUIString("threadStatusCreatingSaveData", 0, "Creating save data for #%s#...");
    addUIString("threadStatusCopyingFile", 0, "Copying '#%s#'...");
    addUIString("threadStatusDeletingFile", 0, "Deleting...");
    addUIString("threadStatusOpeningFolder", 0, "Opening '#%s#'...");
    addUIString("threadStatusAddingFileToZip", 0, "Adding '#%s#' to ZIP...");
    addUIString("threadStatusDecompressingFile", 0, "Decompressing '#%s#'...");
    addUIString("threadStatusResettingSaveData", 0, "Resetting Save Data for #%s#...");
    addUIString("threadStatusDeletingSaveData", 0, "Deleting Save Data for #%s#...");
    addUIString("threadStatusExtendingSaveData", 0, "Extending Save Data for #%s#...");
    addUIString("threadStatusCreatingSaveData", 0, "Creating Save Data for #%s#...");
    addUIString("threadStatusResettingSaveData", 0, "Resetting save data...");
    addUIString("threadStatusDeletingUpdate", 0, "Deleting pending update...");
    addUIString("threadStatusCheckingForUpdate", 0, "Checking for updates...");
    addUIString("threadStatusDownloadingUpdate", 0, "Downloading update...");
    addUIString("threadStatusGetDirProps", 0, "Getting Folder Properties...");
    addUIString("threadStatusPackingJKSV", 0, "Writing JKSV folder contents to ZIP...");

    //Random leftover pop-ups
    addUIString("popCPUBoostEnabled", 0, "CPU Boost Enabled for ZIP.");
    addUIString("popErrorCommittingFile", 0, "Error committing file to save!");
    addUIString("popZipIsEmpty", 0, "ZIP file is empty!");
    addUIString("popFolderIsEmpty", 0, "Folder is empty!");
    addUIString("popSaveIsEmpty", 0, "Save data is empty!");
    addUIString("popProcessShutdown", 0, "#%s# successfully shutdown.");
    addUIString("popAddedToPathFilter", 0, "'#%s#' added to path filters.");

    //Keyboard hints
    addUIString("swkbdEnterName", 0, "Enter a new name");
    addUIString("swkbdSaveIndex", 0, "Enter Cache Index");
    addUIString("swkbdSetWorkDir", 0, "Enter a new Output Path");
    addUIString("swkbdProcessID", 0, "Enter Process ID");
    addUIString("swkbdSysSavID", 0, "Enter System Save ID");
    addUIString("swkbdRename", 0, "Enter a new name for item");
    addUIString("swkbdMkDir", 0, "Enter a folder name");
}

void ui::loadTrans()
{
    bool transFile = fs::fileExists(fs::getWorkDir() + "trans.txt");
    if(!transFile && (data::sysLang == SetLanguage_ENUS || cfg::config["langOverride"]))
        ui::initStrings();
    else
    {
        std::string file;
        if(transFile)
            file = fs::getWorkDir() + "trans.txt";
        else
        {
            file = "romfs:/lang/";
            switch(data::sysLang)
            {
                //I removed these for now. Old translation files are incompatible and will cause crashes.
                default:
                    ui::initStrings();
                    return;
                    break;
            }
        }

        fs::dataFile lang(file);
        while(lang.readNextLine(true))
        {
            std::string name = lang.getName();
            int ind = lang.getNextValueInt();
            std::string str = lang.getNextValueStr();
            addUIString(name, ind, str);
        }
    }
}

void ui::saveTranslationFile(void *a)
{
    threadInfo *t = (threadInfo *)a;
    t->status->setStatus("Saving the file master...");

    std::string out = fs::getWorkDir() + "en-US.txt";
    FILE *enUS = fopen(out.c_str(), "w");
    for(auto& s : ui::strings)
        fprintf(enUS, "%s = %i, \"%s\"\n", s.first.first.c_str(), s.first.second, s.second.c_str());
    fclose(enUS);
    t->finished = true;
}
