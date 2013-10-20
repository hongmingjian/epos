#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include "int13h.h"
#include "FAT.h"
#include "image.h"

typedef struct tBIOSDriveAndImageFile {
  uint8 BIOSDrive;
  FILE* pImageFile;
  uint32 SectorCount;
} tBIOSDriveAndImageFile;

tBIOSDriveAndImageFile* pBIOSDriveAndImageFile = NULL;
unsigned BIOSDriveAndImageFileCount = 0;

tFATFS stdio_FATFS;
int stdio_IsFATFSInit = 0;

long fsize (FILE *f)
{
  long len, cur;
  if (f == NULL)
    return -1;
  cur = ftell (f);
  if (cur < 0)
    return -1;
  if (fseek (f, 0, SEEK_END))
    return -1;
  len = ftell (f);
  if (len < 0)
    return -1;
  if (fseek (f, cur, SEEK_SET))
    return -1;
  return len;
}

void CloseImageFiles (void)
{
  uint i;

  if (BIOSDriveAndImageFileCount && (pBIOSDriveAndImageFile != NULL))
  {
    for (i = 0; i < BIOSDriveAndImageFileCount; i++)
    {
      if (pBIOSDriveAndImageFile[i].pImageFile != NULL)
        fclose (pBIOSDriveAndImageFile[i].pImageFile);

      pBIOSDriveAndImageFile[i].pImageFile = NULL;
    }

    BIOSDriveAndImageFileCount = 0;
    free (pBIOSDriveAndImageFile);
  }
}

tBIOSDriveAndImageFile* GetImage (uint8 Drive)
{
  uint i;

  if ((!BIOSDriveAndImageFileCount) || (pBIOSDriveAndImageFile == NULL))
    return NULL;

  for (i = 0; i < BIOSDriveAndImageFileCount; i++)
    if (pBIOSDriveAndImageFile[i].BIOSDrive == Drive)
      return &pBIOSDriveAndImageFile[i];

  return NULL;
}

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
  (void)Drive;

  return 1;
}

int BIOS_ResetHardDisk (uint8 Drive)
{
  (void)Drive;

  return 1;
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
  tBIOSDriveAndImageFile* pBIOSDriveAndImageFile;

  switch (Drive)
  {
    case 0:
    case 1:
      if (GetImage (Drive) != NULL)
      {
        // 512*2*18*80 = 1.44 MB floppy
        *pMaxHead = 1;
        *pMaxPackedCylSec = BIOS_PackCylSec (80, 18);
        return 1;
      }
      break;

    case 0x80:
    case 0x81:
    case 0x82:
    case 0x83:
      if ((pBIOSDriveAndImageFile = GetImage (Drive)) != NULL)
      {
        // 512*256*63*24 = 189 MB
        // 512*256*63*33 = 256 MB
        *pMaxHead = 255;
        *pMaxPackedCylSec = BIOS_PackCylSec (pBIOSDriveAndImageFile->SectorCount / 256 / 63, 63);
        return 1;
      }
  }
  return 0;
}

tBIOSDriveType BIOS_GetDriveType (uint8 Drive)
{
  switch (Drive)
  {
    case 0:
    case 1:
      if (GetImage (Drive) != NULL)
        return bdt_REMOVABLE_NO_CHANGE_CTRL;
      break;

    case 0x80:
    case 0x81:
    case 0x82:
    case 0x83:
      if (GetImage (Drive) != NULL)
        return bdt_HARD_DRIVE;
      break;
  }
  return bdt_NO_DRIVE;
}

static void far* AlignBuf (const void far *pBuf, size_t BufSize)
{
  (void)BufSize;
  return (void*)pBuf;
}

static void* Far2Near (const void far* pBuf, size_t BufSize)
{
  (void)BufSize;
  return (void*)pBuf;
}

