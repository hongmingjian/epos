/**
 * vim: filetype=c:fenc=utf-8:ts=4:et:sw=4:sts=4
 *
 * Copyright (C) 2015 Hong MingJian<hongmingjian@gmail.com>
 * All rights reserved.
 *
 * This file is part of the EPOS.
 *
 * Redistribution and use in source and binary forms are freely
 * permitted provided that the above copyright notice and this
 * paragraph and the following disclaimer are duplicated in all
 * such forms.
 *
 * This software is provided "AS IS" and without any express or
 * implied warranties, including, without limitation, the implied
 * warranties of merchantability and fitness for a particular
 * purpose.
 *
 */
#include <stddef.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include <syscall.h>
#include <stdio.h>
#include <string.h>
#include "graphics.h"

#define LOWORD(l) ((uint16_t)(l))
#define HIWORD(l) ((uint16_t)(((uint32_t)(l) >> 16) & 0xFFFF))
#define LOBYTE(w) ((uint8_t)(w))
#define HIBYTE(w) ((uint8_t)(((uint16_t)(w) >> 8) & 0xFF))

#define MAJOR(x) HIBYTE(x)
#define MINOR(x) LOBYTE(x)

#define LADDR(seg,off) ((uint32_t)(((uint16_t)(seg)<<4)+(uint16_t)(off)))

int vm86call(int fintr, uint32_t n, struct vm86_context *vm86ctx);

struct VBEInfoBlock {
    uint8_t VbeSignature[4];
    uint16_t VbeVersion;
    uint32_t OemStringPtr;
    uint32_t Capabilities;
    uint32_t VideoModePtr;
    uint16_t TotalMemory;

    // Added for VBE 2.0 and above
    uint16_t OemSoftwareRev;
    uint32_t OemVendorNamePtr;
    uint32_t OemProductNamePtr;
    uint32_t OemProductRevPtr;
    uint8_t reserved[222];
    uint8_t OemData[256];
} __attribute__ ((gcc_struct, packed));

struct ModeInfoBlock {
    uint16_t ModeAttributes;
    uint8_t  WinAAttributes;
    uint8_t  WinBAttributes;
    uint16_t WinGranularity;
    uint16_t WinSize;
    uint16_t WinASegment;
    uint16_t WinBSegment;
    uint32_t WinFuncPtr;
    uint16_t BytesPerScanLine;

    // Mandatory information for VBE 1.2 and above
    uint16_t XResolution; //水平分辨率
    uint16_t YResolution; //垂直分辨率
    uint8_t  XCharSize;
    uint8_t  YCharSize;
    uint8_t  NumberOfPlanes;
    uint8_t  BitsPerPixel; //像素位数
    uint8_t  NumberOfBanks;
    uint8_t  MemoryModel;
    uint8_t  BankSize;
    uint8_t  NumberOfImagePages;
    uint8_t  reserved1;

    uint8_t RedMaskSize;
    uint8_t RedFieldPosition;
    uint8_t GreenMaskSize;
    uint8_t GreenFieldPosition;
    uint8_t BlueMaskSize;
    uint8_t BlueFieldPosition;
    uint8_t RsvdMaskSize;
    uint8_t RsvdFieldPosition;
    uint8_t DirectColorModeInfo;

    // Mandatory information for VBE 2.0 and above
    uint32_t PhysBasePtr;
    uint32_t reserved2;
    uint16_t reserved3;

    // Mandatory information for VBE 3.0 and above
    uint16_t LinBytesPerScanLine;
    uint8_t  BnkNumberOfImagePages;
    uint8_t  LinNumberOfImagePages;
    uint8_t  LinRedMaskSize;
    uint8_t  LinRedFieldPosition;
    uint8_t  LinGreenMaskSize;
    uint8_t  LinGreenFieldPosition;
    uint8_t  LinBlueMaskSize;
    uint8_t  LinBlueFieldPosition;
    uint8_t  LinRsvdMaskSize;
    uint8_t  LinRsvdFieldPosition;
    uint32_t MaxPixelClock;

