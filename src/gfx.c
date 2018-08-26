#include <switch.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <png.h>
#include <jpeglib.h>

#include "gfx.h"

tex *frameBuffer;

static inline uint32_t blend(const clr px, const clr fb)
{
    if(px.a == 0x00)
        return clrGetColor(fb);
    else if(px.a == 0xFF)
        return clrGetColor(px);

    uint8_t subAl = 0xFF - px.a;

    uint8_t fR = (px.r * px.a + fb.r * subAl) / 0xFF;
    uint8_t fG = (px.g * px.a + fb.g * subAl) / 0xFF;
    uint8_t fB = (px.b * px.a + fb.b * subAl) / 0xFF;

    return (0xFF << 24 | fB << 16 | fG << 8 | fR);
}

static inline uint32_t smooth(const clr px1, const clr px2)
{
    uint8_t fR = (px1.r + px2.r) / 2;
    uint8_t fG = (px1.g + px2.g) / 2;
    uint8_t fB = (px1.b + px2.b) / 2;
    uint8_t fA = (px1.a + px2.a) / 2;

    return (fA << 24 | fB << 16 | fG << 8 | fR);
}

bool graphicsInit(int windowWidth, int windowHeight)
{
    gfxInitResolution((uint32_t)windowWidth, (uint32_t)windowHeight);
    gfxInitDefault();
    plInitialize();
    consoleInit(NULL);

    gfxSetMode(GfxMode_LinearDouble);

    //Make a fake tex that points to framebuffer
    frameBuffer = malloc(sizeof(tex));
    frameBuffer->width = windowWidth;
    frameBuffer->height = windowHeight;
    frameBuffer->data = (uint32_t *)gfxGetFramebuffer(NULL, NULL);
    frameBuffer->size = windowWidth * windowHeight;

    return true;
}

bool graphicsExit()
{
    free(frameBuffer);

    plExit();
    gfxExit();

    return true;
}

void gfxHandleBuffs()
{
    gfxFlushBuffers();
    gfxSwapBuffers();
    gfxWaitForVsync();
}

static void drawGlyph(const FT_Bitmap *bmp, tex *target, int _x, int _y, const clr c)
{
    if(bmp->pixel_mode != FT_PIXEL_MODE_GRAY)
        return;

    uint8_t *bmpPtr = bmp->buffer;
    for(int y = _y; y < _y + bmp->rows; y++)
    {
        if(y > target->height || y < 0)
            continue;

        uint32_t *rowPtr = &target->data[y * target->width + _x];
        for(int x = _x; x < _x + bmp->width; x++, bmpPtr++, rowPtr++)
        {
            if(x > target->width || x < 0)
                continue;

            if(*bmpPtr > 0)
            {
                clr txClr = clrCreateRGBA(c.r, c.g, c.b, *bmpPtr);
                clr tgtClr = clrCreateU32(*rowPtr);

                *rowPtr = blend(txClr, tgtClr);
            }
        }
    }
}

static inline void resizeFont(const font *f, int sz)
{
    if(f->external)
        FT_Set_Char_Size(f->face[0], 0, sz * 64, 90, 90);
    else
    {
        for(int i = 0; i < 6; i++)
            FT_Set_Char_Size(f->face[i], 0, sz * 64, 90, 90);
    }
}

static inline FT_GlyphSlot loadGlyph(const uint32_t c, const font *f, FT_Int32 flags)
{
    if(f->external)
    {
        FT_Load_Glyph(f->face[0], FT_Get_Char_Index(f->face[0], c), flags);
        return f->face[0]->glyph;
    }
    for(int i = 0; i < 6; i++)
    {
        FT_UInt cInd = 0;
        if( (cInd = FT_Get_Char_Index(f->face[i], c)) != 0 && \
                FT_Load_Glyph(f->face[i], cInd, flags) == 0)
        {
            return f->face[i]->glyph;
        }
    }

    return NULL;
}

