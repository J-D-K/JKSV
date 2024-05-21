#pragma once

#define getRed(r) r >> 24 & 0xFF
#define getGreen(g) g >> 16 & 0xFF
#define getBlue(b) b >> 8 & 0xFF
#define getAlpha(a) a & 0xFF
#define createColor(r, g, b, a) (r << 24 | g << 16 | b << 8 | a)

#define COLOR_DIM_BACKGROUND 0x00000088
#define COLOR_DEFAULT_CLEAR 0x2D2D2DFF
#define COLOR_TRANSPARENT 0x00000000
#define COLOR_DIALOG_BOX 0x505050FF
#define COLOR_WHITE 0xFFFFFFFF
#define COLOR_BLACK 0x000000FF
#define COLOR_SLIDE_PANEL_TARGET 0x000000AA
#define COLOR_RED 0xFF0000FF
#define COLOR_GREEN 0x00FF00FF
#define COLOR_BLUE 0x0099EEFF
#define COLOR_YELLOW 0xF8FC00FF
#define COLOR_HEART 0xFF4444FF