    uint8_t reserved4[190];
} __attribute__ ((gcc_struct, packed));

static struct VBEInfoBlock vib;
static struct ModeInfoBlock mib;
static int oldmode;

static int bankShift;
static int currBank = -1;

struct graphic_dev g_graphic_dev = {0};

static int getVBEInfo(struct VBEInfoBlock *pvib)
{
    char *VbeSignature;
    struct vm86_context vm86ctx = {.ss = 0x0000, .esp = 0x1000};
    vm86ctx.eax=0x4f00;

    /*
     * vm86call用了0x534处的一个字节，用0x536（而不是0x535）是为了两字节对齐
     */
    vm86ctx.es =0x0050;
    vm86ctx.edi=0x0036;

    VbeSignature = (char *)LADDR(vm86ctx.es, LOWORD(vm86ctx.edi));
    VbeSignature[0]='V'; VbeSignature[1]='B';
    VbeSignature[2]='E'; VbeSignature[3]='2';
    vm86call(1, 0x10, &vm86ctx);
    if(LOWORD(vm86ctx.eax) != 0x4f) {
        return -1;
    }

    *pvib = *(struct VBEInfoBlock *)LADDR(vm86ctx.es, LOWORD(vm86ctx.edi));

    return 0;
}

static int getModeInfo(int mode, struct ModeInfoBlock *pmib)
{
    struct vm86_context vm86ctx = {.ss = 0x0000, .esp = 0x1000};
    vm86ctx.eax=0x4f01;
    vm86ctx.ecx=mode;

    /*
     * getVBEInfo用了0x200字节
     */
    vm86ctx.es =0x0070;
    vm86ctx.edi=0x0036;

    vm86call(1, 0x10, &vm86ctx);
    if(LOWORD(vm86ctx.eax) != 0x4f)
        return -1;
    *pmib = *(struct ModeInfoBlock *)LADDR(vm86ctx.es, LOWORD(vm86ctx.edi));
    return 0;
}

static int getVBEMode()
{
    struct vm86_context vm86ctx = {.ss = 0x0000, .esp = 0x1000};
    vm86ctx.eax=0x4f03;
    vm86call(1, 0x10, &vm86ctx);
    if(LOWORD(vm86ctx.eax) != 0x4f)
        return -1;
    return vm86ctx.ebx & 0xffff;
}

static int setVBEMode(int mode)
{
    struct vm86_context vm86ctx = {.ss = 0x0000, .esp = 0x1000};
    vm86ctx.eax=0x4f02;
    vm86ctx.ebx=mode;
    vm86call(1, 0x10, &vm86ctx);
    if(LOWORD(vm86ctx.eax) != 0x4f)
        return -1;
    return 0;
}

static int switchBank(int bank)
{
    struct vm86_context vm86ctx = {.ss = 0x0000, .esp = 0x1000};

    if(bank == currBank)
        return 0;

    vm86ctx.ebx = 0x0;
    vm86ctx.edx = (bank << bankShift);

#if 0
    vm86ctx.eax=0x4f05;
    vm86call(1, 0x10, &vm86ctx);
    if(LOWORD(vm86ctx.eax) != 0x4f)
        return -1;
#else
    vm86call(0, mib.WinFuncPtr, &vm86ctx);
#endif

    currBank = bank;

    return 0;
}

