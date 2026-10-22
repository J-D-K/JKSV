#pragma once
#include "fslib.hpp"

#include <cstdint>
#include <json-c/json.h>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace config
{
    class ConfigContext
    {
        public:
            /// @brief Constructor. Initializes the map with default values.
            ConfigContext();

            /// @brief Ensures the config directory exists.
            void create_directory();

            /// @brief Resets and saves the config back to the default values.
            void initialize();

            /// @brief Loads the config and custom paths files from the SD card if possible.
            bool load();

            /// @brief Saves the config to the SD if possible.
            void save();

            /// @brief Attempts to find and return the value of the key passed.
            uint8_t get_by_key(std::string_view key) const noexcept;

            /// @brief Attempts to toggle the value for the key passed. For simple 1 and 0.
            void toggle_by_key(std::string_view key) noexcept;

            /// @brief Attempts to set the key passed to the value passd.
            void set_by_key(std::string_view key, uint8_t value) noexcept;

            /// @brief Returns the current working directory.
            fslib::Path get_working_directory() const;

            /// @brief Sets the current work directory if the path passed is valid.
            bool set_working_directory(const fslib::Path &workDir) noexcept;

            /// @brief Returns the transition scaling speed.
            double get_animation_scaling() const noexcept;

            /// @brief Sets the current animation scaling speed.
            void set_animation_scaling(double scaling) noexcept;

            /// @brief Adds a favorite to the favorite titles.
            void add_favorite(uint64_t applicationID);

            /// @brief Removes a title from the favorites.
            void remove_favorite(uint64_t applicationID) noexcept;

            /// @brief Returns if the application ID passed is a favorite.
            bool is_favorite(uint64_t applicationID) const noexcept;

            /// @brief Adds the application ID passed to the blacklist.
            void add_to_blacklist(uint64_t applicationID);

            /// @brief Removes the application ID passed from the blacklist.
            void remove_from_blacklist(uint64_t applicationID) noexcept;

            /// @brief Writes all of the currently blacklisted titles to the vector passed.
            void get_blacklist(std::vector<uint64_t> &listOut);

            /// @brief Returns if the application ID passed is found in the blacklist.
            bool is_blacklisted(uint64_t applicationID) const noexcept;

            /// @brief Returns if the blacklist is empty.
            bool blacklist_empty() const noexcept;

            /// @brief Returns if the application ID passed has a custom output path.
            bool has_custom_path(uint64_t applicationID) const noexcept;

            /// @brief Adds a new output path.
            void add_custom_path(uint64_t applicationID, std::string_view newPath);

            /// @brief Gets the custom output path of the applicationID passed.
            void get_custom_path(uint64_t applicationID, char *buffer, size_t bufferSize);

            /// @brief Returns the last IP used to connect as a receiver.
            const std::string &get_peer_last_ip() const noexcept;

            /// @brief Sets and persists the last IP used to connect as a receiver.
            void set_peer_last_ip(std::string_view ip);

        private:
            /// @brief This is where the majority of the config values are.
            std::unordered_map<std::string_view, uint8_t> m_configMap{};

            /// @brief The main directory JKSV operates from.
            fslib::Path m_workingDirectory{};

            /// @brief This scales the transitions.
            double m_animationScaling{};

            /// @brief This holds the favorite titles.
            std::unordered_set<uint64_t> m_favorites{};

            /// @brief This holds the blacklisted titles.
            std::unordered_set<uint64_t> m_blacklist{};

            /// @brief This holds user defined paths.
            std::unordered_map<uint64_t, std::string> m_paths{};

            /// @brief Last IP entered manually when receiving a peer transfer.
            std::string m_peerLastIp{};

            /// @brief Loads the config file from SD.
            bool load_config_file();

            /// @brief Saves the config to the SD.
            void save_config_file();

            /// @brief Reads a json array to the vector passed.
            void read_array_to_set(std::unordered_set<uint64_t> &set, json_object *array);

            /// @brief Loads the custom output path file from the SD if possible.
            void load_custom_paths();

            /// @brief Saves the custom output paths to the SD card.
            void save_custom_paths();
    };
}
