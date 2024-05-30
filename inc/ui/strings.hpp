#pragma once
#include <string>

/* HUGE TO DO: REVISE THIS ENTIRE NAMING THING*/

// To prevent typos
#define LANG_AUTHOR "author"
#define LANG_USER_GUIDE "helpUser"
#define LANG_TITLE_GUIDE "helpTitle"
#define LANG_FOLDER_GUIDE "helpFolder"
#define LANG_SETTINGS_GUIDE "helpSettings"

// Dialog/Confirm stuff
#define LANG_DIALOG_YES "dialogYes"
#define LANG_DIALOG_NO "dialogNo"
#define LANG_DIALOG_OK "dialogOK"
#define LANG_SETTINGS_ON "settingsOn"
#define LANG_SETTINGS_OFF "settingsOff"
#define LANG_HOLDING_TEXT "holdingText"

// Confirmation
#define LANG_CONFIRM_BLACKLIST "confirmBlacklist"
#define LANG_CONFIRM_OVERWRITE "confirmOverwrite"
#define LANG_CONFIRM_RESTORE "confirmRestore"
#define LANG_CONFIRM_DELETE "confirmDelete"
#define LANG_CONFIRM_COPY "confirmCopy"
#define LANG_CONFIRM_DELETE_SAVEDATA "confirmDeleteSaveData"
#define LANG_CONFIRM_RESET_SAVEDATA "confirmResetSaveData"
#define LANG_CONFIRM_CREATE_ALL_SAVEDATA "confirmCreateAllSaveData"
#define LANG_CONFIRM_DELETE_BACKUPS_TITLE "confirmDeleteBackupsTitle"
#define LANG_CONFIRM_DELETE_ALL_BACKUPS "confirmDeleteBackupsAll"
#define LANG_CONFIRM_DRIVE_OVERWRITE "confirmDriveOverwrite"

// Save data management strings
#define LANG_SAVEDATA_NONE_FOUND "saveDataNoneFound"
#define LANG_SAVEDATA_CREATED_FOR_USER "saveDataCreatedForUser"
#define LANG_SAVEDATA_CREATION_FAILED "saveDataCreationFailed"
#define LANG_SAVEDATA_RESET_SUCCESS "saveDataResetSuccess"
#define LANG_SAVEDATA_DELETE_SUCCESS "saveDataDeleteSuccess"
#define LANG_SAVEDATA_EXTEND_SUCCESS "saveDataExtendSuccess"
#define LANG_SAVEDATA_DELETE_ALL_USER_SAVES "saveDataDeleteAllUser"
#define LANG_SAVEDATA_BACKUP_DELETED "saveDataBackupDeleted"
#define LANG_SAVEDATA_BACKUP_TRASHED "saveDataBackupMovedToTrash"
#define LANG_SAVEDATA_ERROR_MOUNTING "saveDataErrorMounting"

// Save data types
#define LANG_SAVEDATA_TYPE_TEXT "saveDataTypeText"

// Wifi/internet stuff
#define LANG_ONLINE_ERROR_CONNECTING "onlineErrorConnecting"
#define LANG_ONLINE_NO_UPDATES "onlineNoUpdates"

// Menu strings
#define LANG_FILE_MODE_MENU "fileModeMenu"
#define LANG_FOLDER_MENU_NEW "folderMenuNew"
#define LANG_SETTINGS_MENU "settingsMenu"
#define LANG_MAIN_MENU_SETTINGS "mainMenuSettings"
#define LANG_MAIN_MENU_EXTRAS "mainMenuExtras"
#define LANG_EXTRAS_MENU "extrasMenu"
#define LANG_USER_OPTIONS_MENU "userOptions"
#define LANG_TITLE_OPTIONS_MENU "titleOptions"
#define LANG_SAVE_TYPE_MAIN_MENU "saveTypeMainMenu"

#define LANG_TITLE_OPTIONS_MENU "titleOptions"

// Thread status strings
#define LANG_THREAD_CREATING_SAVE "threadStatusCreatingSaveData"
#define LANG_THREAD_COPYING_FILE "threadStatusCopyingFile"
#define LANG_THREAD_DELETE_FILE "threadStatusDeletingFile"
#define LANG_THREAD_OPENING_FOLDER "threadStatusOpeningFolder"
#define LANG_THREAD_ADDING_TO_ZIP "threadStatusAddingFolderToZip"
#define LANG_THREAD_DECOMPRESSING_FILE "threadStatusDecompressingFile"
#define LANG_THREAD_DELETING_SAVE_DATA "threadStatusDeletingSaveData"
#define LANG_THREAD_EXTENDING_SAVE_DATA "threadStatusExtendingSaveData"
#define LANG_THREAD_CREATING_SAVE_DATA "threadStatusCreatingSaveData"
#define LANG_THREAD_RESET_SAVE_DATA "threadStatusResettingSaveData"
#define LANG_THREAD_DELETING_UPDATE "threadStatusDeletingUpdate"
#define LANG_THREAD_CHECKING_FOR_UPDATE "threadStatusCheckingForUpdate"
#define LANG_THREAD_DOWNLOADING_UPDATE "threadStatusDownloadingUpdate"
#define LANG_THREAD_GET_DIRECTORY_PROPS "threadStatusGetDirProps"
#define LANG_THREAD_PACKING_JKSV "threadStatusPackingJKSV"
#define LANG_THREAD_SAVING_TRANS "threadStatusSavingTranslations"
#define LANG_THREAD_CALCULATING_SAVE_SIZE "threadStatusCalculatingSaveSize"
#define LANG_THREAD_UPLOADING_FILE "threadStatusUploadingFile"
#define LANG_THREAD_DOWNLOADLING_FILE "threadStatusDownloadingFile"

#define LANG_THREAD_COMPRESSING_UPLOAD "threadStatusCompressingSaveForUpload"

// Popup message strings
#define LANG_POP_CPU_BOOST "popCPUBoostEnabled"
#define LANG_POP_ERROR_COMMITING "popErrorCommitingFile"
#define LANG_POP_ZIP_EMPTY "popZipIsEmpty"
#define LANG_POP_SAVE_EMPTY "popSaveIsEmpty"
#define LANG_POP_FOLDER_EMPTY "popFolderIsEmpty"
#define LANG_POP_PROCESS_SHUTDOWN "popProcessShutdown"
#define LANG_POP_ADDED_PATH_FILTER "popAddedPathToFilter"
#define LANG_POP_CHANGED_OUTPUT_DIR "popChangedOutputFolder"
#define LANG_POP_CHANGED_OUTPUT_ERROR "popChangedOutputError"
#define LANG_POP_TRASH_EMPTIED "popTrashEmptied"
#define LANG_POP_SVI_EXPORTED "popSVIExported"
#define LANG_POP_DRIVE_STARTED "popDriveStarted"
#define LANG_POP_DRIVE_FAILED "popDriveFailed"
#define LANG_POP_DRIVE_INACTIVE "popDriveNotActive"

namespace ui
{
    namespace strings
    {
        void init(void);

        std::string getString(const std::string &name, const int &index);
        const char *getCString(const std::string &name, const int &index);
    }
}