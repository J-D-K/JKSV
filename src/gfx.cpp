#include <string>
#include <cstring>
#include <cstdio>
#include <switch.h>
#include <png.h>
#include <jpeglib.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "gfx.h"
#include "ui.h"

static FT_Error ret = 0, libret = 1, faceret = 1;
static FT_Library lib;
static FT_Face face;
static uint32_t fbw = 0;

static inline uint32_t blend(gfx::color& px, gfx::color& fb)
{
    //Skip fully transparent and solid pixels and not waste time
    if(px.a() == 0)
        return fb.clr();
    else if(px.a() == 0xFF)
        return px.clr();

    uint8_t subAl = (uint8_t)0xFF - px.a();

    uint8_t finalRed = (px.r() * px.a() + fb.r() * subAl) / 0xFF;
    uint8_t finalGreen = (px.g() * px.a() + fb.g() * subAl) / 0xFF;
    uint8_t finalBlue = (px.b() * px.a() + fb.b() * subAl) / 0xFF;

    return (0xFF << 24 | finalBlue << 16 | finalGreen << 8 | finalRed);
}

namespace gfx
{
    bool init()
    {
        Result res = 0;

        gfxInitDefault();
        consoleInit(NULL);

        PlFontData font;

        res = plInitialize();
        if(R_FAILED(res))
        {
            printf("plInitialize failed.");
            return false;
        }

        res = plGetSharedFontByType(&font, PlSharedFontType_KO);
        if(R_FAILED(res))
        {
            printf("plGetSharedFontByTypeFailed!");
            return false;
        }

        ret = FT_Init_FreeType(&lib);
        libret = ret;
        if(ret)
        {
            printf("FT_Init_FreeType() failed: %d", ret);
            return false;
        }

        ret = FT_New_Memory_Face(lib, (FT_Byte *)font.address, font.size, 0, &face);
        faceret = ret;
        if(ret)
        {
            printf("FT_New_Memory_Face failed: %d", ret);
            return false;
        }

        gfxSetMode(GfxMode_LinearDouble);

        return true;
    }

    bool exit()
    {
        if(faceret == 0)
            FT_Done_Face(face);
        if(libret == 0)
            FT_Done_FreeType(lib);

        plExit();
        gfxExit();

        return true;
    }

    void handleBuffs()
    {
        gfxFlushBuffers();
        gfxSwapBuffers();
        gfxWaitForVsync();
    }

    //switch-portlibs examples.
    void drawGlyph(const FT_Bitmap& bmp, uint32_t *fb, unsigned _x, unsigned _y, color& txtClr)
    {
        if(bmp.pixel_mode != FT_PIXEL_MODE_GRAY)
            return;

        uint8_t *imgPtr = bmp.buffer;
        color fbPx;
        for(unsigned y = _y; y < _y + bmp.rows; y++)
        {
            uint32_t *rowPtr = &fb[y * fbw + _x];
            for(unsigned x = _x; x < _x + bmp.width; x++, imgPtr++, rowPtr++)
            {
                if(*imgPtr > 0)
                {
                    fbPx.fromU32(*rowPtr);
                    txtClr.setA(*imgPtr);

                    *rowPtr = blend(txtClr, fbPx);
                }
            }
        }
    }

    void drawText(const std::string& str, unsigned x, unsigned y, const unsigned& sz, const uint32_t& clr)
    {
        int tmpX = x;
        FT_Error ret = 0;
        FT_GlyphSlot slot = face->glyph;
        uint32_t tmpChr;
        ssize_t unitCount = 0;

        //Set's font height to size. Easier to work with
        FT_Set_Char_Size(face, 0, sz * 64, 90, 90);
        unsigned height = sz;

        uint32_t *fb = (uint32_t *)gfxGetFramebuffer(&fbw, NULL);

        color textColor;
        textColor.fromU32(clr);

        for(unsigned i = 0; i < str.length(); )
        {
            unitCount = decode_utf8(&tmpChr, (uint8_t *)&str.data()[i]);
            if(unitCount <= 0)
                break;

            i += unitCount;
            if(tmpChr == '\n')
            {
                tmpX = x;
                y += height + 8;
                continue;
            }

            ret = FT_Load_Glyph(face, FT_Get_Char_Index(face, tmpChr), FT_LOAD_RENDER);

            if(ret)
                return;

            unsigned drawY = y + (height - slot->bitmap_top);

            drawGlyph(slot->bitmap, fb, tmpX + slot->bitmap_left, drawY, textColor);
            tmpX += slot->advance.x >> 6;
        }
    }

