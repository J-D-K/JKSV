#ifndef GFX_H
#define GFX_H

#include <string>

namespace gfx
{
    class color
    {
        public:
            void fromRGBA(const uint8_t& _r, const uint8_t& _g, const uint8_t& _b, const uint8_t& _a);
            void fromU32(const uint32_t& _px);
            void invert();

            void setR(const uint8_t& _r) { rgb[3] = _r; }
            void setG(const uint8_t& _g) { rgb[2] = _g; }
            void setB(const uint8_t& _b) { rgb[1] = _b; }
            void setA(const uint8_t& _a) { rgb[0] = _a; }

            uint8_t r() { return rgb[3]; }
            uint8_t g() { return rgb[2]; }
            uint8_t b() { return rgb[1]; }
            uint8_t a() { return rgb[0]; }
            uint32_t clr() { return rgb[0] << 24 | rgb[1] << 16 | rgb[2] << 8 | rgb[3]; }

        private:
            uint8_t rgb[4];
    };

    class tex
    {
        public:
            /*
            Now uses libpng and png files
            Only accepts RGBA8 PNGs
            Others will have issues.
            */
            void loadPNGFile(const std::string& path);
            void loadJpegMem(const uint8_t *txData, const size_t& jpegSz);
            void loadJpegFile(const std::string& path);
            //Frees memory used by data
            void deleteData();
            unsigned getWidth() { return width; }
            unsigned getHeight() { return height; }
            uint32_t *getDataPointer() { return data; }

            void draw(unsigned x, unsigned y);
            void drawInvert(unsigned x, unsigned y);
            void drawNoBlend(unsigned x, unsigned y);

            //For lazy-scaling icons
            //Skips every-other pixel and row, so 1/2 scale
            void drawNoBlendSkip(unsigned x, unsigned y);
            void drawNoBlendSkipSmooth(unsigned x, unsigned y);

            //These only repeat pixels.
            void drawRepeatHori(unsigned x, unsigned y, unsigned w);
            void drawRepeatHoriNoBlend(unsigned x, unsigned y, unsigned w);

            void drawRepeatVert(unsigned x, unsigned y, unsigned h);
            void drawRepeatVertNoBlend(unsigned x, unsigned y, unsigned h);

        protected:
            size_t sz;
            unsigned width, height;
            uint32_t *data = NULL;
    };


    //Inits graphics and shared font. Code for shared font is from switch-portlibs examples
    bool init();
    bool exit();

    //Changes gfx mode to linear double
    void switchMode();

    void handleBuffs();

    //Clears framebuffer to clr. RGBA8
    void clearBufferColor(const uint32_t& clr);

    //Draws text using shared font.
    void drawText(const std::string& str, unsigned x, unsigned y, const unsigned& sz, const uint32_t& clr);

    //Returns width of str
    unsigned getTextWidth(const std::string& str, const unsigned& sz);

    //Returns height of text if using sz
    unsigned getTextHeight(const unsigned& sz);

    //Draws a rectangle. clr = RGBA8. Be careful not to go outside framebuffer
    void drawRectangle(const unsigned& x, const unsigned& y, const unsigned& width, const unsigned& height, const uint32_t& clr);
}

#endif // GFX_H
