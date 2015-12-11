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
#include <inttypes.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include <syscall.h>
#include "vga.h"

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

struct vga_dev g_vga_dev = {0};

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

int list_vga_modes()
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

int init_vga(int mode)
{
    if(getVBEInfo(&vib)) {
        printf("No VESA BIOS EXTENSION(VBE) detected!\r\n");
        return -1;
    }

    if(getModeInfo(mode, &mib)) {
        printf("Mode 0x%04x is not supported!\r\n", mode);
        return -1;
    }

    g_vga_dev.XResolution = mib.XResolution;
    g_vga_dev.YResolution = mib.YResolution;
    g_vga_dev.BitsPerPixel = mib.BitsPerPixel;
    g_vga_dev.NumberOfPlanes = mib.NumberOfPlanes;

    if((mib.ModeAttributes & 0x80) &&
       (mib.PhysBasePtr != 0)) {
        g_vga_dev.BytesPerScanLine = (MAJOR(vib.VbeVersion) >= 3) ?
                                     mib.LinBytesPerScanLine :
                                     mib.BytesPerScanLine;
        g_vga_dev.FrameBufferSize = g_vga_dev.BytesPerScanLine*g_vga_dev.YResolution;
        g_vga_dev.Linear = 1;
        g_vga_dev.pfnSwitchBank = NULL;
    } else {
        bankShift = 0;
        while((unsigned)(64 >> bankShift) != mib.WinGranularity)
            bankShift++;

        g_vga_dev.BytesPerScanLine = mib.BytesPerScanLine;
        g_vga_dev.FrameBufferSize = mib.WinSize*1024;
        g_vga_dev.Linear = 0;
        g_vga_dev.pfnSwitchBank = switchBank;
    }

    printf("VBE%d.%d: setting mode to 0x%04x(%dx%dx%d,%d planes)\r\n",
                MAJOR(vib.VbeVersion),
                MINOR(vib.VbeVersion),
                mode,
                g_vga_dev.XResolution,
                g_vga_dev.YResolution,
                g_vga_dev.BitsPerPixel,
                g_vga_dev.NumberOfPlanes);

    g_vga_dev.FrameBuffer = mmap(NULL, g_vga_dev.FrameBufferSize,
                                 PROT_READ|PROT_WRITE,
                                 MAP_PRIVATE|MAP_FILE,
                                 0x8000,
                                 g_vga_dev.Linear?mib.PhysBasePtr:LADDR(mib.WinASegment, 0));
    if(g_vga_dev.FrameBuffer == MAP_FAILED)
        return -1;

    printf("VBE%d.%d: On-board memory 0x%08x mapped to 0x%08x (%d pages,%s)\r\n",
                MAJOR(vib.VbeVersion),
                MINOR(vib.VbeVersion),
                g_vga_dev.Linear?mib.PhysBasePtr:LADDR(mib.WinASegment, 0),
                g_vga_dev.FrameBuffer,
                (g_vga_dev.FrameBufferSize+sysconf(_SC_PAGESIZE)-1)/sysconf(_SC_PAGESIZE),
                g_vga_dev.Linear?"linear":"banked");

    oldmode = getVBEMode();
    return setVBEMode(mode|(g_vga_dev.Linear?0x4000:0));
}

int exit_vga()
{
    setVBEMode(oldmode);
    munmap(g_vga_dev.FrameBuffer, g_vga_dev.FrameBufferSize);
}
