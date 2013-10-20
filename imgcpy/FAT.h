#ifndef _FAT_H_
#define _FAT_H_

#include "ComTypes.h"
#include "ComDefs.h"
#include "Cache.h"
#include <string.h>
#include <stdio.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/* BIOS/DOS STRUCTURES */

#pragma pack (push, 1)

/*
  BIOS 24-bit CHS sector addressing can address up to:
    1024 cylinders *  16 heads * 63 sectors * 512 bytes per sector =
    =   528,482,304 bytes (aka 528 MB limit) on old PC/XT/AT BIOSes
    1024 cylinders * 256 heads * 63 sectors * 512 bytes per sector =
    = 8,455,716,864 bytes (aka   8 GB limit), on newer AT BIOSes
*/

/*
  ATA 32-bit LBA sector addressing can address up to:
    2**28 sectors * 512 bytes per sector =
    = 137,438,953,472 bytes (aka 137 GB limit)
    on todays BIOSes with int 13h extensions
*/

// Must always be 512 bytes per sector, other sizes aren't supported:
#define FAT_SECTOR_SIZE         512

typedef struct tCHSSectorPos {
  uint8                 Head;
  uint16                PackedCylSec;
} tCHSSectorPos;

typedef enum tPartitionStatus {
  ps_INACTIVE_PARTITION = 0x00,
  ps_ACTIVE_PARTITION   = 0x80
} tPartitionStatus;

typedef enum tPartitionType {
  pt_UNKNOWN_PARTITION              = 0x00,
  pt_FAT12_PARTITION                = 0x01,
  pt_FAT16_UNDER_32MB_PARTITION     = 0x04,
  pt_EXTENDED_PARTITION             = 0x05,
  pt_FAT16_OVER_32MB_PARTITION      = 0x06,
  pt_FAT32_PARTITION                = 0x0B,
  pt_FAT32_LBA_PARTITION            = 0x0C,
  pt_FAT16_OVER_32MB_LBA_PARTITION  = 0x0E,
  pt_EXTENDED_LBA_PARTITION         = 0x0F
} tPartitionType;

typedef struct tPartitionEntry {
  uint8                 PartitionStatus;  // tPartitionStatus
  tCHSSectorPos         StartSectorCHS;
  uint8                 PartitionType;    // tPartitionType
  tCHSSectorPos         EndSectorCHS;
  uint32                StartSectorLBA;
  uint32                SectorsCount;
} tPartitionEntry;

typedef struct tPartitionSector {
  uchar                 aBootCode[0x1BE];
  tPartitionEntry       aPartitions[4];
  uint16                Signature0xAA55;
} tPartitionSector;

typedef struct tFATBPB1 {
  uint16                BytesPerSector;
  uint8                 SectorsPerCluster;
  uint16                ReservedSectorsCount;
  uint8                 NumberOfFATs;
  uint16                RootEntriesCount;
  uint16                TotalSectorsCount16;
  uint8                 MediaType;
  uint16                SectorsPerFAT1x;
  uint16                SectorsPerTrack;
  uint16                HeadsPerCylinder;
  uint32                HiddenSectorsCount;
  uint32                TotalSectorsCount32;
} tFATBPB1;

typedef union tFATBPB2 {
  struct
  {
  uint8                 DriveNumber;
  uint8                 reserved1;
  uint8                 ExtendedBootSignature;
  uint32                VolumeSerialNumber;
  char                  VolumeLabel[11];
  char                  FileSystemName[8];
  uchar                 aBootCode1x[0x1C];
  } FAT1x;
  struct
  {
  uint32                SectorsPerFAT32;
  uint16                ExtendedFlags;
  uint16                FSVersion;
  uint32                RootDirectoryClusterNo;
  uint16                FSInfoSectorNo;
  uint16                BackupBootSectorNo;
  uint8                 reserved[12];
  uint8                 DriveNumber;
  uint8                 reserved1;
  uint8                 ExtendedBootSignature;
  uint32                VolumeSerialNumber;
  char                  VolumeLabel[11];
  char                  FileSystemName[8];
  } FAT32;
} tFATBPB2;

typedef struct tFATBPB {
  tFATBPB1              BPB1;
  tFATBPB2              BPB2;
} tFATBPB;

typedef struct tFATBootSector {
  uchar                 aJump[3];
  char                  OEMName[8];
  tFATBPB               BPB;
  uchar                 aBootCode32[0x1A4];
  uint16                Signature0xAA55;
} tFATBootSector;

