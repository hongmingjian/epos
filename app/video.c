#include "../global.h"
#include "syscall.h"

#define LADDR(seg,off) ((uint32_t)(((uint16_t)(seg)<<4)+(uint16_t)(off)))
int vm86int(int n, struct vm86_context *vm86ctx);

struct VBEInfoBlock {
	uint8_t VESASignature[4];
	uint16_t VESAVersion;
	uint32_t OEMStringPtr;
	uint32_t Capabilities;
	uint32_t VideoModePtr;
	uint16_t TotalMemory;
	uint8_t reserved[236];
} __attribute__ ((packed));

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

static struct vm86_context g_vm86ctx = {.ss = 0x9000, .esp = 0x0000};
static struct VBEInfoBlock g_vib;
static struct ModeInfoBlock g_mib;
static int g_curBank = -1;
static int g_oldmode;

static int getVBEInfo(struct VBEInfoBlock *pvib)
{
  g_vm86ctx.eax=0x4f00;
  g_vm86ctx.es=0x1000;
  g_vm86ctx.edi=0x0000;
  vm86int(0x10, &g_vm86ctx);
  if(LOWORD(g_vm86ctx.eax) != 0x4f)
    return -1;

  *pvib = *(struct VBEInfoBlock *)LADDR(0x1000, 0x0000);
  return 0;
}

static int getModeInfo(int mode, struct ModeInfoBlock *pmib)
{
  g_vm86ctx.eax=0x4f01;
  g_vm86ctx.ecx=mode;
  g_vm86ctx.es =0x4000;
  g_vm86ctx.edi=0x0000;
  vm86int(0x10, &g_vm86ctx);
  if(LOWORD(g_vm86ctx.eax) != 0x4f)
    return -1;
  *pmib = *(struct ModeInfoBlock *)LADDR(0x4000, 0x0000);
  return 0;
}

int listGraphicsModes()
{
  uint16_t *vmp;
  struct VBEInfoBlock *pvib;
  struct ModeInfoBlock *pmib;

  g_vm86ctx.eax=0x4f00;
  g_vm86ctx.es=0x1000;
  g_vm86ctx.edi=0x0000;
  vm86int(0x10, &g_vm86ctx);
  if(LOWORD(g_vm86ctx.eax) != 0x4f)
    return -1;
  pvib = (struct VBEInfoBlock *)LADDR(0x1000, 0x0000);

  printf(" Mode   Resolution  BytesPerScanLine WinASegment NumberOfBanks\r\n");
  for(vmp = (uint16_t *)LADDR(HIWORD(pvib->VideoModePtr), 
		              LOWORD(pvib->VideoModePtr));
      *vmp != 0xffff; 
      vmp++) {
    g_vm86ctx.eax=0x4f01;
    g_vm86ctx.ecx=*vmp;
    g_vm86ctx.es =0x4000;
    g_vm86ctx.edi=0x0000;
    vm86int(0x10, &g_vm86ctx);
    if(LOWORD(g_vm86ctx.eax) != 0x4f)
      continue;
    pmib = (struct ModeInfoBlock *)LADDR(0x4000, 0x0000);

    if(!(pmib->ModeAttributes & 0x10)) {
      continue;
    }

    printf("0x%04x %4dx%4dx%2d %16d 0x%04x %12d\r\n", 
           *vmp, 
	   pmib->XResolution, 
	   pmib->YResolution, 
	   pmib->BitsPerPixel, 
	   pmib->BytesPerScanLine, 
           pmib->WinASegment,
	   pmib->NumberOfBanks
	   );
  }
  printf(" Mode   Resolution  BytesPerScanLine WinASegment NumberOfBanks\r\n");
  return 0;
}

static int getVBEMode()
{
  g_vm86ctx.eax=0x4f03;
  vm86int(0x10, &g_vm86ctx);
  return g_vm86ctx.ebx & 0xffff;
}

static int setVBEMode(int mode)
{
  g_vm86ctx.eax=0x4f02;
  g_vm86ctx.ebx=mode;
  vm86int(0x10, &g_vm86ctx);
  if(LOWORD(g_vm86ctx.eax) != 0x4f)
    return -1;
  return 0; 
}

static void setBank(int bank)
{
  int bankShift;

  if(bank == g_curBank)
    return;
  g_curBank = bank;

  bankShift = 0; 
  while((unsigned)(64 >> bankShift) != g_mib.WinGranularity)
    bankShift++;

  bank <<= bankShift;

  g_vm86ctx.eax=0x4f05;
  g_vm86ctx.ebx = 0x0;
  g_vm86ctx.edx = bank;
  vm86int(0x10, &g_vm86ctx);

  g_vm86ctx.eax=0x4f05;
  g_vm86ctx.ebx = 0x1;
  g_vm86ctx.edx = bank;
  vm86int(0x10, &g_vm86ctx);
}

void putPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b)
{
  long addr = y * g_mib.BytesPerScanLine + x*(g_mib.BitsPerPixel/8);
  uint8_t *p = (uint8_t *)LADDR(g_mib.WinASegment, LOWORD(addr));
  setBank(HIWORD(addr));
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

  g_oldmode = getVBEMode();
  return setVBEMode(mode);
}

void exitGraphics()
{
  setVBEMode(g_oldmode);
}

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

/* Draw a simple moire pattern of lines on the display */
void drawMoire(void)
{
    int i;
    int xres = g_mib.XResolution,
        yres = g_mib.YResolution;
    
    for (i = 0; i < xres; i += 5)
    {
        line(xres/2,yres/2,i,0,i % 0xFF, 0, 0); 
        line(xres/2,yres/2,i,yres,(i+1) % 0xFF, 0, 0);
    }
    for (i = 0; i < yres; i += 5)
    {
        line(xres/2,yres/2,0,i,(i+2) % 0xFF, 0, 0); 
        line(xres/2,yres/2,xres,i,(i+3) % 0xFF, 0, 0);
    } 
    line(0,0,xres-1,0,15, 0, 0); 
    line(0,0,0,yres-1,15, 0, 0);
    line(xres-1,0,xres-1,yres-1,15, 0, 0);
    line(0,yres-1,xres-1,yres-1,15, 0, 0);
}


void testGraphics()
{
  if(initGraphics(0x115)) {
    return;
  }

  drawMoire();

  exitGraphics();

  listGraphicsModes();
}

