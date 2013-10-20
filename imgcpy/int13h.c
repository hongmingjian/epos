#include "int13h.h"

uint8  BIOS_UnpackSector (uint16 PackedCylSec)
{
  return PackedCylSec & 0x3F;
}

uint16 BIOS_UnpackCylinder (uint16 PackedCylSec)
{
  return (PackedCylSec>>8) | ((PackedCylSec<<2) & 0x300);
}

uint16 BIOS_PackCylSec (uint16 Cylinder, uint8 Sector)
{
  return (Cylinder<<8) | ((Cylinder>>2) & 0xC0) | Sector;
}

uint32 BIOS_CHS2LBA (tFATBPB* pBPB, uint16 Cylinder, uint8 Head, uint8 Sector)
{
  return ((uint32)Cylinder * pBPB->BPB1.HeadsPerCylinder + Head) *
         pBPB->BPB1.SectorsPerTrack + Sector - 1;
}

void BIOS_LBA2CHS (tFATBPB* pBPB, uint32 LBA, uint16* pCylinder, uint8* pHead, uint8* pSector)
{
  uint32 tmp = LBA / pBPB->BPB1.SectorsPerTrack;
  *pSector  = (LBA % pBPB->BPB1.SectorsPerTrack) + 1;
  *pCylinder = tmp / pBPB->BPB1.HeadsPerCylinder;
  *pHead     = tmp % pBPB->BPB1.HeadsPerCylinder;
}

int BIOS_IsFloppyDisk (uint8 Drive)
{
  return Drive < 0x80;
}

int BIOS_IsHardDisk (uint8 Drive)
{
  return Drive >= 0x80;
}

int BIOS_ResetFloppyDisk (uint8 Drive)
{
  union REGS inregs, outregs;
  struct SREGS segregs;

  if (!BIOS_IsFloppyDisk (Drive))
    return 0;

  inregs.h.ah = 0x00;
  inregs.h.dl = Drive;
  int86x (0x13, &inregs, &outregs, &segregs);

  return !outregs.x.cflag;
}

int BIOS_ResetHardDisk (uint8 Drive)
{
  union REGS inregs, outregs;
  struct SREGS segregs;

  if (!BIOS_IsHardDisk (Drive))
    return 0;

  inregs.h.ah = 0x00;
  inregs.h.dl = Drive;
  int86x (0x13, &inregs, &outregs, &segregs);

  return !outregs.x.cflag;
}

int BIOS_ResetDisk (uint8 Drive)
{
  if (BIOS_IsFloppyDisk (Drive))
    return BIOS_ResetFloppyDisk (Drive);
  else
    return BIOS_ResetHardDisk (Drive);
}

int BIOS_GetDriveParameters (uint8 Drive, uint8* pMaxHead, uint16* pMaxPackedCylSec)
{
  union REGS inregs, outregs;
  struct SREGS segregs;

  inregs.h.ah = 0x08;
  inregs.h.dl = Drive;
  inregs.x.di = 0;
  segregs.es = 0;
  int86x (0x13, &inregs, &outregs, &segregs);

  *pMaxHead = outregs.h.dh;
  *pMaxPackedCylSec = outregs.x.cx;
  return !outregs.x.cflag;
}

tBIOSDriveType BIOS_GetDriveType (uint8 Drive)
{
  union REGS inregs, outregs;
  struct SREGS segregs;

  inregs.h.ah = 0x15;
  inregs.h.dl = Drive;
  int86x (0x13, &inregs, &outregs, &segregs);

  if (outregs.x.cflag)
    return bdt_NO_DRIVE;

  switch (outregs.h.ah)
  {
    case bdt_NO_DRIVE:
    case bdt_REMOVABLE_NO_CHANGE_CTRL:
    case bdt_REMOVABLE_WITH_CHANGE_CTRL:
    case bdt_HARD_DRIVE:
      return (tBIOSDriveType)outregs.h.ah;
    default:
      return bdt_NO_DRIVE;
  }
}

#if 0
static tBIOSMediaChange BIOS_HasMediaChanged (uint8 Drive)
{
  union REGS inregs, outregs;
  struct SREGS segregs;
  tBIOSDriveType t;

  t = BIOS_GetDriveType (Drive);
  switch (t)
  {
    case bdt_NO_DRIVE:
    case bdt_HARD_DRIVE:
    default:
      return bmc_NOT_CHANGED;
    case bdt_REMOVABLE_NO_CHANGE_CTRL:
      return bmc_DONT_KNOW;
    case bdt_REMOVABLE_WITH_CHANGE_CTRL:
      inregs.h.ah = 0x16;
      inregs.h.dl = Drive;
      int86x (0x13, &inregs, &outregs, &segregs);
    return (outregs.h.ah == 6) ? bmc_CHANGED : bmc_NOT_CHANGED;
  }
}
#endif