int list_graphic_modes()
{
    uint16_t *modep;

    if(getVBEInfo(&vib)) {
        printf("No VESA BIOS EXTENSION(VBE) detected!\r\n");
        return -1;
    }

    printf("VESA VBE Version %d.%d detected (%s)\r\n",
            MAJOR(vib.VbeVersion),
            MINOR(vib.VbeVersion),
            (char *)(LADDR(HIWORD(vib.OemStringPtr), LOWORD(vib.OemStringPtr))));

    int i = 0;
    printf("\r\n");
    printf(" Mode   Resolution       Mode   Resolution   \r\n");
    printf("---------------------------------------------\r\n");
    for(modep = (uint16_t *)LADDR(HIWORD(vib.VideoModePtr),
                                  LOWORD(vib.VideoModePtr));
        *modep != 0xffff;
        modep++) {
        if(getModeInfo(*modep, &mib))
            continue;

        // Must be supported in hardware
        if(!(mib.ModeAttributes & 0x1))
            continue;

        // Must be graphics mode
        if(!(mib.ModeAttributes & 0x10))
            continue;

        printf("0x%04x %4dx%-4dx%2d",
                *modep,
                mib.XResolution,
                mib.YResolution,
                mib.BitsPerPixel);
        if(mib.NumberOfPlanes > 1)
            printf("x%d ", mib.NumberOfPlanes);
        else
            printf("   ");
        i++;
        if(i % 44 == 0) {
            printf("\r\nPress any key to continue...");
            getchar();
            printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
            printf("\b");
        }
        if(i % 2 == 0)
            printf("\r\n");
        else
            printf("| ");
    }
    if(i % 2)
        printf("\r\n");
    printf("---------------------------------------------\r\n");
    printf(" Mode   Resolution       Mode   Resolution   \r\n");

    printf("Press any key to continue...");
    getchar();
    getchar();
    printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
    return 0;
}

int init_graphic(int mode)
{
    if(getVBEInfo(&vib)) {
        printf("No VESA BIOS EXTENSION(VBE) detected!\r\n");
        return -1;
    }

    if(getModeInfo(mode, &mib)) {
        printf("Mode 0x%04x is not supported!\r\n", mode);
        return -1;
    }

    g_graphic_dev.XResolution = mib.XResolution;
    g_graphic_dev.YResolution = mib.YResolution;
    g_graphic_dev.BitsPerPixel = mib.BitsPerPixel;
    g_graphic_dev.NumberOfPlanes = mib.NumberOfPlanes;

    if((mib.ModeAttributes & 0x80) &&
       (mib.PhysBasePtr != 0)) {
        g_graphic_dev.BytesPerScanLine = (MAJOR(vib.VbeVersion) >= 3) ?
                                     mib.LinBytesPerScanLine :
                                     mib.BytesPerScanLine;
        g_graphic_dev.FrameBufferSize = g_graphic_dev.BytesPerScanLine*g_graphic_dev.YResolution;
        g_graphic_dev.Linear = 1;
        g_graphic_dev.pfnSwitchBank = NULL;
    } else {
        bankShift = 0;
        while((unsigned)(64 >> bankShift) != mib.WinGranularity)
            bankShift++;

        g_graphic_dev.BytesPerScanLine = mib.BytesPerScanLine;
        g_graphic_dev.FrameBufferSize = mib.WinSize*1024;
        g_graphic_dev.Linear = 0;
        g_graphic_dev.pfnSwitchBank = switchBank;
    }

    printf("VBE%d.%d: setting mode to 0x%04x(%dx%dx%d,%d planes)\r\n",
                MAJOR(vib.VbeVersion),
                MINOR(vib.VbeVersion),
                mode,
                g_graphic_dev.XResolution,
                g_graphic_dev.YResolution,
                g_graphic_dev.BitsPerPixel,
                g_graphic_dev.NumberOfPlanes);

    g_graphic_dev.FrameBuffer = mmap(NULL, g_graphic_dev.FrameBufferSize,
                                 PROT_READ|PROT_WRITE,
                                 MAP_PRIVATE|MAP_FILE,
                                 0x8000,
                                 g_graphic_dev.Linear?mib.PhysBasePtr:LADDR(mib.WinASegment, 0));
    if(g_graphic_dev.FrameBuffer == MAP_FAILED)
        return -1;

    printf("VBE%d.%d: On-board memory 0x%08x mapped to 0x%08x (%d pages,%s)\r\n",
                MAJOR(vib.VbeVersion),
                MINOR(vib.VbeVersion),
                g_graphic_dev.Linear?mib.PhysBasePtr:LADDR(mib.WinASegment, 0),
                g_graphic_dev.FrameBuffer,
                (g_graphic_dev.FrameBufferSize+sysconf(_SC_PAGESIZE)-1)/sysconf(_SC_PAGESIZE),
                g_graphic_dev.Linear?"linear":"banked");

    oldmode = getVBEMode();
    return setVBEMode(mode|(g_graphic_dev.Linear?0x4000:0));
}