void drawText(const char *str, tex *target, const font *f, int x, int y, int sz, clr c)
{
    int tmpX = x;
    uint32_t tmpChr = 0;
    ssize_t unitCnt = 0;

    resizeFont(f, sz);

    size_t length = strlen(str);
    for(unsigned i = 0; i < length; )
    {
        unitCnt = decode_utf8(&tmpChr, (const uint8_t *)&str[i]);
        if(unitCnt <= 0)
            break;

        i += unitCnt;
        if(tmpChr == '\n')
        {
            tmpX = x;
            y += sz + 8;
            continue;
        }

        FT_GlyphSlot slot = loadGlyph(tmpChr, f, FT_LOAD_RENDER);
        if(slot != NULL)
        {
            int drawY = y + (sz - slot->bitmap_top);
            drawGlyph(&slot->bitmap, target, tmpX + slot->bitmap_left, drawY, c);

            tmpX += slot->advance.x >> 6;
        }
    }
}

void drawTextWrap(const char *str, tex *target, const font *f, int x, int y, int sz, clr c, int maxWidth)
{
    char wordBuf[128];
    size_t nextbreak = 0;
    size_t strLength = strlen(str);
    int tmpX = x;
    for(unsigned i = 0; i < strLength; )
    {
        nextbreak = strcspn(&str[i], " /");

        memset(wordBuf, 0, 128);
        memcpy(wordBuf, &str[i], nextbreak + 1);

        size_t width = textGetWidth(wordBuf, f, sz);

        if(tmpX + width >= x + maxWidth)
        {
            tmpX = x;
            y += sz + 8;
        }

        size_t wLength = strlen(wordBuf);
        uint32_t tmpChr = 0;
        for(unsigned j = 0; j < wLength; )
        {
            ssize_t unitCnt = decode_utf8(&tmpChr, (const uint8_t *)&wordBuf[j]);
            if(unitCnt <= 0)
                break;

            j += unitCnt;
            if(tmpChr == '\n')
            {
                tmpX = x;
                y += sz + 8;
                continue;
            }

            FT_GlyphSlot slot = loadGlyph(tmpChr, f, FT_LOAD_RENDER);
            if(slot != NULL)
            {
                int drawY = y + (sz - slot->bitmap_top);
                drawGlyph(&slot->bitmap, target, tmpX + slot->bitmap_left, drawY, c);

                tmpX += slot->advance.x >> 6;
            }
        }

        i += strlen(wordBuf);
    }
}

size_t textGetWidth(const char *str, const font *f, int sz)
{
    size_t width = 0;
    uint32_t untCnt = 0, tmpChr = 0;
    FT_Error ret = 0;

    resizeFont(f, sz);

    size_t length = strlen(str);
    for(unsigned i = 0; i < length; )
    {
        untCnt = decode_utf8(&tmpChr, (const uint8_t *)&str[i]);

        if(untCnt <= 0)
            break;

        i += untCnt;
        FT_GlyphSlot slot = loadGlyph(tmpChr, f, FT_LOAD_DEFAULT);
        if(ret)
            return 0;

        width += slot->advance.x >> 6;
    }

    return width;
}

void drawRect(tex *target, int x, int y, int w,  int h, const clr c)
{
    uint32_t clr = clrGetColor(c);

    for(int tY = y; tY < y + h; tY++)
    {
        uint32_t *rowPtr = &target->data[tY * target->width + x];
        for(int tX = x; tX < x + w; tX++, rowPtr++)
            *rowPtr = clr;
    }
}

void drawRectAlpha(tex *target, int x, int y, int w, int h, const clr c)
{
    for(int tY = y; tY < y + h; tY++)
    {
        uint32_t *rowPtr = &target->data[tY * target->width + x];
        for(int tX = x; tX < x + w; tX++, rowPtr++)
            *rowPtr = blend(c, clrCreateU32(*rowPtr));
    }
}

tex *texCreate(int w, int h)
{
    tex *ret = malloc(sizeof(tex));

    ret->width = w;
    ret->height = h;

    ret->data = (uint32_t *)malloc(w * h * sizeof(uint32_t));
    memset(ret->data, 0, w * h * sizeof(uint32_t));
    ret->size = ret->width * ret->height;

    return ret;
}

