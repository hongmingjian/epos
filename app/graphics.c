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
  char *VbeSignature;
  struct vm86_context vm86ctx = {.ss = 0x9000, .esp = 0x0000};
  vm86ctx.eax=0x4f00;
  vm86ctx.es=0x0070;
  vm86ctx.edi=0x0000;
  VbeSignature = (char *)LADDR(vm86ctx.es, LOWORD(vm86ctx.edi));
  VbeSignature[0]='V'; VbeSignature[1]='B';
  VbeSignature[2]='E'; VbeSignature[3]='2';
  vm86int(0x10, &vm86ctx);
  if(LOWORD(vm86ctx.eax) != 0x4f) {
    return -1;
  }

  *pvib = *(struct VBEInfoBlock *)LADDR(vm86ctx.es, LOWORD(vm86ctx.edi));
  return 0;
}

static int getModeInfo(int mode, struct ModeInfoBlock *pmib)
{
  struct vm86_context vm86ctx = {.ss = 0x9000, .esp = 0x0000};
  vm86ctx.eax=0x4f01;
  vm86ctx.ecx=mode;
  vm86ctx.es =0x0090;
  vm86ctx.edi=0x0000;
  vm86int(0x10, &vm86ctx);
  if(LOWORD(vm86ctx.eax) != 0x4f)
    return -1;
  *pmib = *(struct ModeInfoBlock *)LADDR(vm86ctx.es, LOWORD(vm86ctx.edi));
  return 0;
}

static int getVBEMode()
{
  struct vm86_context vm86ctx = {.ss = 0x9000, .esp = 0x0000};
  vm86ctx.eax=0x4f03;
  vm86int(0x10, &vm86ctx);
  if(LOWORD(vm86ctx.eax) != 0x4f)
    return -1;
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
  struct vm86_context vm86ctx = {.ss = 0x9000, .esp = 0x0000};

  vm86ctx.eax=0x4f05;
  vm86ctx.ebx = 0x0100;
  vm86int(0x10, &vm86ctx);
  if(LOWORD(vm86ctx.eax) != 0x4f)
    return -1;

  bank <<= g_bankShift;
  if(bank == LOWORD(vm86ctx.edx))
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
  uint16_t *modep;
#ifdef _WIN32
  uint16_t mode;
#endif

  if(getVBEInfo(&g_vib)) {
    printf("No VESA BIOS EXTENSION(VBE) detected!\r\n");
    return -1;
  }

//  printf("VideoModePtr: %04X:%04X\r\n", HIWORD(g_vib.VideoModePtr),
//         LOWORD(g_vib.VideoModePtr));

  printf("\r\n");
  printf(" Mode      Resolution\r\n");
  printf("----------------------\r\n");
#ifdef _WIN32
  modep = &mode;
  for(mode = 0x100; mode < 0x200; mode++) {
#else
  for(modep = (uint16_t *)LADDR(LOWORD(g_vib.VideoModePtr),
                                HIWORD(g_vib.VideoModePtr));
      *modep != 0xffff;
      modep++) {
#endif
    if(getModeInfo(*modep, &g_mib))
      continue;

    // Must be supported in hardware
    if(!(g_mib.ModeAttributes & 0x1))
      continue;

    // Must be graphics mode
    if(!(g_mib.ModeAttributes & 0x10))
      continue;

    if(g_mib.BitsPerPixel != 24)
      continue;
    if(g_mib.NumberOfPlanes != 1)
      continue;

    printf("0x%04x    %4dx%4dx%2d\r\n",
           *modep,
           g_mib.XResolution,
           g_mib.YResolution,
           g_mib.BitsPerPixel,
           g_mib.BytesPerScanLine
          );
  }
  printf("----------------------\r\n");
  printf(" Mode      Resolution\r\n");
  return 0;
}

int initGraphics(int mode)
{
  if(getVBEInfo(&g_vib)) {
    printf("No VESA BIOS EXTENSION(VBE) detected!\r\n");
    return -1;
  }

  if(getModeInfo(mode, &g_mib)) {
    printf("Mode 0x%04x is not supported!\r\n", mode);
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

void putPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b)
{
  long addr = y * g_mib.BytesPerScanLine + x*(g_mib.BitsPerPixel/8);
  uint8_t *p = (uint8_t *)LADDR(g_mib.WinASegment, LOWORD(addr));
  switchBank(HIWORD(addr));
  *p = b; *(p+1)=g; *(p+2)=r;
}

