#pragma once
#include <string_view>

namespace graphics::fonts
{
    namespace names
    {
        // These are all the internal names of the fonts so the mananger pulls and reuses them.
        inline constexpr std::string_view FOURTEEN_PIXEL    = "FourteenPixel";
        inline constexpr std::string_view TWENTY_PIXEL      = "TwentyPixel";
        inline constexpr std::string_view TWENTY_TWO_PIXEL  = "TwentyTwoPixel";
        inline constexpr std::string_view TWENTY_FOUR_PIXEL = "TwentyFourPixel";
        inline constexpr std::string_view THIRTY_TWO_PIXEL  = "ThirtyTwoPixel";
        inline constexpr std::string_view THIRTY_FOUR_PIXEL = "ThirtyFourPixel";
    }

    namespace sizes
    {
        // These are the pixel sizes.
        inline constexpr int FOURTEEN_PIXEL    = 14;
        inline constexpr int TWENTY_PIXEL      = 20;
        inline constexpr int TWENTY_TWO_PIXEL  = 22;
        inline constexpr int TWENTY_FOUR_PIXEL = 24;
        inline constexpr int THIRTY_TWO_PIXEL  = 32;
        inline constexpr int THIRTY_FOUR_PIXEL = 34;
    }

}