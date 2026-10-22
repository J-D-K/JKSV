#include "tasks/saveimport.hpp"

#include "appstates/MainMenuState.hpp"
#include "appstates/SaveImportState.hpp"
#include "error.hpp"
#include "fs/fs.hpp"
#include "fslib.hpp"
#include "logging/logger.hpp"
#include "strings/strings.hpp"
#include "stringutil.hpp"
#include "ui/PopMessageManager.hpp"

// Imports one backup and returns false instead of terminating the batch, so a
// single bad file does not stop the remaining entries. copyTask receives
// byte-level copy progress; it can be null when the caller only tracks files.
static bool import_one(sys::ProgressTask *task,
                       sys::ProgressTask *copyTask,
                       data::User *user,
                       const fslib::Path &target,
                       bool notifyFailure);
static bool test_for_save(data::User *user, const fs::SaveMetaData &saveMeta);
static bool create_save_data_from_meta(sys::ProgressTask *task, data::User *user, const fs::SaveMetaData &saveMeta);

void tasks::saveimport::import_save_backup(sys::threadpool::JobData taskData)
{
    auto castData = std::static_pointer_cast<SaveImportState::DataStruct>(taskData);

    sys::ProgressTask *task = static_cast<sys::ProgressTask *>(castData->task);
    if (error::is_null(task)) { return; }

    import_one(task, task, castData->user, castData->path, true);
    task->complete();
}

// Applies every path to one user. The UI polls BatchData::started and
// BatchData::finished, then reads succeeded and failed for the summary.
void tasks::saveimport::import_save_backups(sys::threadpool::JobData taskData)
{
    auto castData = std::static_pointer_cast<tasks::saveimport::BatchData>(taskData);

    sys::ProgressTask *task = static_cast<sys::ProgressTask *>(castData->task);
    data::User *user        = castData->user;
    if (error::is_null(task))
    {
        castData->finished = true;
        return;
    }

    castData->started = true;

    const int total = static_cast<int>(castData->paths.size());
    task->reset(total);

    const char *statusFormat = strings::get_by_name(strings::names::PEER, 53);

    for (int i = 0; i < total; i++)
    {
        const fslib::Path &path = castData->paths[i];
        const char *filename    = path.get_filename();

        std::string status = stringutil::get_formatted_string(statusFormat ? statusFormat : "Applying %d/%d: %s",
                                                              i + 1,
                                                              total,
                                                              filename);
        task->set_status(status);

        if (import_one(task, nullptr, user, path, false)) { castData->succeeded++; }
        else
        {
            castData->failed++;
            logger::log("saveimport: failed to apply %s", path.string().c_str());
        }

        task->update_current(i + 1);
    }

    castData->finished = true;
    task->complete();
}

static bool import_one(sys::ProgressTask *task,
                       sys::ProgressTask *copyTask,
                       data::User *user,
                       const fslib::Path &target,
                       bool notifyFailure)
{
    if (error::is_null({task, user})) { return false; }

    fs::SaveMetaData metaData{};
    const bool metaRead = fs::read_save_meta(target, metaData);
    if (!metaRead || metaData.revision < 1) { return false; }

    const bool saveExists  = test_for_save(user, metaData);
    const bool saveCreated = !saveExists && create_save_data_from_meta(task, user, metaData);
    if (!saveExists && !saveCreated)
    {
        if (notifyFailure)
        {
            const char *popFailed = strings::get_by_name(strings::names::SAVECREATE_POPS, 1);
            ui::PopMessageManager::push_message(ui::PopMessageManager::DEFAULT_TICKS, popFailed);
        }
        return false;
    }

    const FsSaveDataInfo *saveInfo = user->get_save_info_by_id(metaData.applicationID);
    if (error::is_null(saveInfo)) { return false; }

    {
        fs::ScopedSaveMount saveMount{fs::DEFAULT_SAVE_MOUNT, saveInfo};
        if (!saveMount.is_open()) { return false; }

        const bool isDir = fslib::directory_exists(target);
        if (isDir) { fs::copy_directory_commit(target, fs::DEFAULT_SAVE_ROOT, metaData.journalSize, copyTask); }
        else
        {
            fs::MiniUnzip unzip{target};
            if (!unzip.is_open()) { return false; }

            fs::copy_zip_to_directory(unzip, fs::DEFAULT_SAVE_ROOT, metaData.journalSize, copyTask);
        }
    }

    return true;
}

static bool test_for_save(data::User *user, const fs::SaveMetaData &saveMeta)
{
    const FsSaveDataType saveType = user->get_account_save_type();

    bool mounted{};
    switch (saveType)
    {
        case FsSaveDataType_Account:
        {
            mounted =
                fslib::open_account_save_file_system(fs::DEFAULT_SAVE_MOUNT, saveMeta.applicationID, user->get_account_id());
        }
        break;

        case FsSaveDataType_Bcat:
        {
            mounted = fslib::open_bcat_save_file_system(fs::DEFAULT_SAVE_MOUNT, saveMeta.applicationID);
        }
        break;

        case FsSaveDataType_Device:
        {
            mounted = fslib::open_device_save_file_system(fs::DEFAULT_SAVE_MOUNT, saveMeta.applicationID);
        }
        break;

        case FsSaveDataType_Cache:
        {
            mounted = fslib::open_cache_save_file_system(fs::DEFAULT_SAVE_MOUNT,
                                                         saveMeta.applicationID,
                                                         saveMeta.saveDataIndex,
                                                         static_cast<FsSaveDataSpaceId>(saveMeta.saveDataIndex));
        }
        break;

        default: return false;
    }

    if (mounted) { fslib::close_file_system(fs::DEFAULT_SAVE_MOUNT); }

    return mounted;
}

static bool create_save_data_from_meta(sys::ProgressTask *task, data::User *user, const fs::SaveMetaData &saveMeta)
{
    if (error::is_null({task, user})) { return false; }

    // Gonna borrow this.
    const char *statusFormat = strings::get_by_name(strings::names::USEROPTION_STATUS, 0);
    const std::string hexID  = stringutil::get_formatted_string("%016llX", saveMeta.applicationID);
    std::string status       = stringutil::get_formatted_string(statusFormat, hexID.c_str());
    task->set_status(status);

    const bool saveCreated = fs::create_save_data_for(user, saveMeta);
    if (!saveCreated) { return false; }

    // Need to reload and refresh to reflect changes.
    user->clear_data_entries();
    user->load_user_data();
    MainMenuState::refresh_view_states();

    return true;
}