tex *texLoadPNGFile(const char *path)
{
    FILE *pngIn = fopen(path, "rb");
    if(pngIn != NULL)
    {
        png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        if(png == 0)
            return NULL;

        png_infop pngInfo = png_create_info_struct(png);
        if(pngInfo == 0)
            return NULL;

        int jmp = setjmp(png_jmpbuf(png));
        if(jmp)
            return NULL;

        png_init_io(png, pngIn);
        png_read_info(png, pngInfo);

        if(png_get_color_type(png, pngInfo) != PNG_COLOR_TYPE_RGBA)
        {
            png_destroy_read_struct(&png, &pngInfo, NULL);
            return NULL;
        }

        tex *ret = malloc(sizeof(tex));
        ret->width = png_get_image_width(png, pngInfo);
        ret->height = png_get_image_height(png, pngInfo);

        ret->data = (uint32_t *)malloc((ret->width * ret->height) * sizeof(uint32_t));
        ret->size = ret->width * ret->height;

        png_bytep *rows = malloc(sizeof(png_bytep) * ret->height);
        for(int i = 0; i < ret->height; i++)
            rows[i] = malloc(png_get_rowbytes(png, pngInfo));

        png_read_image(png, rows);

        uint32_t *dataPtr = &ret->data[0];
        for(int y = 0; y < ret->height; y++)
        {
            uint32_t *rowPtr = (uint32_t *)rows[y];
            for(int x = 0; x < ret->width; x++)
                *dataPtr++ = *rowPtr++;
        }

        for(int i = 0; i < ret->height; i++)
            free(rows[i]);

        free(rows);

        png_destroy_read_struct(&png, &pngInfo, NULL);
        fclose(pngIn);

        return ret;
    }
    return NULL;
}

tex *texLoadJPEGFile(const char *path)
{
    FILE *jpegIn = fopen(path, "rb");
    if(jpegIn != NULL)
    {
        struct jpeg_decompress_struct jpegInfo;
        struct jpeg_error_mgr error;

        jpegInfo.err = jpeg_std_error(&error);

        jpeg_create_decompress(&jpegInfo);
        jpeg_stdio_src(&jpegInfo, jpegIn);
        jpeg_read_header(&jpegInfo, true);

        if(jpegInfo.jpeg_color_space == JCS_YCbCr)
            jpegInfo.out_color_space = JCS_RGB;

        tex *ret = malloc(sizeof(tex));

        ret->width = jpegInfo.image_width;
        ret->height = jpegInfo.image_height;

        ret->data = (uint32_t *)malloc((ret->width * ret->height) * sizeof(uint32_t));
        ret->size = ret->width * ret->height;

        jpeg_start_decompress(&jpegInfo);

        JSAMPARRAY row = malloc(sizeof(JSAMPROW));
        row[0] = malloc(sizeof(JSAMPLE) * ret->width * 3);

        uint32_t *dataPtr = &ret->data[0];
        for(int y = 0; y < ret->height; y++)
        {
            jpeg_read_scanlines(&jpegInfo, row, 1);
            uint8_t *jpegPtr = row[0];
            for(int x = 0; x < ret->width; x++, jpegPtr += 3)
                *dataPtr++ = (0xFF << 24 | jpegPtr[2] << 16 | jpegPtr[1] << 8 | jpegPtr[0]);
        }

        jpeg_finish_decompress(&jpegInfo);
        jpeg_destroy_decompress(&jpegInfo);

        free(row[0]);
        free(row);

        fclose(jpegIn);

        return ret;
    }
    return NULL;
}

