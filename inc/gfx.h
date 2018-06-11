#ifndef GFX_H
#define GFX_H

#include <string>

namespace gfx
{
    bool init(const uint32_t& _fontSize);
    bool fini();

    void switchMode();
    void handleBuffs();

    void clearConsoleColor(const uint32_t& clr);
    void drawText(const std::string& str, unsigned x, unsigned y, const uint32_t& sz, const uint32_t& clr);
    void drawRectangle(uint32_t x, uint32_t y, const uint32_t& width, const uint32_t& height, const uint32_t& clr);

    class tex
    {
        public:
            void loadFromFile(const std::string& path);
            ~tex();
            void draw(uint32_t x, uint32_t y);

        private:
            uint32_t sz;
            uint16_t width, height;
            uint8_t *data = NULL;
    };
}

#endif // GFX_H