typedef struct tFAT32FSInfoSector {
  uint32                LeadingSignature0x41615252;
  uint8                 reserved1[480];
  uint32                StrucSignature0x61417272;
  uint32                LastKnownFreeClusterCount;
  uint32                FirstClusterToCheckIfFree;
  uint8                 reserved2[12];
  uint32                TrailingSignature0xAA550000;
} tFAT32FSInfoSector;

typedef enum tFATDirEntryAttribute {
  dea_READ_ONLY         = 0x01,
  dea_HIDDEN            = 0x02,
  dea_SYSTEM            = 0x04,
  dea_VOLUME_ID         = 0x08,
  dea_DIRECTORY         = 0x10,
  dea_ARCHIVE           = 0x20,
  dea_LONG_NAME         = dea_READ_ONLY|dea_HIDDEN|dea_SYSTEM|dea_VOLUME_ID
} tFATDirEntryAttribute;

typedef struct tFATDirectoryEntry {
  char                  Name[8];
  char                  Extension[3];
  uint8                 Attribute;
  uint8                 WinNTreserved;
  uint8                 CreationTimeSecTenths;
  uint16                CreationTime2Secs;
  uint16                CreationDate;
  uint16                LastAccessDate;
  uint16                FirstClusterHiWord;
  uint16                LastWriteTime;
  uint16                LastWriteDate;
  uint16                FirstClusterLoWord;
  uint32                Size;
} tFATDirectoryEntry;

#define DELETED_DIR_ENTRY_MARKER        0xE5U

#pragma pack (pop)

/* INTERNAL STRUCTURES FOR DISK I/O */

typedef enum tFATType {
  ft_UNKNOWN,
  ft_FAT12,
  ft_FAT16,
  ft_FAT32
} tFATType;

typedef enum tFATEntryType {
  fet_CLUSTER,                  // not last cluster of file/dir
  fet_LAST_CLUSTER,             // last cluster of file/dir
  fet_FREE_CLUSTER,             // free cluster
  fet_BAD_CLUSTER               // bad cluster
} tFATEntryType;

typedef struct tLogicDiskLocation {
  tPartitionEntry       PartitionEntry;
  uint8                 BIOSDrive;
} tLogicDiskLocation;

typedef struct tLogicDisk {
  int                   Exists;
  tLogicDiskLocation    DiskLocation;
  int                   IsFloppy;
  uint32                VolumeSerialNumber;
  int                   IsValid;
  ulong                 FATErrorsCount;
  ulong                 MiscErrorsCount;
  tFATBPB               BPB;
  tFATType              FATType;
  uint32                FAT1LBA;
  uint32                SectorsPerFAT;
  uint8                 NumberOfFATs;
  uint32                RootDirectoryLBA;
  uint32                DataAreaLBA;
  uint32                DataSectorsCount;
  uint32                ClustersCount;
  uint32                CurrentDirClusterNo;
  uint32                LastKnownFreeClusterCount;
  uint32                FirstClusterToCheckIfFree;

  time_t                LastAccessTime;
} tLogicDisk;

struct tFATFS;

// Cached portioin of a File Allocation Table (FAT)
typedef struct tFATImage {
  struct tFATFS*        pFATFS;
  int                   DiskNo; // signed ???
  uint32                FirstLoadedFATSectorNo;
  uchar                 a3FATSectors[3*FAT_SECTOR_SIZE];
} tFATImage;

// Cached portion of a directory
typedef struct tDirImage {
  struct tFATFS*        pFATFS;
  int                   DiskNo; // signed ???
  uint32                FirstClusterNo;
  uint32                CurrentClusterNo;
  uint32                CurrentSectorNo;
  uchar                 aSector[FAT_SECTOR_SIZE];
} tDirImage;

typedef enum tIORes {
  ior_OK,                       // OK (no error)
  ior_FILE_FOUND,               // OK, file found (can be returned for dir functions)
  ior_DIR_FOUND,                // OK, directory found (can be returned for file functions)
  ior_NOT_FOUND,                // file or directory not found
  ior_NO_MORE_FILES,            // directory read reached end of directory
  ior_BAD_NAME,                 // attempted to find/create file or dir with bad name
  ior_PROHIBITED,               // permission denied, not enough rights, etc
  ior_NO_SPACE,                 // ran out of free disk space
  ior_NO_MEM,                   // run out of RAM or null buffer pointer
  ior_ARGUMENT_OUT_OF_RANGE,    // argument is out of range
  ior_INVALID_ARGUMENT,         // argument is invalid
  ior_ERR_IO,                   // input/output error (read/write/etc)
  ior_ERR_MEDIA_CHANGED,        // media was changed, can't continue operation
  ior_FS_DAMAGED,               // the on-disk FAT structure is damaged/inconsistent
  ior_MEMORY_CORRUPTED          // the in-memory FS structures are corrupted
} tIORes;