tIORes BIOS_ReadSector (uint8 Drive, uint8 Head, uint16 PackedCylSec,
                        uint8 SectorCount, void far *pBuf)
{
  tBIOSDriveAndImageFile* pBIOSDriveAndImageFile;
  FILE* f;
  uint16 MaxPackedCylSec;
  uint8 MaxHead;
  tFATBPB BPB;
  uint32 LBA;
  void* p;

  pBIOSDriveAndImageFile = GetImage (Drive);

  if (pBIOSDriveAndImageFile == NULL)
    return ior_ERR_IO;

  if ((f = pBIOSDriveAndImageFile->pImageFile) == NULL)
    return ior_ERR_IO;

  if (!BIOS_GetDriveParameters (Drive, &MaxHead, &MaxPackedCylSec))
    return ior_ERR_IO;

  BPB.BPB1.SectorsPerTrack = BIOS_UnpackSector (MaxPackedCylSec);
  BPB.BPB1.HeadsPerCylinder = MaxHead+1;

  LBA = BIOS_CHS2LBA (&BPB,
                      BIOS_UnpackCylinder (PackedCylSec),
                      Head,
                      BIOS_UnpackSector (PackedCylSec));

  if (fseek (f, LBA*512, SEEK_SET) != 0)
    return ior_ERR_IO;

  p = Far2Near (pBuf, 512UL*SectorCount);
  if (p == NULL)
    return ior_ERR_IO;

  if (fread (p, 1, 512UL*SectorCount, f) != 512UL*SectorCount)
    return ior_ERR_IO;

  return ior_OK;
}

tIORes BIOS_WriteSector (uint8 Drive, uint8 Head, uint16 PackedCylSec,
                         uint8 SectorCount, const void far *pBuf)
{
  tBIOSDriveAndImageFile* pBIOSDriveAndImageFile;
  FILE* f;
  uint16 MaxPackedCylSec;
  uint8 MaxHead;
  tFATBPB BPB;
  uint32 LBA;
  void* p;

  pBIOSDriveAndImageFile = GetImage (Drive);

  if (pBIOSDriveAndImageFile == NULL)
    return ior_ERR_IO;

  if ((f = pBIOSDriveAndImageFile->pImageFile) == NULL)
    return ior_ERR_IO;

  if (!BIOS_GetDriveParameters (Drive, &MaxHead, &MaxPackedCylSec))
    return ior_ERR_IO;

  BPB.BPB1.SectorsPerTrack = BIOS_UnpackSector (MaxPackedCylSec);
  BPB.BPB1.HeadsPerCylinder = MaxHead+1;

  LBA = BIOS_CHS2LBA (&BPB,
                      BIOS_UnpackCylinder (PackedCylSec),
                      Head,
                      BIOS_UnpackSector (PackedCylSec));

  if (fseek (f, LBA*512, SEEK_SET) != 0)
    return ior_ERR_IO;

  p = Far2Near (pBuf, 512UL*SectorCount);
  if (p == NULL)
    return ior_ERR_IO;

  if (fwrite (p, 1, 512UL*SectorCount, f) != 512UL*SectorCount)
    return ior_ERR_IO;

  return ior_OK;
}

int BIOS_IsExtendedRWSupported (uint8 Drive)
{
  if (BIOS_IsFloppyDisk (Drive))
    return 0;

  return 1;
}

tIORes BIOS_ExtendedReadSector (uint8 Drive, uint32 LBA,
                                uint8 SectorCount, void far *pBuf)
{
  tBIOSDriveAndImageFile* pBIOSDriveAndImageFile;
  FILE* f;
  void* p;

  pBIOSDriveAndImageFile = GetImage (Drive);

  if (pBIOSDriveAndImageFile == NULL)
    return ior_ERR_IO;

  if ((f = pBIOSDriveAndImageFile->pImageFile) == NULL)
    return ior_ERR_IO;

  if (fseek (f, LBA*512, SEEK_SET) != 0)
    return ior_ERR_IO;

  p = Far2Near (pBuf, 512UL*SectorCount);
  if (p == NULL)
    return ior_ERR_IO;

  if (fread (p, 1, 512UL*SectorCount, f) != 512UL*SectorCount)
    return ior_ERR_IO;

  return ior_OK;
}

