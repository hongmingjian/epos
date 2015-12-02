/**
 * vim: filetype=c:fenc=utf-8:ts=4:et:sw=4:sts=4
 */
#ifndef _GRAPHICS_H
#define _GRAPHICS_H

#include "vga.h"

typedef uint32_t COLORREF;
#define getRValue(c) ((uint8_t)(c))
#define getGValue(c) ((uint8_t)(((uint16_t)(c))>>8))
#define getBValue(c) ((uint8_t)((c)>>16))
#define RGB(r,g,b) ((COLORREF)((uint8_t)(r)|\
                               ((uint8_t)(g) << 8)|\
                               ((uint8_t)(b) << 16)))

/**
 * 进入图形模式后，用于在屏幕位置(x,y)以颜色cr打点
 */
void setPixel(int x, int y, COLORREF cr);

#endif /*_GRAPHICS_H*/
