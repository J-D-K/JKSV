#include "data/data.hpp"

#include "appstates/DataLoadingState.hpp"
#include "appstates/FadeState.hpp"
#include "data/DataContext.hpp"
#include "error.hpp"
#include "logging/logger.hpp"
#include "strings/strings.hpp"

#include <switch.h>

namespace
{
    // clang-format off
    // This seems stupid, but it's the only way, really.
    struct StateDataStruct : sys::Task::DataStruct
    {
        bool clearCache{};
    };
    // clang-format on

    data::DataContext s_context{};
} // namespace

/// @brief The main routine for the task to load data.
static void data_initialize_task(sys::threadpool::JobData taskData);

void data::launch_initialization(bool clearCache, sdl2::Renderer &renderer, std::function<void()> onDestruction)
{
    auto taskData        = std::make_shared<StateDataStruct>();
    taskData->clearCache = clearCache;

    auto loadingState = DataLoadingState::create(s_context, renderer, onDestruction, data_initialize_task, taskData);
    StateManager::push_state(loadingState);
}

void data::get_users(data::UserList &userList) { s_context.get_users(userList); }

data::TitleInfo *data::get_title_info_by_id(uint64_t applicationID) noexcept
{ return s_context.get_title_by_id(applicationID); }

void data::load_title_to_map(uint64_t applicationID) { s_context.load_title(applicationID); }

bool data::title_exists_in_map(uint64_t applicationID) noexcept { return s_context.title_is_loaded(applicationID); }

void data::get_title_info_list(data::TitleInfoList &listOut) { s_context.get_title_info_list(listOut); }

void data::get_title_info_by_type(FsSaveDataType saveType, data::TitleInfoList &listOut)
{ s_context.get_title_info_list_by_type(saveType, listOut); }

static void data_initialize_task(sys::threadpool::JobData taskData)
{
    auto castData         = std::static_pointer_cast<StateDataStruct>(taskData);
    sys::Task *task       = castData->task;
    const bool clearCache = castData->clearCache;

    if (error::is_null(task)) { return; }
    const char *statusFinalizing = strings::get_by_name(strings::names::DATA_LOADING_STATUS, 6);

    if (clearCache) { s_context.delete_cache(); }
    s_context.read_cache(task);
    s_context.load_application_records(task);
    s_context.import_svi_files(task);
    s_context.load_create_users(task);
    s_context.load_user_save_info(task);
    s_context.write_cache(task);

    task->set_status(statusFinalizing); // This is here so at least they know something is happening instead of a freeze.
    task->complete();
}
