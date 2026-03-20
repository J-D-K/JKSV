#pragma once
#include <string_view>

namespace graphics::targets
{
    namespace names
    {
        inline constexpr std::string_view SECONDARY = "SecondaryTarget";
    }

    namespace dims
    {
        // Dimensions of the secondary render target.
        inline constexpr int SECONDARY_WIDTH  = 1080;
        inline constexpr int SECONDARY_HEIGHT = 555;
    }
}