int exit_graphic()
{
    setVBEMode(oldmode);
    munmap(g_graphic_dev.FrameBuffer, g_graphic_dev.FrameBufferSize);
    memset(&g_graphic_dev, 0, sizeof(g_graphic_dev));
    return 0;
}

/**
 * References:
 * [1] http://webpages.charter.net/danrollins/techhelp/0089.HTM
 * [2] http://www.brokenthorn.com/Resources/OSDevVid2.html
 *
 */
void setPixel(int x, int y, COLORREF cr)
{
    if(x < 0 || x >= g_graphic_dev.XResolution ||
       y < 0 || y >= g_graphic_dev.YResolution)
        return;

    uint8_t bpp = g_graphic_dev.BitsPerPixel;
    if(g_graphic_dev.BitsPerPixel == 15)
        bpp++;

    uint32_t addr = (y*g_graphic_dev.BytesPerScanLine) + (x*bpp)/8;
    uint8_t *p;

    switch(g_graphic_dev.BitsPerPixel) {
    case 1:
        break;

    case 2:
        if(y&1) {
            p = g_graphic_dev.FrameBuffer + 0x2000;
        } else {
            p = g_graphic_dev.FrameBuffer;
        }
        p += (y/2)*g_graphic_dev.BytesPerScanLine+(x*bpp)/8;

        *p &=    ~(3<<(6-2*(x&3)));
        *p |= (cr&3)<<(6-2*(x&3));

        break;

    case 4:
        break;

    case 8:
        if(g_graphic_dev.Linear) {
            p = g_graphic_dev.FrameBuffer+addr;
        } else {
            p = g_graphic_dev.FrameBuffer+LOWORD(addr);
            g_graphic_dev.pfnSwitchBank(HIWORD(addr));
        }
        *p = getRValue(cr);
        break;

    case 15:
        if(g_graphic_dev.Linear) {
            p = g_graphic_dev.FrameBuffer+addr;
        } else {
            p = g_graphic_dev.FrameBuffer+LOWORD(addr);
            g_graphic_dev.pfnSwitchBank(HIWORD(addr));
        }
        *((uint16_t *)p) = (uint16_t)(((getRValue(cr) & 0x1f)<<10) |
                                      ((getGValue(cr) & 0x1f)<<5) |
                                       (getBValue(cr) & 0x1f));
        break;

    case 16:
        if(g_graphic_dev.Linear) {
            p = g_graphic_dev.FrameBuffer+addr;
        } else {
            p = g_graphic_dev.FrameBuffer+LOWORD(addr);
            g_graphic_dev.pfnSwitchBank(HIWORD(addr));
        }
        *((uint16_t *)p) = (uint16_t)(((getRValue(cr) & 0x1f)<<11) |
                                      ((getGValue(cr) & 0x3f)<<5) |
                                       (getBValue(cr) & 0x1f));
        break;

    case 24:
        if(g_graphic_dev.Linear) {
            p = g_graphic_dev.FrameBuffer+addr;
            *p++ = getBValue(cr);
            *p++ = getGValue(cr);
            *p++ = getRValue(cr);
        } else {
            int bank = HIWORD(addr);
            p = g_graphic_dev.FrameBuffer+LOWORD(addr);
            g_graphic_dev.pfnSwitchBank(bank);
            *p++ = getBValue(cr);

            if(p-g_graphic_dev.FrameBuffer >= g_graphic_dev.FrameBufferSize) {
                p = g_graphic_dev.FrameBuffer;
                bank++;
                g_graphic_dev.pfnSwitchBank(bank);
            }
            *p++ = getGValue(cr);

            if(p-g_graphic_dev.FrameBuffer >= g_graphic_dev.FrameBufferSize) {
                p = g_graphic_dev.FrameBuffer;
                bank++;
                g_graphic_dev.pfnSwitchBank(bank);
            }
            *p++ = getRValue(cr);
        }
        break;

    case 32:
        if(g_graphic_dev.Linear) {
            p = g_graphic_dev.FrameBuffer+addr;
        } else {
            p = g_graphic_dev.FrameBuffer+LOWORD(addr);
            g_graphic_dev.pfnSwitchBank(HIWORD(addr));
        }
        *((uint32_t *)p) = (uint32_t)((getAValue(cr)<<24) |
                                      (getRValue(cr)<<16) |
                                      (getGValue(cr)<< 8) |
                                      (getBValue(cr)<< 0));
        break;
    }
}

