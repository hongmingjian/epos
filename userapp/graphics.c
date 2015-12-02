/**
 * vim: filetype=c:fenc=utf-8:ts=4:et:sw=4:sts=4
 */

#include "graphics.h"

#define LOWORD(l) ((uint16_t)(l))
#define HIWORD(l) ((uint16_t)(((uint32_t)(l) >> 16) & 0xFFFF))
#define LOBYTE(w) ((uint8_t)(w))
#define HIBYTE(w) ((uint8_t)(((uint16_t)(w) >> 8) & 0xFF))

void setPixel(int x, int y, COLORREF cr)
{
    long addr = y * g_vga_dev.BytesPerScanLine + x*(g_vga_dev.BitsPerPixel/8);
    uint8_t *p;
    if(g_vga_dev.Linear) {
        p = g_vga_dev.FrameBuffer+addr;
        *p     = getBValue(cr);
        *(p+1) = getGValue(cr);
        *(p+2) = getRValue(cr);
    } else {
        int bank = HIWORD(addr);
        p = g_vga_dev.FrameBuffer+LOWORD(addr);
        g_vga_dev.pfnSwitchBank(bank);
        *p = getBValue(cr); p++;

        if(p-g_vga_dev.FrameBuffer >= g_vga_dev.FrameBufferSize) {
            p = g_vga_dev.FrameBuffer;
            bank++;
            g_vga_dev.pfnSwitchBank(bank);
        }
        *p = getGValue(cr); p++;

        if(p-g_vga_dev.FrameBuffer >= g_vga_dev.FrameBufferSize) {
            p = g_vga_dev.FrameBuffer;
            bank++;
            g_vga_dev.pfnSwitchBank(bank);
        }
        *p = getRValue(cr);
    }
}
