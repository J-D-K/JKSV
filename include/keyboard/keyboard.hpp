#pragma once
#include "keyboard/Dictionary.hpp"

#include <functional>
#include <optional>
#include <string_view>
#include <switch.h>

namespace keyboard
{
    /// @brief Gets input using the Switch's keyboard.
    /// @param keyboardType Type of keyboard shown.
    /// @param defaultText The default text in the keyboard.
    /// @param header The header of the keyboard.
    /// @param stringOut Pointer to buffer to write to.
    /// @param stringLength Size of the buffer to write too.
    /// @param dictionary Optional. Dictionary/words.
    /// @return True if input was successful and valid. False if it wasn't.
    bool get_input(SwkbdType keyboardType,
                   std::string_view defaultText,
                   std::string_view header,
                   char *stringOut,
                   size_t stringLength,
                   std::optional<std::reference_wrapper<keyboard::Dictionary>> dictionary = std::nullopt);
} // namespace keyboard
