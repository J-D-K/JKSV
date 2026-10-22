#include "config/config.hpp"

#include "config/ConfigContext.hpp"
#include "logging/logger.hpp"

namespace
{
    config::ConfigContext s_context{};
}

void config::initialize()
{
    // Ensure the directory actually exists.
    s_context.create_directory();

    // This will load the config if the file is found.
    s_context.load();
}

void config::reset_to_default() { s_context.initialize(); }

void config::save() { s_context.save(); }

uint8_t config::get_by_key(std::string_view key) noexcept { return s_context.get_by_key(key); }

void config::toggle_by_key(std::string_view key) noexcept { s_context.toggle_by_key(key); }

void config::set_by_key(std::string_view key, uint8_t value) noexcept { s_context.set_by_key(key, value); }

fslib::Path config::get_working_directory() { return s_context.get_working_directory(); }

bool config::set_working_directory(const fslib::Path &path) noexcept { return s_context.set_working_directory(path); }

double config::get_animation_scaling() noexcept { return s_context.get_animation_scaling(); }

void config::set_animation_scaling(double newScale) noexcept { s_context.set_animation_scaling(newScale); }

void config::add_remove_favorite(uint64_t applicationID)
{
    const bool favorite = s_context.is_favorite(applicationID);
    if (favorite) { s_context.remove_favorite(applicationID); }
    else { s_context.add_favorite(applicationID); }
}

bool config::is_favorite(uint64_t applicationID) noexcept { return s_context.is_favorite(applicationID); }

void config::add_remove_blacklist(uint64_t applicationID)
{
    const bool blacklisted = s_context.is_blacklisted(applicationID);
    if (blacklisted) { s_context.remove_from_blacklist(applicationID); }
    else { s_context.add_to_blacklist(applicationID); }
}

void config::get_blacklisted_titles(std::vector<uint64_t> &listOut) { s_context.get_blacklist(listOut); }

bool config::is_blacklisted(uint64_t applicationID) noexcept { return s_context.is_blacklisted(applicationID); }

bool config::blacklist_is_empty() noexcept { return s_context.blacklist_empty(); }

void config::add_custom_path(uint64_t applicationID, std::string_view customPath)
{
    s_context.add_custom_path(applicationID, customPath);
}

bool config::has_custom_path(uint64_t applicationID) noexcept { return s_context.has_custom_path(applicationID); }

void config::get_custom_path(uint64_t applicationID, char *pathOut, size_t pathOutSize)
{
    s_context.get_custom_path(applicationID, pathOut, pathOutSize);
}

const std::string &config::get_peer_last_ip() noexcept { return s_context.get_peer_last_ip(); }

void config::set_peer_last_ip(std::string_view ip) { s_context.set_peer_last_ip(ip); }