/**
 * Copyright (c) 1993-1998 - Video Electronics Standards Association.
 * All rights reserved.
 *
 */
void line(int x1,int y1,int x2,int y2, COLORREF cr)
{
    int d; /* Decision variable */
    int dx,dy; /* Dx and Dy values for the line */
    int Eincr, NEincr;/* Decision variable increments */
    int yincr;/* Increment for y values */
    int t; /* Counters etc. */

#define ABS(a) ((a) >= 0 ? (a) : -(a))

    dx = ABS(x2 - x1);
    dy = ABS(y2 - y1);
    if (dy <= dx)
    {
        /* We have a line with a slope between -1 and 1
         *
         * Ensure that we are always scan converting the line from left to
         * right to ensure that we produce the same line from P1 to P0 as the
         * line from P0 to P1.
         */
        if (x2 < x1)
        {
            t=x2;x2=x1;x1=t; /*SwapXcoordinates */
            t=y2;y2=y1;y1=t; /*SwapYcoordinates */
        }
        if (y2 > y1)
            yincr = 1;
        else
            yincr = -1;
        d = 2*dy - dx;/* Initial decision variable value */
        Eincr = 2*dy;/* Increment to move to E pixel */
        NEincr = 2*(dy - dx);/* Increment to move to NE pixel */
        setPixel(x1,y1,cr); /* Draw the first point at (x1,y1) */

        /* Incrementally determine the positions of the remaining pixels */
        for (x1++; x1 <= x2; x1++)
        {
            if (d < 0)
                d += Eincr; /* Choose the Eastern Pixel */
            else
            {
                d += NEincr; /* Choose the North Eastern Pixel */
                y1 += yincr; /* (or SE pixel for dx/dy < 0!) */
            }
            setPixel(x1,y1,cr); /* Draw the point */
        }
    }
    else
    {
        /* We have a line with a slope between -1 and 1 (ie: includes
         * vertical lines). We must swap our x and y coordinates for this. *
         * Ensure that we are always scan converting the line from left to
         * right to ensure that we produce the same line from P1 to P0 as the
         * line from P0 to P1.
         */
        if (y2 < y1)
        {
            t=x2;x2=x1;x1=t; /*SwapXcoordinates */
            t=y2;y2=y1;y1=t; /*SwapYcoordinates */
        }
        if (x2 > x1)
            yincr = 1;
        else
            yincr = -1;
        d = 2*dx - dy;/* Initial decision variable value */
        Eincr = 2*dx;/* Increment to move to E pixel */
        NEincr = 2*(dx - dy);/* Increment to move to NE pixel */
        setPixel(x1,y1,cr); /* Draw the first point at (x1,y1) */

        /* Incrementally determine the positions of the remaining pixels */
        for (y1++; y1 <= y2; y1++)
        {
            if (d < 0)
                d += Eincr; /* Choose the Eastern Pixel */
            else
            {
                d += NEincr; /* Choose the North Eastern Pixel */
                x1 += yincr; /* (or SE pixel for dx/dy < 0!) */
            }
            setPixel(x1, y1, cr);
        }
    }
}

