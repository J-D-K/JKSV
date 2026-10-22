#pragma once
#include "config/keys.hpp"
#include "fslib.hpp"

#include <string>
#include <string_view>
#include <vector>

/// @brief This API acts like a passthrough for the context so the context doesn't need to be global.
namespace config
{
    /// @brief Attempts to load config from file. If it fails, loads defaults.
    void initialize();

    /// @brief Resets config to default values.
    void reset_to_default();

    /// @brief Saves config to file.
    void save();

    /// @brief Retrieves the config value according to the key passed.
    /// @param key Key to retrieve. See config::keys
    /// @return Key's value if found. 0 if it is not.
    uint8_t get_by_key(std::string_view key) noexcept;

    /// @brief Toggles the key. This is only for basic true or false settings.
    /// @param key Key to toggle.
    void toggle_by_key(std::string_view key) noexcept;

    /// @brief Sets the key according
    /// @param key Key to set.
    /// @param value Value to set the key to.
    void set_by_key(std::string_view key, uint8_t value) noexcept;

    /// @brief Returns the working directory.
    /// @return Working directory.
    fslib::Path get_working_directory();

    /// @brief Attempts to set the working directory to the one passed.
    /// @param path Path for JKSV to use.
    bool set_working_directory(const fslib::Path &path) noexcept;

    /// @brief Returns the scaling speed of UI transitions and animations.
    /// @return Scaling variable.
    double get_animation_scaling() noexcept;

    /// @brief Sets the UI animation scaling.
    /// @param newScale New value to set the scaling to.
    void set_animation_scaling(double newScale) noexcept;

    /// @brief Adds or removes a title from the favorites list.
    /// @param applicationID Application ID of title to add or remove.
    void add_remove_favorite(uint64_t applicationID);

    /// @brief Returns if the title is found in the favorites list.
    /// @param applicationID Application ID to search for.
    /// @return True if found. False if not.
    bool is_favorite(uint64_t applicationID) noexcept;

    /// @brief Adds or removes title from blacklist.
    /// @param applicationID Application ID to add or remove.
    void add_remove_blacklist(uint64_t applicationID) noexcept;

    /// @brief Gets the currently blacklisted application IDs.
    /// @param listOut Vector to store application IDs to.
    void get_blacklisted_titles(std::vector<uint64_t> &listOut);

    /// @brief Returns if the title is found in the blacklist.
    /// @param applicationID Application ID to search for.
    /// @return True if found. False if not.
    bool is_blacklisted(uint64_t applicationID) noexcept;

    /// @brief Returns whether or not the blacklist is empty.
    bool blacklist_is_empty() noexcept;

    /// @brief Adds a custom output path for the title.
    /// @param applicationID Application ID of title to add a path for.
    /// @param customPath Path to assign to the output.
    void add_custom_path(uint64_t applicationID, std::string_view customPath);

    /// @brief Searches to see if the application ID passed has a custom output path.
    /// @param applicationID Application ID to check.
    /// @return True if it does. False if it doesn't.
    bool has_custom_path(uint64_t applicationID) noexcept;

    /// @brief Gets the custom, defined path for the title.
    /// @param applicationID Application ID of title to get.
    /// @param pathOut Buffer to write the path to.
    /// @param pathOutSize Size of the buffer to write the path to.
    void get_custom_path(uint64_t applicationID, char *pathOut, size_t pathOutSize);

    /// @brief Returns the last IP used to connect as a receiver.
    const std::string &get_peer_last_ip() noexcept;

    /// @brief Sets and persists the last IP used to connect as a receiver.
    void set_peer_last_ip(std::string_view ip);
} // namespace config
