#pragma once
#include "data/DataCommon.hpp"
#include "data/TitleInfo.hpp"
#include "data/User.hpp"
#include "sdl.hpp"
#include "sys/Task.hpp"

#include <mutex>
#include <unordered_map>
#include <vector>

namespace data
{
    class DataContext
    {
        public:
            /// @brief Default constructor.
            DataContext() = default;

            /// @brief Loads the users from the system and creates the accounts for system users.
            bool load_create_users(sys::Task *task);

            /// @brief Loops and runs the load routine for all users found.
            void load_user_save_info(sys::Task *task);

            /// @brief Gets a vector of pointers to the users loaded.
            void get_users(data::UserList &listOut);

            /// @brief Loads the titles from the Switch's application records.
            void load_application_records(sys::Task *task);

            /// @brief Returns whether a title is loaded with the application ID passed.
            bool title_is_loaded(uint64_t applicationID) noexcept;

            /// @brief Attempts to load a title with the application ID passed.
            void load_title(uint64_t applicationID);

            /// @brief Returns the title info mapped to applicationID. nullptr on not found.
            data::TitleInfo *get_title_by_id(uint64_t applicationID) noexcept;

            /// @brief Gets a vector of pointers to all of the current title info instances.
            void get_title_info_list(data::TitleInfoList &listOut);

            /// @brief Gets a list of title info that has savedata for type.
            void get_title_info_list_by_type(FsSaveDataType type, data::TitleInfoList &listOut);

            /// @brief Imports the SVI files from the SD card.
            void import_svi_files(sys::Task *task);

            /// @brief Deletes the cache file if it exists.
            void delete_cache();

            /// @brief Attempts to read the cache file from the SD card.
            bool read_cache(sys::Task *task);

            /// @brief Writes the cache to file.
            bool write_cache(sys::Task *task);

            /// @brief Processes the icon queue.
            void process_icon_queue(sdl2::Renderer &renderer);

        private:
            /// @brief User vector.
            std::vector<data::User> m_users{};

            /// @brief Map of titles paired with their application ID.
            std::unordered_map<uint64_t, data::TitleInfo> m_titleInfo{};

            /// @brief Queue of the above to process the icons.
            std::vector<data::DataCommon *> m_iconQueue{};

            /// @brief Mutex for users.
            std::mutex m_userMutex{};

            /// @brief Mutex for titles.
            std::mutex m_titleMutex{};

            /// @brief Mutex to make sure the icon queue doesn't get mutilated.
            std::mutex m_iconQueueMutex{};

            /// @brief Whether or not the cache is still valid.
            bool m_cacheIsValid{};
    };
}
