#pragma once
#include "fslib.hpp"
#include "sys/Task.hpp"

#include <atomic>
#include <vector>

namespace data
{
class User;
}

namespace tasks::saveimport
{
/// @brief Data passed to the batch import task.
/// @note The worker writes the atomic progress and completion flags; the UI
///       thread polls them without additional locking.
struct BatchData : sys::Task::DataStruct
{
    /// @brief User that receives every backup.
    data::User *user{};

    /// @brief Backups to import.
    std::vector<fslib::Path> paths{};

    /// @brief Number of backups imported successfully.
    std::atomic<int> succeeded{};

    /// @brief Number of backups that failed.
    std::atomic<int> failed{};

    /// @brief Set when the task starts.
    std::atomic<bool> started{};

    /// @brief Set when the task finishes.
    std::atomic<bool> finished{};
};

/// @brief Imports a random save backup according to the meta data.
/// @param taskData Job data containing the import request.
void import_save_backup(sys::threadpool::JobData taskData);

/// @brief Imports several save backups to one user.
/// @param taskData Job data containing a BatchData pointer.
void import_save_backups(sys::threadpool::JobData taskData);
} // namespace tasks::saveimport
