#pragma once
#include "data/DataCommon.hpp"
#include "sdl.hpp"

#include <cstdint>
#include <memory>
#include <switch.h>
#include <vector>

namespace data
{
    class TitleInfo;

    /// @brief Vector of pointers to titleinfo instances.
    using TitleInfoList = std::vector<data::TitleInfo *>;

    /// @brief Class that holds data related to titles loaded from the system.
    class TitleInfo final : public data::DataCommon
    {
        public:
            /// @brief Constructs a TitleInfo instance. Loads control data, icon.
            /// @param applicationID Application ID of title to load.
            TitleInfo(uint64_t applicationID) noexcept;

            /// @brief Initializes a TitleInfo instance using external (cached) NsApplicationControlData
            /// @param applicationID Application ID of the title loaded from cache.
            /// @param controlData Reference to the control data to init from.
            TitleInfo(uint64_t applicationID, NsApplicationControlData &controlData) noexcept;

            // None of this nonesense around these parts.
            TitleInfo(const TitleInfo &)            = delete;
            TitleInfo &operator=(const TitleInfo &) = delete;

            /// @brief Returns the application ID of the title.
            /// @return Title's application ID.
            uint64_t get_application_id() const noexcept;

            /// @brief Returns a pointer to the control data for the title.
            /// @return Pointer to control data.
            const NsApplicationControlData *get_control_data() const noexcept;

            /// @brief Returns whether or not the title has control data.
            /// @return Whether or not the title has control data.
            bool has_control_data() const noexcept;

            /// @brief Returns the title of the title?
            /// @return Title directly from the NACP.
            const char *get_title() const noexcept;

            /// @brief Returns the path safe version of the title for file system usage.
            /// @return Path safe version of the title.
            const char *get_path_safe_title() const noexcept;

            /// @brief Returns the publisher of the title.
            /// @return Publisher string from NACP.
            const char *get_publisher() const noexcept;

            /// @brief Returns the owner ID of the save data.
            uint64_t get_save_data_owner_id() const noexcept;

            /// @brief Returns the save data container's base size.
            /// @param saveType Type of save data to return.
            /// @return Size of baseline save data if applicable. If not, 0.
            int64_t get_save_data_size(uint8_t saveType) const noexcept;

            /// @brief Returns the maximum size of the save data container.
            /// @param saveType Type of save data to return.
            /// @return Maximum size of the save container if applicable. If not, 0.
            int64_t get_save_data_size_max(uint8_t saveType) const noexcept;

            /// @brief Returns the journaling size for the save type passed.
            /// @param saveType Save type to return.
            /// @return Journal size if applicable. If not, 0.
            int64_t get_journal_size(uint8_t saveType) const noexcept;

            /// @brief Returns the maximum journal size for the save type passed.
            /// @param saveType Save type to return.
            /// @return Maximum journal size if applicable. If not, 0.
            int64_t get_journal_size_max(uint8_t saveType) const noexcept;

            /// @brief Returns if a title uses the save type passed.
            /// @param saveType Save type to check for.
            /// @return True on success. False on failure.
            bool has_save_data_type(uint8_t saveType) const noexcept;

            /// @brief Allows the path safe title to be set to a new path.
            /// @param newPathSafe Buffer containing the new safe path to use.
            void set_path_safe_title(const char *newPathSafe) noexcept;

            /// @brief Loads the icon from the nacp.
            void load_icon(sdl2::Renderer &renderer) override;

        private:
            /// @brief This defines how long the buffer is for the path safe version of the title.
            static inline constexpr size_t SIZE_PATH_SAFE = 0x200;

            /// @brief Stores application ID for easier grabbing since JKSV is all pointers.
            uint64_t m_applicationID{};

            /// @brief Where all the good stuff is.
            NsApplicationControlData m_data{};

            /// @brief Stores the pointer to the language entry of the title.
            NacpLanguageEntry *m_entry{};

            /// @brief Saves whether or not the title has control data.
            bool m_hasData{};

            /// @brief This is the path safe version of the title.
            char m_pathSafeTitle[TitleInfo::SIZE_PATH_SAFE]{};

            /// @brief Shared icon texture.
            sdl2::SharedTexture m_icon{};

            /// @brief Private function to get/create the path safe title.
            void get_create_path_safe_title() noexcept;

            /// @brief Checks and returns whether or not a US English or GB English title exists.
            /// @param titleOut Pointer to pointer to store title pointer.
            bool get_english_title(const char **titleOut) const noexcept;
    };
} // namespace data