tIORes BIOS_ExtendedWriteSector (uint8 Drive, uint32 LBA,
                                 uint8 SectorCount, const void far *pBuf)
{
  tBIOSDriveAndImageFile* pBIOSDriveAndImageFile;
  FILE* f;
  void* p;

  pBIOSDriveAndImageFile = GetImage (Drive);

  if (pBIOSDriveAndImageFile == NULL)
    return ior_ERR_IO;

  if ((f = pBIOSDriveAndImageFile->pImageFile) == NULL)
    return ior_ERR_IO;

  if (fseek (f, LBA*512, SEEK_SET) != 0)
    return ior_ERR_IO;

  p = Far2Near (pBuf, 512UL*SectorCount);
  if (p == NULL)
    return ior_ERR_IO;

  if (fwrite (p, 1, 512UL*SectorCount, f) != 512UL*SectorCount)
    return ior_ERR_IO;

  return ior_OK;
}

tIORes BIOS_ReadSectorLBA (tFATBPB* pBPB, uint8 Drive, uint32 LBA,
                           uint8 SectorCount, void far *pBuf)
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

#if 0
    movedata (FP_SEG(pIOBuf), FP_OFF(pIOBuf),
              FP_SEG(pBuf),   FP_OFF(pBuf), 512);
#else
    memcpy (pBuf, pIOBuf, 512);
#endif

    if (res != ior_OK)
      return res;

    LBA++;
    { char far* cpBuf = pBuf; cpBuf += 512; pBuf = cpBuf; }
  }

  return ior_OK;
}

tIORes BIOS_WriteSectorLBA (tFATBPB* pBPB, uint8 Drive, uint32 LBA,
                            uint8 SectorCount, const void far *pBuf)
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
#if 0
    movedata (FP_SEG(pBuf),   FP_OFF(pBuf),
              FP_SEG(pIOBuf), FP_OFF(pIOBuf), 512);
#else
    memcpy (pIOBuf, pBuf, 512);
#endif

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

int DoesDriveExist (tFATFS* pFATFS, uint8 Drive)
{
  (void)pFATFS;
  return BIOS_GetDriveType (Drive) != bdt_NO_DRIVE;
}

void GetDateAndTime (tFATFS* pFATFS,
                     unsigned* pYear, unsigned* pMonth, unsigned* pDay,
                     unsigned* pHour, unsigned* pMinute, unsigned* pSecond)
{
  time_t t;
  struct tm* ptm;

  (void)pFATFS;

  time (&t);
  ptm = localtime (&t);

  *pYear   = ptm->tm_year + 1900;
  *pMonth  = ptm->tm_mon + 1;
  *pDay    = ptm->tm_mday;
  *pHour   = ptm->tm_hour;
  *pMinute = ptm->tm_min;
  *pSecond = ptm->tm_sec;
}

int IsFloppyTooLongUnaccessed (tFATFS* pFATFS, time_t LastTime,
                               time_t* pNewTime)
{
  time_t NewTime;

  (void)pFATFS;

  NewTime = time (NULL);
  if (pNewTime != NULL)
    *pNewTime = NewTime;

  // if a floppy hasn't been accessed for 2 seconds or more, it will be checked
  // for a change (using serial number). And if there was a change, the caches
  // will be invalidated:
  return NewTime - LastTime >= 2;
}

static tIORes ReadSectorLBA (tFATFS* pFATFS,
                             tFATBPB* pBPB, uint8 Drive, uint32 LBA,
                             uint8 SectorCount, void *pBuf)
{
  (void)pFATFS;
  return BIOS_ReadSectorLBA (pBPB, Drive, LBA, SectorCount, (void far*)pBuf);
}

static tIORes WriteSectorLBA (tFATFS* pFATFS,
                              tFATBPB* pBPB, uint8 Drive, uint32 LBA,
                              uint8 SectorCount, const void *pBuf)
{
  (void)pFATFS;
  return BIOS_WriteSectorLBA (pBPB, Drive, LBA, SectorCount, (const void far*)pBuf);
}

