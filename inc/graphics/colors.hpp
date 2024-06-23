#pragma once

#define COLOR_DIM_BACKGROUND 0x00000088
#define COLOR_DEFAULT_CLEAR 0x2D2D2DFF
#define COLOR_TRANSPARENT 0x00000000
#define COLOR_DIALOG_BOX 0x505050FF
#define COLOR_WHITE 0xFFFFFFFF
#define COLOR_BLACK 0x000000FF
#define COLOR_DARK_GRAY 0xCACACAFF
#define COLOR_SLIDE_PANEL_TARGET 0x000000AA
#define COLOR_RED 0xFF0000FF
#define COLOR_GREEN 0x00FF00FF
#define COLOR_BLUE 0x0099EEFF
#define COLOR_YELLOW 0xF8FC00FF
#define COLOR_HEART 0xFF4444FF

static inline uint8_t getRed(const uint32_t &color)
{
    return color >> 24 & 0xFF;
}

static inline uint8_t getGreen(const uint32_t &color)
{
    return color >> 16 & 0xFF;
}

static inline uint8_t getBlue(const uint32_t &color)
{
    return color >> 8 & 0xFF;
}

static inline uint8_t getAlpha(const uint32_t &color)
{
    return color & 0xFF;
}

static inline uint32_t createColor(const uint8_t &red, const uint8_t &green, const uint8_t &blue, const uint8_t &alpha)
{
    return (red << 24 | green << 16 | blue << 8 | alpha);
}