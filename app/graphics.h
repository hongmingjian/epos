/**
 * vim: filetype=c:fenc=utf-8:ts=2:et:sw=2:sts=2
 */
#ifndef _GRAPHICS_H
#define _GRAPHICS_H

#include "../global.h"

typedef uint32_t COLORREF;

#define getRValue(c) ((uint8_t)(c))
#define getGValue(c) ((uint8_t)(((uint16_t)(c))>>8))
#define getBValue(c) ((uint8_t)((c)>>16))
#define RGB(r,g,b) ((COLORREF)((uint8_t)(r)|\
                              ((uint8_t)(g) << 8)|\
                              ((uint8_t)(b) << 16)))

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
        uint16_t XResolution;
        uint16_t YResolution;
        uint8_t  XCharSize;
        uint8_t  YCharSize;
        uint8_t  NumberOfPlanes;
        uint8_t  BitsPerPixel;
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
} __attribute__ ((packed));

extern struct ModeInfoBlock g_mib;

int listGraphicsModes();

int initGraphics(int mode);
int exitGraphics();

void setPixel(int x, int y, COLORREF cr);

#endif /*_GRAPHICS_H*/