#define GET_PHYS_ADDR(x) ( (((uint32)FP_SEG(x))<<4) + FP_OFF(x) )

// Used to align sector buffer so that it doesn't cross 64KB boundary,
// which is important for DMA-based BIOS disk I/O:
static void far* AlignBuf (const void far *pBuf, size_t BufSize)
{
  uint32 a = GET_PHYS_ADDR(pBuf);
  if ((a ^ (a + BufSize - 1)) & 0xFFFF0000UL)
    a = (a + 0x10000UL) & 0xFFFF0000UL;
  return MK_FP((const)(a>>4),(const)(a&15));
}

#if 0
static void* Far2Near (const void far* pBuf, size_t BufSize)
{
  static const char c = 0;
  uint32 a = GET_PHYS_ADDR(pBuf);
  uint32 b = (uint32)FP_SEG(&c)<<4;
  if ((b > a) || (b + 0x10000UL < a + BufSize))
    return NULL;
  return (void*)(a - b);
}
#endif

tIORes BIOS_ReadSector (uint8 Drive, uint8 Head, uint16 PackedCylSec,
                        uint8 SectorCount, void far* pBuf)
{
  union REGS inregs, outregs;
  struct SREGS segregs;
  int attempts = 5;

  while (attempts--)
  {
    // read sector(s)
    inregs.h.ah = 0x02;
    inregs.h.al = SectorCount;
    inregs.x.cx = PackedCylSec;
    inregs.h.dh = Head;
    inregs.h.dl = Drive;
    inregs.x.bx = FP_OFF (pBuf);
    segregs.es  = FP_SEG (pBuf);
    int86x (0x13, &inregs, &outregs, &segregs);

    if ((outregs.h.ah == 6) && outregs.x.cflag)
    {
      return ior_ERR_MEDIA_CHANGED;
    }

    if (!outregs.x.cflag)
      return ior_OK;
    BIOS_ResetDisk (Drive);
  }
  
  return ior_ERR_IO;
}

tIORes BIOS_WriteSector (uint8 Drive, uint8 Head, uint16 PackedCylSec,
                         uint8 SectorCount, const void far* pBuf)
{
  union REGS inregs, outregs;
  struct SREGS segregs;
  int attempts = 5;

  while (attempts--)
  {
    // write sector(s)
    inregs.h.ah = 0x03;
    inregs.h.al = SectorCount;
    inregs.x.cx = PackedCylSec;
    inregs.h.dh = Head;
    inregs.h.dl = Drive;
    inregs.x.bx = FP_OFF (pBuf);
    segregs.es  = FP_SEG (pBuf);
    int86x (0x13, &inregs, &outregs, &segregs);

    if ((outregs.h.ah == 6) && outregs.x.cflag)
    {
      return ior_ERR_MEDIA_CHANGED;
    }

    if (!outregs.x.cflag)
      return ior_OK;
    BIOS_ResetDisk (Drive);
  }

  return ior_ERR_IO;
}

int BIOS_IsExtendedRWSupported (uint8 Drive)
{
  union REGS inregs, outregs;
  struct SREGS segregs;

  if (BIOS_IsFloppyDisk (Drive))
    return 0;

  inregs.h.ah = 0x41;
  inregs.h.dl = Drive;
  inregs.x.bx = 0x55AA;
  int86x (0x13, &inregs, &outregs, &segregs);
  return ((!outregs.x.cflag) && (outregs.x.bx == 0xAA55) && (outregs.x.cx & 1));
}

tIORes BIOS_ExtendedReadSector (uint8 Drive, uint32 LBA,
                                uint8 SectorCount, void far* pBuf)
{
  tInt13hExtRWPacket p;
  union REGS inregs, outregs;
  struct SREGS segregs;
  int attempts = 5;

  p.SizeOfThisPacket = sizeof(p);
  p.Reserved = 0;
  p.BlocksCount = SectorCount;
  p.pDataBuffer = pBuf;
  p.LoLBA = LBA;
  p.HiLBA = 0;

  while (attempts--)
  {
    // read sector(s)
    inregs.h.ah = 0x42;
    inregs.h.dl = Drive;
    inregs.x.si = FP_OFF (&p);
    segregs.ds  = FP_SEG (&p);
    int86x (0x13, &inregs, &outregs, &segregs);

    if ((outregs.h.ah == 6) && outregs.x.cflag)
    {
      return ior_ERR_MEDIA_CHANGED;
    }

    if (!outregs.x.cflag)
      return ior_OK;
    BIOS_ResetHardDisk (Drive);
  }

  return ior_ERR_IO;
}