tex *texLoadJPEGMem(const uint8_t *jpegData, size_t jpegSize)
{
    struct jpeg_decompress_struct jpegInfo;
    struct jpeg_error_mgr error;

    jpegInfo.err = jpeg_std_error(&error);

    jpeg_create_decompress(&jpegInfo);
    jpeg_mem_src(&jpegInfo, jpegData, jpegSize);
    jpeg_read_header(&jpegInfo, true);

    if(jpegInfo.jpeg_color_space == JCS_YCbCr)
        jpegInfo.out_color_space = JCS_RGB;

    tex *ret = malloc(sizeof(tex));
    ret->width = jpegInfo.image_width;
    ret->height = jpegInfo.image_height;

    ret->data = (uint32_t *)malloc((ret->width * ret->height) * sizeof(uint32_t));
    ret->size = ret->width * ret->height;

    jpeg_start_decompress(&jpegInfo);

    JSAMPARRAY row = malloc(sizeof(JSAMPARRAY));
    row[0] = malloc(sizeof(JSAMPLE) * ret->width * 3);

    uint32_t *dataPtr = &ret->data[0];
    for(int y = 0; y < ret->height; y++)
    {
        jpeg_read_scanlines(&jpegInfo, row, 1);
        uint8_t *jpegPtr = row[0];
        for(int x = 0; x < ret->width; x++, jpegPtr += 3)
            *dataPtr++ = (0xFF << 24 | jpegPtr[2] << 16 | jpegPtr[1] << 8 | jpegPtr[0]);
    }

    jpeg_finish_decompress(&jpegInfo);
    jpeg_destroy_decompress(&jpegInfo);

    free(row[0]);
    free(row);

    return ret;
}

void texDestroy(tex *t)
{
    if(t->data != NULL)
        free(t->data);

    if(t != NULL)
        free(t);
}

void texClearColor(tex *in, const clr c)
{
    uint32_t *dataPtr = &in->data[0];
    uint32_t color = clrGetColor(c);
    for(int i = 0; i < in->size; i++)
        *dataPtr++ = color;
}

void texDraw(const tex *t, tex *target, int x, int y)
{
    if(t != NULL)
    {
        uint32_t *dataPtr = &t->data[0];
        for(int tY = y; tY < y + t->height; tY++)
        {
            uint32_t *rowPtr = &target->data[tY * target->width + x];
            for(int tX = x; tX < x + t->width; tX++, rowPtr++)
            {
                clr dataClr = clrCreateU32(*dataPtr++);
                clr fbClr   = clrCreateU32(*rowPtr);

                *rowPtr = blend(dataClr, fbClr);
            }
        }
    }
}

void texDrawNoAlpha(const tex *t, tex *target, int x, int y)
{
    if(t != NULL)
    {
        uint32_t *dataPtr = &t->data[0];
        for(int tY = y; tY < y + t->height; tY++)
        {
            uint32_t *rowPtr = &target->data[tY * target->width + x];
            for(int tX = x; tX < x + t->width; tX++)
                *rowPtr++ = *dataPtr++;
        }
    }
}

void texDrawSkip(const tex *t, tex *target, int x, int y)
{
    if(t != NULL)
    {
        uint32_t *dataPtr = &t->data[0];
        for(int tY = y; tY < y + (t->height / 2); tY++, dataPtr += t->width)
        {
            uint32_t *rowPtr = &target->data[tY * target->width + x];
            for(int tX = x; tX < x + (t->width / 2); tX++, rowPtr++)
            {
                clr px1 = clrCreateU32(*dataPtr++);
                clr px2 = clrCreateU32(*dataPtr++);
                clr fbPx = clrCreateU32(*rowPtr);

                *rowPtr = blend(clrCreateU32(smooth(px1, px2)), fbPx);
            }
        }
    }
}

void texDrawSkipNoAlpha(const tex *t, tex *target, int x, int y)
{
    if(t != NULL)
    {
        uint32_t *dataPtr = &t->data[0];
        for(int tY = y; tY < y + (t->height / 2); tY++, dataPtr += t->width)
        {
            uint32_t *rowPtr = &target->data[tY * target->width + x];
            for(int tX = x; tX < x + (t->width / 2); tX++, rowPtr++)
            {
                clr px1 = clrCreateU32(*dataPtr++);
                clr px2 = clrCreateU32(*dataPtr++);

                *rowPtr = smooth(px1, px2);
            }
        }
    }
}

