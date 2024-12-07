#pragma once
#include <cstdint>

namespace UI
{
    // I got tired of repeating this for every class that needs to render a bounding box
    class ColorMod
    {
        public:
            ColorMod(void) = default;

            // Updates the color mod.
            void Update(void);
            // Allows me to use this like the class isn't even there.
            operator uint8_t(void) const;

        private:
            // Which direction the shift is going. True = add, false = subtract;
            bool m_Direction = true;
            // Current mod value.
            uint8_t m_ColorMod = 0;
    };
} // namespace UI