    unsigned getTextWidth(const std::string& str, const unsigned& sz)
    {
        unsigned width = 0;

        uint32_t unitCount = 0, tmpChr = 0;
        FT_UInt glyphIndex = 0;
        FT_GlyphSlot slot = face->glyph;
        FT_Error ret = 0;

        FT_Set_Char_Size(face, 0, 64 * sz, 90, 90);

        for(unsigned i = 0; i < str.length(); )
        {
            unitCount = decode_utf8(&tmpChr, (uint8_t *)&str.data()[i]);

            if(unitCount <= 0)
                break;

            i += unitCount;
            glyphIndex = FT_Get_Char_Index(face, tmpChr);
            ret = FT_Load_Glyph(face, glyphIndex, FT_LOAD_RENDER);

            if(ret)
                return 0;

            width += slot->advance.x >> 6;
        }

        return width;
    }

    unsigned getTextHeight(const unsigned& sz)
    {
        //Pointless now, but keep
        return sz;
    }

    void clearBufferColor(const uint32_t& clr)
    {
        size_t fbSize = gfxGetFramebufferSize();
        uint32_t *fb = (uint32_t *)gfxGetFramebuffer(NULL, NULL);
        std::memset(fb, clr, fbSize);
    }

    void drawRectangle(const unsigned& x, const unsigned& y, const unsigned& width, const unsigned& height, const unsigned& clr)
    {
        uint32_t tX, tY;
        uint32_t *fb = (uint32_t *)gfxGetFramebuffer(NULL, NULL);

        for(tY = y; tY < y + height; tY++)
        {
            uint32_t *rowPtr = &fb[tY * fbw + x];
            for(tX = x; tX < x + width; tX++)
            {
                *rowPtr++ = clr;
            }
        }
    }

    void color::fromRGBA(const uint8_t& _r, const uint8_t& _g, const uint8_t& _b, const uint8_t& _a)
    {
        rgb[0] = _a;
        rgb[1] = _b;
        rgb[2] = _g;
        rgb[3] = _r;
    }

    void color::fromU32(const uint32_t& _px)
    {
        rgb[0] = _px >> 24 & 0xFF;
        rgb[1] = _px >> 16 & 0xFF;
        rgb[2] = _px >>  8 & 0xFF;
        rgb[3] = _px & 0xFF;
    }

    void color::invert()
    {
        rgb[3] = 0xFF - rgb[3];
        rgb[2] = 0xFF - rgb[2];
        rgb[1] = 0xFF - rgb[1];
    }

    void tex::loadPNGFile(const std::string& path)
    {
        FILE *pngIn = fopen(path.c_str(), "rb");
        if(pngIn != NULL)
        {
            png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
            if(png == 0)
                return;

            png_infop pngInfo = png_create_info_struct(png);
            if(pngInfo == 0)
                return;

            int jmp = setjmp(png_jmpbuf(png));
            if(jmp)
                return;

            png_init_io(png, pngIn);

            png_read_info(png, pngInfo);

            if(png_get_color_type(png, pngInfo) != PNG_COLOR_TYPE_RGBA)
                return;

            width = png_get_image_width(png, pngInfo);
            height = png_get_image_height(png, pngInfo);

            data = new uint32_t[width * height];

            png_bytep *rows = new png_bytep[sizeof(png_bytep) * height];
            for(unsigned i = 0; i < height; i++)
                rows[i] = new png_byte[png_get_rowbytes(png, pngInfo)];

            png_read_image(png, rows);

            uint32_t *dataPtr = &data[0];
            for(unsigned y = 0; y < height; y++)
            {
                uint32_t *rowPtr = (uint32_t *)rows[y];
                for(unsigned x = 0; x < width; x++)
                    *dataPtr++ = *rowPtr++;
            }

            for(unsigned i = 0; i < height; i++)
                delete rows[i];

            delete[] rows;

            png_destroy_read_struct(&png, &pngInfo, NULL);
        }
        fclose(pngIn);
    }