void texDrawInvert(const tex *t, tex *target, int x, int y)
{
    if(t != NULL)
    {
        uint32_t *dataPtr = &t->data[0];
        for(int tY = y; tY < y + t->height; tY++)
        {
            uint32_t *rowPtr = &target->data[tY * target->width + x];
            for(int tX = x; tX < x + t->width; tX++, rowPtr++)
            {
                clr dataClr = clrCreateU32(*dataPtr++);
                clrInvert(&dataClr);
                clr fbClr = clrCreateU32(*rowPtr);

                *rowPtr = blend(dataClr, fbClr);
            }
        }
    }
}

void texSwapColors(tex *t, const clr old, const clr newColor)
{
    uint32_t oldClr = clrGetColor(old), newClr = clrGetColor(newColor);

    uint32_t *dataPtr = &t->data[0];
    for(unsigned i = 0; i < t->size; i++, dataPtr++)
    {
        if(*dataPtr == oldClr)
            *dataPtr = newClr;
    }

}

tex *texCreateFromPart(const tex *src, int x, int y, int w, int h)
{
    tex *ret = texCreate(w, h);

    uint32_t *retPtr = &ret->data[0];
    for(int tY = y; tY < y + h; tY++)
    {
        uint32_t *srcPtr = &src->data[tY * src->width + x];
        for(int tX = x; tX < x + w; tX++)
            *retPtr++ = *srcPtr++;
    }

    return ret;
}

void texScaleToTex(const tex *in, tex *out, int scale)
{
    for(int y = 0; y < in->height; y++)
    {
        for(int tY = y * scale; tY < (y * scale) + scale; tY++)
        {
            uint32_t *inPtr = &in->data[y * in->width];
            for(int x = 0; x < in->width; x++, inPtr++)
            {
                for(int tX = x * scale; tX < (x * scale) + scale; tX++)
                {
                    out->data[tY * (in->width * scale) + tX] = *inPtr;
                }
            }
        }
    }
}

font *fontLoadSharedFonts()
{
    font *ret = malloc(sizeof(font));
    if((ret->libRet = FT_Init_FreeType(&ret->lib)))
    {
        free(ret);
        return NULL;
    }


    for(int i = 0; i < 6; i++)
    {
        PlFontData plFont;
        if(R_FAILED(plGetSharedFontByType(&plFont, i)))
        {
            free(ret);
            return NULL;
        }

        if((ret->faceRet = FT_New_Memory_Face(ret->lib, plFont.address, plFont.size, 0, &ret->face[i])))
        {
            free(ret);
            return NULL;
        }
    }

    ret->external = false;
    ret->fntData = NULL;

    return ret;
}

font *fontLoadTTF(const char *path)
{
    font *ret = malloc(sizeof(font));
    if((ret->libRet = FT_Init_FreeType(&ret->lib)))
    {
        free(ret);
        return NULL;
    }

    FILE *ttf = fopen(path, "rb");
    fseek(ttf, 0, SEEK_END);
    size_t ttfSize = ftell(ttf);
    fseek(ttf, 0, SEEK_SET);

    ret->fntData = malloc(ttfSize);
    fread(ret->fntData, 1, ttfSize, ttf);
    fclose(ttf);

    if((ret->faceRet = FT_New_Memory_Face(ret->lib, ret->fntData, ttfSize, 0, &ret->face[0])))
    {
        free(ret->fntData);
        free(ret);
        return NULL;
    }

    ret->external = true;

    return ret;
}

void fontDestroy(font *f)
{
    if(f->external && f->faceRet == 0)
        FT_Done_Face(f->face[0]);
    else if(!f->external && f->faceRet == 0)
    {
        for(int i = 0; i < 6; i++)
            FT_Done_Face(f->face[i]);
    }
    if(f->libRet == 0)
        FT_Done_FreeType(f->lib);
    if(f->fntData != NULL)
        free(f->fntData);

    free(f);
}