typedef struct tReadDirState {
  int                   DiskNo; // signed ???
  uint32                VolumeSerialNumber;
  int                   IsPermanentError;
  tIORes                ErrorCode;
  int                   IsEndReached;
  uint32                FirstClusterNo;
  uint32                CurrentClusterNo;
  uint32                CurrentSectorNo;
  uint16                SectorPosition;
} tReadDirState;

typedef enum tFileOpenMode {
  fom_FAIL_IF_NOT_EXIST   = 0x01,
  fom_CREATE_IF_NOT_EXIST = 0x02,
  fom_TRUNCATE_IF_EXIST   = 0x04,
  fom_OPEN_FOR_WRITE      = 0x08
} tFileOpenMode;

typedef enum tAccessMode {
  am_F_OK       = 0x00, // use am_F_OK exactly or combination of others (XWR)
  am_X_OK       = 0x01, // am_X_OK is ignored (i.e. everything can be executed)
  am_W_OK       = 0x02, // am_W_OK: checks read-only attribute
  am_R_OK       = 0x04  // am_R_OK: every existing file is readable
} tAccessMode;          // for FAT_access()

typedef enum tModeT {
  mt_S_IXUSR    = 0100, // mt_S_IXUSR is ignored
  mt_S_IWUSR    = 0200, // mt_S_IWUSR controls the read-only attribute
  mt_S_IRUSR    = 0400, // mt_S_IRUSR is ignored
  mt_S_IRWXU    = 0700
} tModeT;               // for FAT_mkdir() and FAT_chmod()

typedef enum tFileBufferedDataState {
  fbds_INVALID,
  fbds_VALID,
  fbds_VALID_UNFLUSHED
} tFileBufferedDataState;

typedef struct tFATFile {
  int                   DiskNo; // signed ???
  uint32                VolumeSerialNumber;
  int                   IsOpen;
  tFileOpenMode         OpenMode;
  tFileBufferedDataState BufferedDataState;
  int                   IsTimeUpdateNeeded;
  int                   IsPermanentError;
  tIORes                ErrorCode;
  uint32                FileSize;
  uint32                FilePosition;
  uint32                FirstClusterNo;
  uint32                CurrentClusterNo;       // current when BufferedDataState != fbds_INVALID
  uint32                CurrentSectorNo;        // current when BufferedDataState != fbds_INVALID
  uint16                SectorPosition;         // current when BufferedDataState != fbds_INVALID
  uint32                DirClusterNo;
  tReadDirState         ReadDirState;
  tFATDirectoryEntry    DirectoryEntry;
  // variables to optimize basic file operations
  uint32                LastClusterNo;          // file growing
  uint32                CurClusterOrdinalNo;    // forward file seeking
  // buffered data
  uchar                 aSector[FAT_SECTOR_SIZE];// valid if BufferedDataState != fbds_INVALID
} tFATFile;

typedef struct tFATFileStatus {
  int                   DiskNo; // signed ???
  uint32                FileSize;
  tFATDirEntryAttribute Attribute;
  uint                  Year;
  uint                  Month;
  uint                  Day;
  uint                  Hour;
  uint                  Minute;
  uint                  Second;
} tFATFileStatus;               // for FAT_stat()

typedef struct tFATFSStatus {
  uint32                ClusterSize;
  uint32                TotalClusterCount;
  uint32                TotalFreeClusterCount;
} tFATFSStatus;                 // for FAT_statvfs()

#define MAX_LOGIC_DISKS         26
#define MAX_FAT_IMAGES_AT_ONCE  4
#define MAX_DIR_IMAGES_AT_ONCE  4

typedef int (*tpDoesDriveExistFxn) (struct tFATFS* pFATFS,
                                    uint8 Drive);

typedef tIORes (*tpReadSectorLBAFxn) (struct tFATFS* pFATFS,
                                      tFATBPB* pBPB, uint8 Drive, uint32 LBA,
                                      uint8 SectorCount, void *pBuf);