    void tex::loadJpegMem(const uint8_t *txData, const size_t& jpegSz)
    {
        struct jpeg_decompress_struct jpegInfo;
        struct jpeg_error_mgr error;

        jpegInfo.err = jpeg_std_error(&error);

        jpeg_create_decompress(&jpegInfo);
        jpeg_mem_src(&jpegInfo, txData, jpegSz);
        jpeg_read_header(&jpegInfo, true);

        //make sure we have RGB
        if(jpegInfo.jpeg_color_space == JCS_YCbCr)
            jpegInfo.out_color_space = JCS_RGB;

        width = jpegInfo.image_width;
        height = jpegInfo.image_height;

        data = new uint32_t[width * height];

        jpeg_start_decompress(&jpegInfo);

        //Do it line by line. All at once doesn't seem to work?
        JSAMPARRAY row = (JSAMPARRAY)new JSAMPARRAY[sizeof(JSAMPROW)];
        row[0] = (JSAMPROW)new JSAMPROW[(sizeof(JSAMPLE) * width) * 3];

        //Makes stuff easier
        uint8_t *ptr;
        uint32_t *dataPtr = &data[0];
        for(unsigned y = 0; y < height; y++)
        {
            unsigned x;
            jpeg_read_scanlines(&jpegInfo, row, 1);
            for(x = 0, ptr = row[0]; x < width; x++, ptr += 3)
            {
                *dataPtr++ = (0xFF << 24 | ptr[2] << 16 | ptr[1] << 8 | ptr[0]);
            }
        }

        jpeg_finish_decompress(&jpegInfo);
        jpeg_destroy_decompress(&jpegInfo);

        delete row[0];
        delete[] row;
    }

    void tex::loadJpegFile(const std::string& path)
    {
        FILE *jpegFile = fopen(path.c_str(), "rb");
        if(jpegFile != NULL)
        {
            struct jpeg_decompress_struct jpegInfo;
            struct jpeg_error_mgr error;

            jpegInfo.err = jpeg_std_error(&error);

            jpeg_create_decompress(&jpegInfo);
            jpeg_stdio_src(&jpegInfo, jpegFile);
            jpeg_read_header(&jpegInfo, true);

            //make sure we have RGB
            if(jpegInfo.jpeg_color_space == JCS_YCbCr)
                jpegInfo.out_color_space = JCS_RGB;

            width = jpegInfo.image_width;
            height = jpegInfo.image_height;

            data = new uint32_t[width * height];

            jpeg_start_decompress(&jpegInfo);

            //Do it line by line. All at once doesn't seem to work?
            JSAMPARRAY row = (JSAMPARRAY)new JSAMPARRAY[sizeof(JSAMPROW)];
            row[0] = (JSAMPROW)new JSAMPROW[(sizeof(JSAMPLE) * width) * 3];

            //Makes stuff easier
            uint8_t *ptr;
            uint32_t *dataPtr = &data[0];
            for(unsigned y = 0; y < height; y++)
            {
                unsigned x;
                jpeg_read_scanlines(&jpegInfo, row, 1);
                for(x = 0, ptr = row[0]; x < width; x++, ptr += 3)
                {
                    *dataPtr++ = (0xFF << 24 | ptr[2] << 16 | ptr[1] << 8 | ptr[0]);
                }
            }

            jpeg_finish_decompress(&jpegInfo);
            jpeg_destroy_decompress(&jpegInfo);

            delete row[0];
            delete[] row;
        }
        fclose(jpegFile);
    }

    void tex::deleteData()
    {
        if(data != NULL)
        {
            delete[] data;
            data = NULL;
        }
    }

    void tex::draw(unsigned x, unsigned y)
    {
        if(data != NULL)
        {
            unsigned tY, tX;
            uint32_t *fb = (uint32_t *)gfxGetFramebuffer(NULL, NULL);

            color dataClr, fbClr;

            uint32_t *dataPtr = &data[0];
            for(tY = y; tY < y + height; tY++)
            {
                uint32_t *rowPtr = &fb[tY * fbw + x];
                for(tX = x; tX < x + width; tX++, rowPtr++)
                {
                    dataClr.fromU32(*dataPtr++);
                    fbClr.fromU32(*rowPtr);
                    *rowPtr = blend(dataClr, fbClr);
                }
            }
        }
    }

    void tex::drawInvert(unsigned x, unsigned y)
    {
        if(data != NULL)
        {
            unsigned tX, tY;
            uint32_t *fb = (uint32_t *)gfxGetFramebuffer(NULL, NULL);

            color dataClr, fbClr;
            uint32_t *dataPtr = &data[0];
            for(tY = y; tY < y + height; tY++)
            {
                uint32_t *rowPtr = &fb[tY * fbw + x];
                for(tX = x; tX < x + width; tX++, rowPtr++)
                {
                    dataClr.fromU32(*dataPtr++);
                    dataClr.invert();
                    fbClr.fromU32(*rowPtr);
                    *rowPtr = blend(dataClr, fbClr);
                }
            }
        }
    }

    void tex::drawNoBlend(unsigned x, unsigned y)
    {
        if(data != NULL)
        {
            unsigned tY, tX;
            uint32_t *fb = (uint32_t *)gfxGetFramebuffer(NULL, NULL);

            uint32_t *dataPtr = &data[0];
            for(tY = y; tY < y + height; tY++)
            {
                uint32_t *rowPtr = &fb[tY * fbw + x];
                for(tX = x; tX < x + width; tX++)
                {
                    *rowPtr++ = *dataPtr++;
                }
            }
        }
    }

