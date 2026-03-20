#pragma once
#include "data/DataCommon.hpp"
#include "fslib.hpp"
#include "sdl.hpp"

#include <string>
#include <string_view>
#include <switch.h>
#include <vector>

namespace data
{
    class User;

    /// @brief Type used to store save info and play statistics in the vector. Vector is used to preserve the order since I
    /// can't use a map without having to extra heap allocate it.
    using UserDataEntry = std::pair<uint64_t, std::pair<FsSaveDataInfo, PdmPlayStatistics>>;

    /// @brief Type definition for the user save info/play stats vector.
    using UserSaveInfoList = std::vector<UserDataEntry>;

    /// @brief A vector of pointers to User instances.
    using UserList = std::vector<data::User *>;

    /// @brief Class that stores data for the user.
    class User final : public data::DataCommon
    {
        public:
            /// @brief Constructs a new user with accountID
            /// @param accountID AccountID of user.
            /// @param saveType Save data type account uses.
            User(AccountUid accountID, FsSaveDataType saveType) noexcept;

            /// @brief This is the constructor used to create the fake system users.
            /// @param accountID AccountID to associate with saveType.
            /// @param pathSafeNickname The path safe version of the save data since JKSV is in everything the Switch supports.
            /// @param iconPath Path to the icon to load for account.
            /// @param saveType Save data type of user.
            User(AccountUid accountID,
                 std::string_view nickname,
                 std::string_view pathSafeNickname,
                 FsSaveDataType saveType) noexcept;

            /// @brief Move constructor and operator.
            User(User &&user) noexcept;
            User &operator=(User &&user) noexcept;

            // Non of this around these parts.
            User(const User &)            = delete;
            User &operator=(const User &) = delete;

            /// @brief Pushes data to m_userData
            /// @param saveInfo SaveDataInfo.
            /// @param playStats Play statistics.
            void add_data(uint64_t applicationID, const FsSaveDataInfo &saveInfo, const PdmPlayStatistics &playStats);

            /// @brief Clears the user save info vector.
            void clear_data_entries() noexcept;

            /// @brief Erases data at index.
            /// @param index Index of save data info to erase.
            void erase_data(int index);

            /// @brief Runs the sort algo on the vector.
            void sort_data() noexcept;

            /// @brief Returns the account ID of the user
            AccountUid get_account_id() const noexcept;

            /// @brief Returns the primary  save data type o
            FsSaveDataType get_account_save_type() const noexcept;

            /// @brief Returns the user's full UTF-8 nickname.
            const char *get_nickname() const noexcept;

            /// @brief Returns the path safe version of the user's nickname.
            const char *get_path_safe_nickname() const noexcept;

            /// @brief Returns the total data entries.
            size_t get_total_data_entries() const noexcept;

            /// @brief Returns the application ID of the title at index.
            uint64_t get_application_id_at(int index) const noexcept;

            /// @brief Returns a pointer to the save data info at index.
            FsSaveDataInfo *get_save_info_at(int index) noexcept;

            /// @brief Returns a pointer to the play statistics at index.
            PdmPlayStatistics *get_play_stats_at(int index) noexcept;

            /// @brief Returns a pointer to the save info of applicationID.
            FsSaveDataInfo *get_save_info_by_id(uint64_t applicationID) noexcept;

            /// @brief Returns a reference to the internal map for range based loops.
            data::UserSaveInfoList &get_user_save_info_list() noexcept;

            /// @brief Returns a pointer to the play statistics of applicationID
            PdmPlayStatistics *get_play_stats_by_id(uint64_t applicationID) noexcept;

            /// @brief Erases the save info from the vector according to the pointer passed.
            void erase_save_info(const FsSaveDataInfo *saveInfo);

            /// @brief Erases a UserDataEntry according to the application ID passed.
            /// @param applicationID ID of the save to erase.
            void erase_save_info_by_id(uint64_t applicationID);

            /// @brief Loads the save data info and play statistics for the current user using the information passed to the
            /// constructor.
            void load_user_data();

            /// @brief Loads the icon from the system and converts it to a texture.
            void load_icon(sdl2::Renderer &renderer) override;

        private:
            /// @brief Account's ID
            AccountUid m_accountID{};

            /// @brief Type of save data account uses.
            FsSaveDataType m_saveType{};

            /// @brief User's nickname.
            char m_nickname[0x20]{};

            /// @brief Path safe version of nickname.
            char m_pathSafeNickname[0x20]{};

            /// @brief Vector containing save info and play statistics.
            data::UserSaveInfoList m_userData{};

            /// @brief Loads account structs from system.
            /// @param profile AccountProfile struct to write to.
            /// @param profileBase AccountProfileBase to write to.
            void load_account(AccountProfile &profile, AccountProfileBase &profileBase);

            /// @brief Creates a placeholder since something went wrong.
            void create_account();

            /// @brief Attempts to locate the data associated with applicationID
            data::UserSaveInfoList::iterator find_title_by_id(uint64_t applicationID);

            /// @brief Returns whether or not the index is within bounds.
            inline bool index_check(int index) const { return index >= 0 && index < static_cast<int>(m_userData.size()); }
    };
} // namespace data
