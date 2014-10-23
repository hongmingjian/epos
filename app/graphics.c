/**
 * vim: filetype=c:fenc=utf-8:ts=2:et:sw=2:sts=2
 */
#include "../global.h"
#include "syscall.h"
#include "math.h"
#include "graphics.h"

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
} __attribute__ ((packed));

#define LADDR(seg,off) ((uint32_t)(((uint16_t)(seg)<<4)+(uint16_t)(off)))
int vm86int(int n, struct vm86_context *vm86ctx);

static struct VBEInfoBlock g_vib;
static int g_bankShift;
static int g_oldmode;
       struct ModeInfoBlock g_mib;

static int getVBEInfo(struct VBEInfoBlock *pvib)
{
  struct vm86_context vm86ctx = {.ss = 0x9000, .esp = 0x0000};
  vm86ctx.eax=0x4f00;
  vm86ctx.es=0x1000;
  vm86ctx.edi=0x0000;
  vm86int(0x10, &vm86ctx);
  if(LOWORD(vm86ctx.eax) != 0x4f)
    return -1;

  *pvib = *(struct VBEInfoBlock *)LADDR(0x1000, 0x0000);
  return 0;
}

static int getModeInfo(int mode, struct ModeInfoBlock *pmib)
{
  struct vm86_context vm86ctx = {.ss = 0x9000, .esp = 0x0000};
  vm86ctx.eax=0x4f01;
  vm86ctx.ecx=mode;
  vm86ctx.es =0x4000;
  vm86ctx.edi=0x0000;
  vm86int(0x10, &vm86ctx);
  if(LOWORD(vm86ctx.eax) != 0x4f)
    return -1;
  *pmib = *(struct ModeInfoBlock *)LADDR(0x4000, 0x0000);
  return 0;
}

static int getVBEMode()
{
  struct vm86_context vm86ctx = {.ss = 0x9000, .esp = 0x0000};
  vm86ctx.eax=0x4f03;
  vm86int(0x10, &vm86ctx);
  return vm86ctx.ebx & 0xffff;
}

static int setVBEMode(int mode)
{
  struct vm86_context vm86ctx = {.ss = 0x9000, .esp = 0x0000};
  vm86ctx.eax=0x4f02;
  vm86ctx.ebx=mode;
  vm86int(0x10, &vm86ctx);
  if(LOWORD(vm86ctx.eax) != 0x4f)
    return -1;
  return 0;
}

static int switchBank(int bank)
{
  int curBank;
  struct vm86_context vm86ctx = {.ss = 0x9000, .esp = 0x0000};

  vm86ctx.eax=0x4f05;
  vm86ctx.ebx = 0x0100;
  vm86int(0x10, &vm86ctx);
  if(LOWORD(vm86ctx.eax) != 0x4f)
    return -1;

  curBank = LOWORD(vm86ctx.edx);
  bank <<= g_bankShift;

  if(bank == curBank)
    return 0;

  vm86ctx.eax=0x4f05;
  vm86ctx.ebx = 0x0;
  vm86ctx.edx = bank;
  vm86int(0x10, &vm86ctx);
  if(LOWORD(vm86ctx.eax) != 0x4f)
    return -1;
  return 0;
}

int listGraphicsModes()
{
  uint16_t *vmp;
  struct VBEInfoBlock *pvib;
  struct ModeInfoBlock *pmib;
  struct vm86_context vm86ctx = {.ss = 0x9000, .esp = 0x0000};

  vm86ctx.eax=0x4f00;
  vm86ctx.es=0x1000;
  vm86ctx.edi=0x0000;
  vm86int(0x10, &vm86ctx);
  if(LOWORD(vm86ctx.eax) != 0x4f)
    return -1;
  pvib = (struct VBEInfoBlock *)LADDR(0x1000, 0x0000);

  printf(" Mode  Resolution\r\n");
  for(vmp = (uint16_t *)LADDR(HIWORD(pvib->VideoModePtr),
		              LOWORD(pvib->VideoModePtr));
      *vmp != 0xffff;
      vmp++) {
    vm86ctx.eax=0x4f01;
    vm86ctx.ecx=*vmp;
    vm86ctx.es =0x4000;
    vm86ctx.edi=0x0000;
    vm86int(0x10, &vm86ctx);
    if(LOWORD(vm86ctx.eax) != 0x4f)
      continue;
    pmib = (struct ModeInfoBlock *)LADDR(0x4000, 0x0000);

    if(!(pmib->ModeAttributes & 0x01))
      continue;
    if(!(pmib->ModeAttributes & 0x10))
      continue;
    if(pmib->BitsPerPixel != 24)
      continue;
    if(pmib->NumberOfPlanes != 1)
      continue;

    printf("0x%04x %4dx%4dx%2d\r\n",
           *vmp,
	   pmib->XResolution,
	   pmib->YResolution,
	   pmib->BitsPerPixel,
	   pmib->BytesPerScanLine
	   );
  }
  printf(" Mode  Resolution\r\n");
  return 0;
}

void putPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b)
{
  long addr = y * g_mib.BytesPerScanLine + x*(g_mib.BitsPerPixel/8);
  uint8_t *p = (uint8_t *)LADDR(g_mib.WinASegment, LOWORD(addr));
  switchBank(HIWORD(addr));
  *p = b; *(p+1)=g; *(p+2)=r;
}

int initGraphics(int mode)
{
  if(getVBEInfo(&g_vib)) {
    printf("No VESA BIOS EXTENSION(VBE) detected!\r\n");
    return -1;
  }

  if(getModeInfo(mode, &g_mib)) {
    printf("VESA mode 0x%x is not supported!\r\n");
    return -1;
  }

  g_bankShift = 0;
  while((unsigned)(64 >> g_bankShift) != g_mib.WinGranularity)
    g_bankShift++;

  g_oldmode = getVBEMode();
  return setVBEMode(mode);
}

int exitGraphics()
{
  return setVBEMode(g_oldmode);
}

/*
 * The codes of `line' come from
 *
 *         VESA BIOS EXTENSION(VBE)
 *                Core Functions
 *                  Standard
 *
 *                  Version: 3.0
 */
void line(int x1,int y1,int x2,int y2, uint8_t r, uint8_t g, uint8_t b)
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
        putPixel(x1,y1,r,g,b); /* Draw the first point at (x1,y1) */

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
            putPixel(x1,y1,r,g,b); /* Draw the point */
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
        putPixel(x1,y1,r,g,b); /* Draw the first point at (x1,y1) */

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
            putPixel(x1, y1, r, g, b);
        }
    }
}