    void tex::drawNoBlendSkip(unsigned x, unsigned y)
    {
        if(data != NULL)
        {
            unsigned tY, tX;
            uint32_t *fb = (uint32_t *)gfxGetFramebuffer(NULL, NULL);

            uint32_t *dataPtr = &data[0];
            for(tY = y; tY < y + (height / 2); tY++, dataPtr += width)
            {
                uint32_t *rowPtr = &fb[tY * fbw + x];
                for(tX = x; tX < x + (width / 2); tX++, dataPtr += 2)
                {
                    *rowPtr++ = *dataPtr;
                }
            }
        }
    }

    uint32_t smooth(color& px1, color& px2)
    {
        uint8_t fR = (px1.r() + px2.r()) / 2;
        uint8_t fG = (px1.g() + px2.g()) / 2;
        uint8_t fB = (px1.b() + px2.b()) / 2;

        return 0xFF << 24 | fB << 16 | fG << 8 | fR;
    }

    void tex::drawNoBlendSkipSmooth(unsigned x, unsigned y)
    {
        if(data != NULL)
        {
            unsigned tY, tX;
            uint32_t *fb = (uint32_t *)gfxGetFramebuffer(NULL, NULL);

            uint32_t *dataPtr = &data[0];
            color px1, px2;
            for(tY = y; tY < y + (height / 2); tY++, dataPtr += width)
            {
                uint32_t *rowPtr = &fb[tY * fbw + x];
                for(tX = x; tX < x + (width / 2); tX++)
                {
                    px1.fromU32(*dataPtr++);
                    px2.fromU32(*dataPtr++);
                    *rowPtr++ = smooth(px1, px2);
                }
            }
        }
    }

    void tex::drawRepeatHori(unsigned x, unsigned y, unsigned w)
    {
        if(data != NULL)
        {
            unsigned tY, tX;
            uint32_t *fb = (uint32_t *)gfxGetFramebuffer(NULL, NULL);

            uint32_t *dataPtr = &data[0];
            color dataClr, fbClr;
            for(tY = y; tY < y + height; tY++, dataPtr++)
            {
                uint32_t *rowPtr = &fb[tY * fbw + x];
                for(tX = x; tX < x + w; tX++, rowPtr++)
                {
                    dataClr.fromU32(*dataPtr);
                    fbClr.fromU32(*rowPtr);
                    *rowPtr = blend(dataClr, fbClr);
                }
            }
        }
    }

    void tex::drawRepeatHoriNoBlend(unsigned x, unsigned y, unsigned w)
    {
        if(data != NULL)
        {
            unsigned tY, tX;
            uint32_t *fb = (uint32_t *)gfxGetFramebuffer(NULL, NULL);

            uint32_t *dataPtr = &data[0];
            for(tY = y; tY < y + height; tY++, dataPtr++)
            {
                uint32_t *rowPtr = &fb[tY * fbw + x];
                for(tX = x; tX < x + w; tX++)
                {
                    *rowPtr++ = *dataPtr;
                }
            }
        }
    }

    void tex::drawRepeatVert(unsigned x, unsigned y, unsigned h)
    {
        if(data != NULL)
        {
            unsigned tY, tX;
            uint32_t *fb = (uint32_t *)gfxGetFramebuffer(NULL, NULL);

            uint32_t *dataPtr = &data[0];
            color dataClr, fbClr;
            for(tY = y; tY < y + h; tY++)
            {
                uint32_t *rowPtr = &fb[tY * fbw + x];
                dataPtr = &data[0];
                for(tX = x; tX < x + width; tX++, dataPtr++, rowPtr++)
                {
                    dataClr.fromU32(*dataPtr);
                    fbClr.fromU32(*rowPtr);
                    *rowPtr = blend(dataClr, fbClr);
                }
            }
        }
    }

    void tex::drawRepeatVertNoBlend(unsigned x, unsigned y, unsigned h)
    {
        if(data != NULL)
        {
            unsigned tY, tX;
            uint32_t *fb = (uint32_t *)gfxGetFramebuffer(NULL, NULL);

            uint32_t *dataPtr = &data[0];
            for(tY = y; tY < y + h; tY++)
            {
                uint32_t *rowPtr = &fb[tY * fbw + x];
                dataPtr = &data[0];
                for(tX = x; tX < x + width; tX++, dataPtr++)
                {
                    *rowPtr++ = *dataPtr;
                }
            }
        }
    }
}
