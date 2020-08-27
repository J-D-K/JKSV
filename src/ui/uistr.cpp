#include <string>

#include "uistr.h"

std::string ui::author = "NULL";
std::string ui::userHelp = "[A] Select   [Y] Dump All   [X] UI Mode   [R] Update   [-] Options   [ZR] Extras";
std::string ui::titleHelp = "[A] Select   [L][R] Change User   [Y] Dump All   [X] Favorite   [-] BlackList   [ZR] Erase   [B] Back";
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
std::string ui::confEraseFolder = "*WARNING*: This *will* delete the current save data for #%s# *from your system*! Are you sure you want to continue?";
std::string ui::noSavesFound = "No saves found for #%s#!";
std::string ui::errorConnecting = "Error Connecting!";
std::string ui::noUpdate = "No updates available!";
std::string ui::advMenuStr[6] = { "Copy to ", "Delete", "Rename", "Make Dir", "Properties", "Close" };
std::string ui::exMenuStr[11] = { "SD to SD Browser", "BIS: PRODINFOF", "BIS: SAFE", "BIS: SYSTEM", "BIS: USER", "Remove Update", "Terminate Process", "Mount System Save", "Rescan Titles", "Mount Process RomFS", "Backup JKSV Folder" };
std::string ui::optMenuStr[14] = { "Include Dev Sv: ", "AutoBackup: ", "Overclock: ", "Hold to Delete: ", "Hold to Restore: ", "Hold to Overwrite: ", "Force Mount: ", "Account Sys. Saves: ", "Write to Sys. Saves: ", "Text UI Mode: ", "Direct FS Cmd: ", "Skip User Select: ", "Export to ZIP: ", "Sort: " };
std::string ui::optMenuExp[14] =
{
    "Includes Device Save data in user accounts.",
    "Automatically creates a save backup before restoring a save.",
    "Applies a small CPU over clock to 1224Mhz at boot. This is the same speed developer units run at.",
    "Whether or not holding [A] is required when deleting folders and files.",
    "Whether or not holding [A] is required when restoring save data.",
    "Whether or not holding [A] is required when overwriting saves on SD.",
    "When enabled, JKSV will only load and show save data that can be opened. When disabled, everything found will be shown.",
    "When enabled, system save data tied to accounts will be shown.",
    "Controls whether system save data and partitions can have files and data written and deleted from them. *This can be extremely dangerous if you don't know what you're doing!*",
    "Changes the UI to be text menu based like the original JKSM for 3DS.",
    "Directly uses the Switch's FS commands to copy files instead of stdio.",
    "Skips the user selection screen and jumps directly to the first user account found.",
    "Exports saves to ZIP files.",
    "Changes the way titles are sorted and listed."
};
std::string ui::holdingText[3] = { "(Hold) ", "(Keep Holding) ", "(Almost there!) " };
std::string ui::sortString[3] = { "Alphabetical", "Time Played", "Last Played" };