static void _printf (tFATFS* pFATFS,
                     const char* format, ...)
{
#if 0//!PRINT_FAT_FS_LOGS
  (void)pFATFS;
  (void)format;
#else
  va_list args;

  (void)pFATFS;

  va_start (args, format);
  vprintf (format, args);

  va_end(args);
#endif
}

tFATFile* _OpenFileSearch (tFATFS* pFATFS,
                           tReadDirState* pReadDirState,
                           tpOpenFilesMatchFxn pOpenFilesMatch)
{
  (void)pFATFS;
  (void)pReadDirState;
  (void)pOpenFilesMatch;
  return NULL;
}

int stdio_InitFATFS(const tBIOSDriveAndImageFName* pBIOSDriveAndImageFileName,
                    unsigned BIOSDrivesCount,
                    int _FATBootDiskHint)
{
  if (!stdio_IsFATFSInit)
  {
    unsigned i;
    int HasOpenFailed = 0;

    if ((pBIOSDriveAndImageFileName == NULL) || (!BIOSDrivesCount))
    {
      return 0;
    }

    pBIOSDriveAndImageFile = malloc(sizeof(tBIOSDriveAndImageFile) * BIOSDrivesCount);

    if (pBIOSDriveAndImageFile == NULL)
      return 0;

    BIOSDriveAndImageFileCount = BIOSDrivesCount;

    for (i = 0; i < BIOSDrivesCount; i++)
    {
      pBIOSDriveAndImageFile[i].BIOSDrive =
        pBIOSDriveAndImageFileName[i].BIOSDrive;
      pBIOSDriveAndImageFile[i].pImageFile =
        fopen (pBIOSDriveAndImageFileName[i].pImageFileName, "r+b");
      if (pBIOSDriveAndImageFile[i].pImageFile != NULL)
      {
        long fsz = fsize(pBIOSDriveAndImageFile[i].pImageFile);
        if (fsz < 0)
          HasOpenFailed = 1;
        else
          pBIOSDriveAndImageFile[i].SectorCount = fsz / 512;
      }
      else
      {
        HasOpenFailed = 1;
      }
    }

    if (!HasOpenFailed)
    {
      uint8* pBIOSDrivesToScan;
      if ((pBIOSDrivesToScan = malloc(sizeof(uint8)*BIOSDrivesCount)) != NULL)
      {
        for (i = 0; i < BIOSDrivesCount; i++)
          pBIOSDrivesToScan[i] = pBIOSDriveAndImageFile[i].BIOSDrive;

        stdio_IsFATFSInit =
          FAT_Init
          (
            &stdio_FATFS,
            pBIOSDrivesToScan,
            BIOSDrivesCount,
            &DoesDriveExist,
            &ReadSectorLBA,
            &WriteSectorLBA,
            &GetDateAndTime,
            &IsFloppyTooLongUnaccessed,
            &_OpenFileSearch,
            &_printf,
            _FATBootDiskHint
          );

        free (pBIOSDrivesToScan);
      }
    }

    if (HasOpenFailed || !stdio_IsFATFSInit)
      CloseImageFiles();
  }

  return stdio_IsFATFSInit;
}

int stdio_FlushFATFS (void)
{
  int res = 1;
  if (!stdio_IsFATFSInit)
    return 0;

  // First, flush all open files:
#if 0
  flushall();
#endif

  if (!FAT_Flush (&stdio_FATFS))
    res = 0;

  return res;
}

int stdio_DoneFATFS (void)
{
  int res = 1;

  if (!stdio_IsFATFSInit)
    return 0;

  // First, close all open files:
#if 0
  if (fcloseall() == EOF)
    res = 0;
#endif

  if (!FAT_Done (&stdio_FATFS))
    res = 0;

  CloseImageFiles();

  stdio_IsFATFSInit = 0;

  return res;
}
