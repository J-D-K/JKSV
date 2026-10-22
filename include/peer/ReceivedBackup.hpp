#pragma once
#include "fs/SaveMetaData.hpp"
#include "fslib.hpp"

#include <cstdint>
#include <string>

namespace peer
{
/// @brief A backup received during a peer batch.
struct ReceivedBackup
{
    /// @brief Final path of the received backup.
    fslib::Path path{};

    /// @brief Save metadata read from the received backup.
    fs::SaveMetaData metadata{};

    /// @brief Application ID from the metadata.
    uint64_t applicationID{};

    /// @brief Game folder name the backup was received into.
    std::string folderName{};

    /// @brief Whether valid metadata was read from the file.
    bool metadataValid{};
};
} // namespace peer