typedef tIORes (*tpWriteSectorLBAFxn) (struct tFATFS* pFATFS,
                                       tFATBPB* pBPB, uint8 Drive, uint32 LBA,
                                       uint8 SectorCount, const void *pBuf);

typedef void (*tpMessageLogFxn) (struct tFATFS* pFATFS,
                                 const char* format, ...);

typedef void (*tpGetDateAndTime) (struct tFATFS* pFATFS,
                                  uint* pYear, uint* pMonth, uint* pDay,
                                  uint* pHour, uint* pMinute, uint* pSecond);

typedef int (*tpIsFloppyTooLongUnaccessed) (struct tFATFS* pFATFS,
                                            time_t LastTime,
                                            time_t* pNewTime);

typedef int (*tpOpenFilesMatchFxn) (struct tFATFS* pFATFS,
                                    tFATFile* pFATFile,
                                    tReadDirState* pReadDirState);

typedef tFATFile* (*tpOpenFileSearchFxn) (struct tFATFS* pFATFS,
                                          tReadDirState* pReadDirState,
                                          tpOpenFilesMatchFxn pOpenFilesMatch);

typedef struct tFATFS {
  tLogicDisk            aLogicDisks[MAX_LOGIC_DISKS];
  int                   LogicDisksCount;                        // signed !!!

  tCacheEntry           aFATCacheEntries[MAX_FAT_IMAGES_AT_ONCE];
  tFATImage             aFATImages[MAX_FAT_IMAGES_AT_ONCE];

  tCacheEntry           aDirCacheEntries[MAX_DIR_IMAGES_AT_ONCE];
  tDirImage             aDirImages[MAX_DIR_IMAGES_AT_ONCE];

  int                   CurrentLogicDiskNo;                     // signed !!!

  tpDoesDriveExistFxn   pDoesDriveExist;
  tpReadSectorLBAFxn    pReadSectorLBA;
  tpWriteSectorLBAFxn   pWriteSectorLBA;
  tpGetDateAndTime      pGetDateAndTime;
  tpIsFloppyTooLongUnaccessed pIsFloppyTooLongUnaccessed;
  tpMessageLogFxn       pMessageLog;
  tpOpenFileSearchFxn   pOpenFileSearch;
} tFATFS;

extern int IsPrimaryPartition (uint8 PartitionType);
extern int IsExtendedPartition (uint8 PartitionType);
extern const char* GetPartitionTypeString (uint8 PartitionType);
extern const char* GetFileSystemTypeString (tFATType FATType);

extern uint16 GetRootDirSectorsCount (const tFATBPB* pBPB);
extern uint32 GetSectorsPerFAT (const tFATBPB* pBPB);
extern uint8 GetNumberOfFATs (const tFATBPB* pBPB);
extern uint32 GetTotalSectorsCount (const tFATBPB* pBPB);
extern uint32 GetDataSectorsCount (const tFATBPB* pBPB);
extern uint32 GetClustersCount (const tFATBPB* pBPB);
extern uint32 GetFirstFATSectorLBA (const tFATBPB* pBPB);
extern uint32 GetFirstDataSectorLBA (const tFATBPB* pBPB);
extern uint32 GetClusterLBA (const tFATBPB* pBPB, uint32 ClusterNo);
extern uint32 GetRootDirFirstSectorLBA (const tFATBPB* pBPB);
extern int IsRootDirCluster (const tFATBPB* pBPB, uint32 ClusterNo);

extern tFATType GetFATType (const tFATBPB* pBPB);
extern tFATEntryType GetFATEntryType (tFATType FATType, uint32 FATEntryValue);
extern uint32 GetVolumeSerialNumber (const tFATBPB* pBPB);

extern uint32 GetDirEntryFirstClusterNo (const tFATDirectoryEntry* pDirEntry);

extern int IsValid8Dot3Name (const char* pName);
extern char* DirEntryNameToASCIIZ (const tFATDirectoryEntry* pDirEntry,
                                   char* pBuf);

extern int FAT_Init (tFATFS* pFATFS,
                     const uint8* pBIOSDrivesToScan,
                     uint NumberOfBIOSDrivesToScan,
                     tpDoesDriveExistFxn pDoesDriveExist,
                     tpReadSectorLBAFxn pReadSectorLBA,
                     tpWriteSectorLBAFxn pWriteSectorLBA,
                     tpGetDateAndTime pGetDateAndTime,
                     tpIsFloppyTooLongUnaccessed pIsFloppyTooLongUnaccessed,
                     tpOpenFileSearchFxn pOpenFileSearch,
                     tpMessageLogFxn pMessageLog,
                     int FATBootDiskHint);