tIORes BIOS_ExtendedWriteSector (uint8 Drive, uint32 LBA,
                                 uint8 SectorCount, const void far* pBuf)
{
  tInt13hExtRWPacket p;
  union REGS inregs, outregs;
  struct SREGS segregs;
  int attempts = 5;

  p.SizeOfThisPacket = sizeof(p);
  p.Reserved = 0;
  p.BlocksCount = SectorCount;
  p.pDataBuffer = (void far*)pBuf;
  p.LoLBA = LBA;
  p.HiLBA = 0;

  while (attempts--)
  {
    // write sector(s)
    inregs.h.ah = 0x43;
    inregs.h.al = 0x00; // write w/o verifying
    inregs.h.dl = Drive;
    inregs.x.si = FP_OFF (&p);
    segregs.ds  = FP_SEG (&p);
    int86x (0x13, &inregs, &outregs, &segregs);

    if ((outregs.h.ah == 6) && outregs.x.cflag)
    {
      return ior_ERR_MEDIA_CHANGED;
    }

    if (!outregs.x.cflag)
      return ior_OK;
    BIOS_ResetHardDisk (Drive);
  }

  return ior_ERR_IO;
}

tIORes BIOS_ReadSectorLBA (tFATBPB* pBPB, uint8 Drive, uint32 LBA,
                           uint8 SectorCount, void far* pBuf)
{
  uint16 Cylinder = 0;
  uint8 Head = 0, Sector = 1;
  tFATBPB BPB;
  int IsNeededCHS;
  tIORes res;
  uint8 SectorBuf[1024];
  void far* pIOBuf;

  pIOBuf = AlignBuf (SectorBuf, 512);

  IsNeededCHS = !BIOS_IsExtendedRWSupported (Drive);

  if (IsNeededCHS && (pBPB == NULL))
  {
    uint16 MaxPackedCylSec;
    uint8 MaxHead, MaxSector;
    if (!BIOS_GetDriveParameters (Drive, &MaxHead, &MaxPackedCylSec))
      return ior_ERR_IO;
    MaxSector = BIOS_UnpackSector (MaxPackedCylSec);
    BPB.BPB1.SectorsPerTrack = MaxSector;
    BPB.BPB1.HeadsPerCylinder = MaxHead+1;
    pBPB = &BPB;
  }

  while (SectorCount--)
  {
    if (IsNeededCHS)
    {
      if (LBA)
        BIOS_LBA2CHS (pBPB, LBA, &Cylinder, &Head, &Sector);

      res = BIOS_ReadSector (Drive, Head, BIOS_PackCylSec (Cylinder, Sector),
                             1, pIOBuf);
    }
    else
    {
      res = BIOS_ExtendedReadSector (Drive, LBA, 1, pIOBuf);
    }

    movedata (FP_SEG(pIOBuf), FP_OFF(pIOBuf),
              FP_SEG(pBuf),   FP_OFF(pBuf), 512);

    if (res != ior_OK)
      return res;

    LBA++;
    { char far* cpBuf = pBuf; cpBuf += 512; pBuf = cpBuf; }
  }

  return ior_OK;
}

tIORes BIOS_WriteSectorLBA (tFATBPB* pBPB, uint8 Drive, uint32 LBA,
                            uint8 SectorCount, const void far* pBuf)
{
  uint16 Cylinder = 0;
  uint8 Head = 0, Sector = 1;
  tFATBPB BPB;
  int IsNeededCHS;
  tIORes res;
  uint8 SectorBuf[1024];
  void far* pIOBuf;

  pIOBuf = AlignBuf (SectorBuf, 512);

  IsNeededCHS = !BIOS_IsExtendedRWSupported (Drive);

  if (IsNeededCHS && (pBPB == NULL))
  {
    uint16 MaxPackedCylSec;
    uint8 MaxHead, MaxSector;
    if (!BIOS_GetDriveParameters (Drive, &MaxHead, &MaxPackedCylSec))
      return ior_ERR_IO;
    MaxSector = BIOS_UnpackSector (MaxPackedCylSec);
    BPB.BPB1.SectorsPerTrack = MaxSector;
    BPB.BPB1.HeadsPerCylinder = MaxHead+1;
    pBPB = &BPB;
  }

  while (SectorCount--)
  {
    movedata (FP_SEG(pBuf),   FP_OFF(pBuf),
              FP_SEG(pIOBuf), FP_OFF(pIOBuf), 512);

    if (IsNeededCHS)
    {
      if (LBA)
        BIOS_LBA2CHS (pBPB, LBA, &Cylinder, &Head, &Sector);

      res = BIOS_WriteSector (Drive, Head, BIOS_PackCylSec (Cylinder, Sector),
                              1, pIOBuf);
    }
    else
    {
      res = BIOS_ExtendedWriteSector (Drive, LBA, 1, pIOBuf);
    }

    if (res != ior_OK)
      return res;

    LBA++;
    { const char far* cpBuf = pBuf; cpBuf += 512; pBuf = cpBuf; }
  }

  return ior_OK;
}
