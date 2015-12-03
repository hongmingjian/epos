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
    if(x >= g_vga_dev.XResolution ||
       y >= g_vga_dev.YResolution)
        return;

    uint8_t bpp = g_vga_dev.BitsPerPixel;
    if(g_vga_dev.BitsPerPixel == 15)
        bpp++;

    uint32_t addr = (y*g_vga_dev.BytesPerScanLine) + (x*bpp)/8;
    uint8_t *p;

    switch(g_vga_dev.BitsPerPixel) {
    case 1:
        if(g_vga_dev.Linear) {
            p = g_vga_dev.FrameBuffer+addr;
        } else {
            p = g_vga_dev.FrameBuffer+LOWORD(addr);
            g_vga_dev.pfnSwitchBank(HIWORD(addr));
        }

        *p &=    ~(1<<(7-(x&7)));
        *p |= (cr&1)<<(7-(x&7));

        break;

    case 2:
        if(g_vga_dev.Linear) {
            p = g_vga_dev.FrameBuffer+addr;
        } else {
            p = g_vga_dev.FrameBuffer+LOWORD(addr);
            g_vga_dev.pfnSwitchBank(HIWORD(addr));
        }

        *p &=    ~(3<<(6-2*(x&3)));
        *p |= (cr&3)<<(6-2*(x&3));

        break;

    case 4:
        if(g_vga_dev.Linear) {
            p = g_vga_dev.FrameBuffer+addr;
        } else {
            p = g_vga_dev.FrameBuffer+LOWORD(addr);
            g_vga_dev.pfnSwitchBank(HIWORD(addr));
        }

        *p &=    ~(0xf<<(4-4*(x&1)));
        *p |= (cr&0xf)<<(4-4*(x&1));

        break;

    case 8:
        if(g_vga_dev.Linear) {
            p = g_vga_dev.FrameBuffer+addr;
        } else {
            p = g_vga_dev.FrameBuffer+LOWORD(addr);
            g_vga_dev.pfnSwitchBank(HIWORD(addr));
        }
        *p = getRValue(cr);
        break;

    case 15:
        if(g_vga_dev.Linear) {
            p = g_vga_dev.FrameBuffer+addr;
        } else {
            p = g_vga_dev.FrameBuffer+LOWORD(addr);
            g_vga_dev.pfnSwitchBank(HIWORD(addr));
        }
        *((uint16_t *)p) = (uint16_t)(((getRValue(cr) & 0x1f)<<10) |
                                      ((getGValue(cr) & 0x1f)<<5) |
                                       (getBValue(cr) & 0x1f));
        break;

    case 16:
        if(g_vga_dev.Linear) {
            p = g_vga_dev.FrameBuffer+addr;
        } else {
            p = g_vga_dev.FrameBuffer+LOWORD(addr);
            g_vga_dev.pfnSwitchBank(HIWORD(addr));
        }
        *((uint16_t *)p) = (uint16_t)(((getRValue(cr) & 0x1f)<<11) |
                                      ((getGValue(cr) & 0x3f)<<5) |
                                       (getBValue(cr) & 0x1f));
        break;

    case 24:
        if(g_vga_dev.Linear) {
            p = g_vga_dev.FrameBuffer+addr;
            *p++ = getBValue(cr);
            *p++ = getGValue(cr);
            *p++ = getRValue(cr);
        } else {
            int bank = HIWORD(addr);
            p = g_vga_dev.FrameBuffer+LOWORD(addr);
            g_vga_dev.pfnSwitchBank(bank);
            *p++ = getBValue(cr);

            if(p-g_vga_dev.FrameBuffer >= g_vga_dev.FrameBufferSize) {
                p = g_vga_dev.FrameBuffer;
                bank++;
                g_vga_dev.pfnSwitchBank(bank);
            }
            *p++ = getGValue(cr);

            if(p-g_vga_dev.FrameBuffer >= g_vga_dev.FrameBufferSize) {
                p = g_vga_dev.FrameBuffer;
                bank++;
                g_vga_dev.pfnSwitchBank(bank);
            }
            *p++ = getRValue(cr);
        }
        break;

    case 32:
        if(g_vga_dev.Linear) {
            p = g_vga_dev.FrameBuffer+addr;
        } else {
            p = g_vga_dev.FrameBuffer+LOWORD(addr);
            g_vga_dev.pfnSwitchBank(HIWORD(addr));
        }
        *((uint32_t *)p) = (uint32_t)((getAValue(cr)<<24) |
                                      (getRValue(cr)<<16) |
                                      (getGValue(cr)<< 8) |
                                      (getBValue(cr)<< 0));
        break;
    }
}
