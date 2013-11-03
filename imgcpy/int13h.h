#ifndef _INT_13H_H_
#define _INT_13H_H_

#include <string.h>
//#include <dos.h>
//#include <mem.h>

#include "ComTypes.h"
#include "ComDefs.h"

#include "FAT.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack (push, 1)

typedef struct tInt13hExtRWPacket {
  uint8     SizeOfThisPacket;
  uint8     Reserved;
  uint16    BlocksCount;
  void far* pDataBuffer;
  uint32    LoLBA;
  uint32    HiLBA;
} tInt13hExtRWPacket;

typedef enum tBIOSDriveType {
  bdt_NO_DRIVE                      = 0x00,
  bdt_REMOVABLE_NO_CHANGE_CTRL      = 0x01,
  bdt_REMOVABLE_WITH_CHANGE_CTRL    = 0x02,
  bdt_HARD_DRIVE                    = 0x03
} tBIOSDriveType;

typedef enum tBIOSMediaChange {
  bmc_NOT_CHANGED,
  bmc_CHANGED,
  bmc_DONT_KNOW
} tBIOSMediaChange;

#pragma pack (pop)

extern uint8  BIOS_UnpackSector (uint16 PackedCylSec);
extern uint16 BIOS_UnpackCylinder (uint16 PackedCylSec);
extern uint16 BIOS_PackCylSec (uint16 Cylinder, uint8 Sector);

extern uint32 BIOS_CHS2LBA (tFATBPB* pBPB, uint16 Cylinder, uint8 Head, uint8 Sector);
extern void BIOS_LBA2CHS (tFATBPB* pBPB, uint32 LBA, uint16* pCylinder, uint8* pHead, uint8* pSector);

extern int BIOS_IsFloppyDisk (uint8 Drive);
extern int BIOS_IsHardDisk (uint8 Drive);

extern int BIOS_ResetFloppyDisk (uint8 Drive);
extern int BIOS_ResetHardDisk (uint8 Drive);
extern int BIOS_ResetDisk (uint8 Drive);
extern int BIOS_GetDriveParameters (uint8 Drive,
                                    uint8* pMaxHead, uint16* pMaxPackedCylSec);
extern tBIOSDriveType BIOS_GetDriveType (uint8 Drive);

extern tIORes BIOS_ReadSector (uint8 Drive, uint8 Head, uint16 PackedCylSec,
                               uint8 SectorCount, void far* pBuf);
extern tIORes BIOS_WriteSector (uint8 Drive, uint8 Head, uint16 PackedCylSec,
                                uint8 SectorCount, const void far* pBuf);

extern int BIOS_IsExtendedRWSupported (uint8 Drive);
extern tIORes BIOS_ExtendedReadSector (uint8 Drive, uint32 LBA,
                                       uint8 SectorCount, void far* pBuf);
extern tIORes BIOS_ExtendedWriteSector (uint8 Drive, uint32 LBA,
                                        uint8 SectorCount, const void far* pBuf);

extern tIORes BIOS_ReadSectorLBA (tFATBPB* pBPB, uint8 Drive, uint32 LBA,
                                  uint8 SectorCount, void far* pBuf);
extern tIORes BIOS_WriteSectorLBA (tFATBPB* pBPB, uint8 Drive, uint32 LBA,
                                   uint8 SectorCount, const void far* pBuf);

#ifdef __cplusplus
}
#endif

#endif // _INT_13H_H_
