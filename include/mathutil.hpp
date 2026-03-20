#pragma once

namespace math
{
    template <typename Type>
        requires std::integral<Type> || std::floating_point<Type>
    class Util
    {
        public:
            /// @brief Calculates the distance between two Types and returns the result as a positive Type.
            static inline constexpr Type absolute_distance(Type a, Type b) noexcept { return a > b ? a - b : b - a; }

            /// @brief Returns size centered within area.
            static inline constexpr Type center_within(Type area, Type size) noexcept { return (area / 2) - (size / 2); }
    };

}