extern int FAT_Flush (tFATFS* pFATFS);

extern int FAT_Done (tFATFS* pFATFS);

extern int FAT_OpenFilesMatch (tFATFS* pFATFS, tFATFile* pFATFile,
                               tReadDirState* pReadDirState);

extern char*      FAT_getcwd (tFATFS* pFATFS, tIORes* pIORes,
                              char* pBuf, size_t BufLen);

extern int        FAT_chdir  (tFATFS* pFATFS, tIORes* pIORes,
                              const char* pPath, int IsDiskChangeAllowed);

extern void       FAT_dos_setdrive (tFATFS* pFATFS, tIORes* pIORes,
                                    uint DiskNo, uint* pTotalDisks);

extern void       FAT_dos_getdrive (tFATFS* pFATFS, tIORes* pIORes,
                                    uint* pDiskNo);

extern tFATFile*  FAT_fopen  (tFATFS* pFATFS, tIORes* pIORes,
                              tFATFile* pFATFile, const char* filename,
                              const char* mode);

extern int        FAT_fclose (tFATFS* pFATFS, tIORes* pIORes,
                              tFATFile* pFATFile);

extern int        FAT_fseek  (tFATFS* pFATFS, tIORes* pIORes,
                              tFATFile* pFATFile, uint32 Position);

extern void       FAT_rewind (tFATFS* pFATFS, tIORes* pIORes,
                              tFATFile* pFATFile);

extern int        FAT_ftell  (tFATFS* pFATFS, tIORes* pIORes,
                              tFATFile* pFATFile, uint32* pPosition);

extern int        FAT_feof   (tFATFS* pFATFS, tIORes* pIORes,
                              tFATFile* pFATFile, int* pIsEOF);

extern int        FAT_fsize  (tFATFS* pFATFS, tIORes* pIORes,
                              tFATFile* pFATFile, uint32* pSize);

extern size_t     FAT_fread  (tFATFS* pFATFS, tIORes* pIORes,
                              void *ptr, size_t size, size_t n,
                              tFATFile* pFATFile);

extern size_t     FAT_fwrite (tFATFS* pFATFS, tIORes* pIORes,
                              const void* ptr, size_t size, size_t n,
                              tFATFile* pFATFile);

extern int        FAT_fflush (tFATFS* pFATFS, tIORes* pIORes,
                              tFATFile* pFATFile);

extern int        FAT_unlink (tFATFS* pFATFS, tIORes* pIORes,
                              const char* filename);

extern int        FAT_opendir (tFATFS* pFATFS, tIORes* pIORes,
                               const char* pPath,
                               tReadDirState* pReadDirState);

extern int        FAT_closedir (tFATFS* pFATFS, tIORes* pIORes,
                                tReadDirState* pReadDirState);

extern int        FAT_rewinddir (tFATFS* pFATFS, tIORes* pIORes,
                                 tReadDirState* pReadDirState);

extern int        FAT_readdir (tFATFS* pFATFS, tIORes* pIORes,
                               tReadDirState* pReadDirState,
                               tFATDirectoryEntry* pDirEntry);

extern int        FAT_mkdir (tFATFS* pFATFS, tIORes* pIORes,
                             const char* pPath, tModeT mode);

extern int        FAT_rmdir (tFATFS* pFATFS, tIORes* pIORes,
                             const char* pPath);

extern int        FAT_access (tFATFS* pFATFS, tIORes* pIORes,
                              const char *filename, tAccessMode amode);

extern int        FAT_stat (tFATFS* pFATFS, tIORes* pIORes,
                            const char *path, tFATFileStatus* pStatus);

extern int        FAT_chmod (tFATFS* pFATFS, tIORes* pIORes,
                             const char *path, tModeT mode);

extern int        FAT_utime (tFATFS* pFATFS, tIORes* pIORes,
                             const char *path,
                             uint Year, uint Month, uint Day,
                             uint Hour, uint Minute, uint Second);

extern int        FAT_statvfs (tFATFS* pFATFS, tIORes* pIORes,
                               const char *path,
                               tFATFSStatus* pStatus);

#ifdef __cplusplus
}
#endif

#endif // _FAT_H_
