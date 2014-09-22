/**
 * vim: filetype=c:fenc=utf-8:ts=2:et:sw=2:sts=2
 */
#include "../global.h"
#include "syscall.h"
#include "math.h"

#define LADDR(seg,off) ((uint32_t)(((uint16_t)(seg)<<4)+(uint16_t)(off)))
int vm86int(int n, struct vm86_context *vm86ctx);

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

static void switchBank(int bank)
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

    if(!(pmib->ModeAttributes & 0x01))
      continue;
    if(!(pmib->ModeAttributes & 0x10))
      continue;

    if(pmib->BitsPerPixel != 24)
      continue;
    if(pmib->NumberOfPlanes != 1)
      continue;
    if(pmib->MemoryModel != 6)
      continue;

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

  g_oldmode = getVBEMode();
  return setVBEMode(mode);
}

int exitGraphics()
{
  return setVBEMode(g_oldmode);
}

/*
 * The codes of `line' and `drawMoire' come from
 *
 *         VESA BIOS EXTENSION(VBE)
 *                Core Functions
 *                  Standard
 *
 *                  Version: 3.0
 */
/* Draw a line from (x1,y1) to (x2,y2) in specified color */
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

/*http://www.brackeen.com/vga*/
void polygon(int num_vertices,
             int *vertices,
             uint8_t r, uint8_t g, uint8_t b)
{
  int i;

  for(i=0;i<num_vertices-1;i++)
  {
    line(vertices[(i<<1)+0],
         vertices[(i<<1)+1],
         vertices[(i<<1)+2],
         vertices[(i<<1)+3],
         r, g, b);
  }
  line(vertices[0],
       vertices[1],
       vertices[(num_vertices<<1)-2],
       vertices[(num_vertices<<1)-1],
       r, g, b);
}

/*http://rosettacode.org/wiki/Bitmap/Midpoint_circle_algorithm#C*/
void circle(int x0, int y0,
            int radius,
            uint8_t r, uint8_t g, uint8_t b)
{
    int f = 1 - radius;
    int ddF_x = 0;
    int ddF_y = -2 * radius;
    int x = 0;
    int y = radius;

    putPixel(x0, y0 + radius, r, g, b);
    putPixel(x0, y0 - radius, r, g, b);
    putPixel(x0 + radius, y0, r, g, b);
    putPixel(x0 - radius, y0, r, g, b);

    while(x < y)
    {
        if(f >= 0)
        {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x + 1;
        putPixel(x0 + x, y0 + y, r, g, b);
        putPixel(x0 - x, y0 + y, r, g, b);
        putPixel(x0 + x, y0 - y, r, g, b);
        putPixel(x0 - x, y0 - y, r, g, b);
        putPixel(x0 + y, y0 + x, r, g, b);
        putPixel(x0 - y, y0 + x, r, g, b);
        putPixel(x0 + y, y0 - x, r, g, b);
        putPixel(x0 - y, y0 - x, r, g, b);
    }
}

/*http://www.itglitz.in/CS71/cg_unit_1_notes.pdf*/
#define ROUND(a) ((int)(a+0.5))
void ellipse(int x0, int y0,
             int rx, int ry,
             uint8_t r, uint8_t g, uint8_t b)
{
  int rx2=rx*rx;
  int ry2=ry*ry;
  int tworx2 = 2*rx2;
  int twory2 = 2*ry2;
  int p;
  int x = 0;
  int y = ry;
  int px = 0;
  int py = tworx2*y;

  /* Plot the first set of points */
  putPixel (x0 + x, y0 + y, r, g, b);
  putPixel (x0 - x, y0 + y, r, g, b);
  putPixel (x0 + x, y0 - y, r, g, b);
  putPixel (x0 - x, y0 - y, r, g, b);

  /* Region 1 */
  p = ROUND(ry2 - (rx2* ry) + (0.25*rx2));
  while (px < py) {
    x++;
    px+=twory2;
    if (p < 0)
      p+=ry2+px;
    else {
      y--;
      py-=tworx2;
      p+=ry2+px-py;
    }
    putPixel (x0 + x, y0 + y, r, g, b);
    putPixel (x0 - x, y0 + y, r, g, b);
    putPixel (x0 + x, y0 - y, r, g, b);
    putPixel (x0 - x, y0 - y, r, g, b);
  }

  /* Region 2 */
  p = ROUND(ry2*(x+0.5)*(x+0.5)+ rx2*(y-1)*(y-1) - rx2*ry2);
  while (y > 0) {
    y--;
    py-=tworx2;
    if (p > 0)
      p+=rx2-py;
    else {
      x++;
      px+=twory2;
      p+=rx2-py+px;
    }
    putPixel (x0 + x, y0 + y, r, g, b);
    putPixel (x0 - x, y0 + y, r, g, b);
    putPixel (x0 + x, y0 - y, r, g, b);
    putPixel (x0 - x, y0 - y, r, g, b);
  }
}

/*
 * The codes of `drawCheckerboard' comes from
 *             Vbespy
 */
/* Draw a colorful checkerboard */
void drawCheckerboard(void)
{
  int x, y;
  int xres = g_mib.XResolution,
      yres = g_mib.YResolution;

  for (y = 0; y < yres; ++y) {
    for (x = 0; x < xres; ++x) {
      int r, g, b;
      if ((x & 16) ^ (y & 16)) {
        r = x * 255 / xres;
        g = y * 255 / yres;
        b = 255 - x * 255 / xres;
      } else {
        r = 255 - x * 255 / xres;
        g = y * 255 / yres;
        b = 255 - y * 255 / yres;
      }
      putPixel(x, y, r, g, b);
    }
  }
}

/*http://rosettacode.org/wiki/Mandelbrot_set#JavaScript*/
void mandelbrot()
{
  int width  = g_mib.XResolution,
      height = g_mib.YResolution;
  double xstart=-2, xend=1,
         ystart=-1, yend=1;
  double x, y, xstep, ystep;

  int maxiter = 100;
  int i, j, k;

  xstep = (xend-xstart)/width;
  ystep = (yend-ystart)/height;

  x = xstart; y = ystart;
  for (i=0; i<height; i++) {
    for (j=0; j<width; j++) {

		{
		  double z,zi,newz,newzi;
		  z = 0;
		  zi = 0;
		  for (k=0; k<maxiter; k++) {
			newz = (z*z)-(zi*zi) + x;
			newzi = 2*z*zi + y;
			z = newz;
			zi = newzi;
			if(((z*z)+(zi*zi)) > 4) {
			  break;
			}
		  }
		}

      if (k==maxiter) {
        putPixel(j, i, 0, 0, 0);
      } else {
        double c = 3*log(k)/log(maxiter - 1.0);
        if (c < 1) {
		  putPixel(j, i, 255*c, 0, 0);
        } else if (c < 2) {
		  putPixel(j, i, 255, 255*(c-1), 0);
        } else {
		  putPixel(j, i, 255, 255, 255*(c-2));
        }
      }
      x += xstep;
    }
    y += ystep;
    x = xstart;
  }
}
