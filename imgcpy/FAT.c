/*******************************************************************************
   TITLE:       Main FAT File System module
   AUTHOR:      Alexei A. Frounze
   DATES:       April 2004 ... December 2006
   LICENSE/(c): free for non-commercial purposes,
                no warranties on operation of any sort given,
                initial authorship must be retained

   FEATURES:
   - support for MSDOS primary/extended partitions and FAT12, FAT16 FS
   - support for MSWindows primary/extended partitions and FAT16, FAT32 FS

   LIMITATIONS:
   - opening files for append (as in fopen("fname", "ab")) isn't supported
   - volume IDs cannot be open for read/write (stat doesn't return them)
   - directories aren't shrunk to smaller size when files and
     subdirectories are deleted from them
   - special filenames reserved for devices AREN'T disallowed:
     CON, PRN, AUX, CLOCK$, NUL, COM1 thru COM9, LPT1 thru LPT9
   - long file names introduced in MSWindows are NOT supported
   - long file names are NOT deleted if the corresponding file/dir is
     being deleted
   - CHS values from the MBRs AREN'T used at all. The code relies only on the
     LBA values in the MBRs. However, all the LBA values are then converted
     down to CHS if the HDD does not support LBA mode and only has CHS.
     This should be no problem, unless the LBA values in the MBRs are wrong
     (e.g.all zeroes)
   - if an error is encountered while reading/writing a file/dir, it gets stuck,
     e.g. no further operation can be done on it (an attempt to make
     sure the errors don't propagate and add up)
   - if the FAT is damaged prior to using this code on it, more damage
     can be caused to the FAT if anything is written to it using this code,
     the same holds for damaged media accessing which gives I/O errors
   - no other language than English supported fully in file/dir names
     (the code doesn't know what is upper/lower case of a non-English character)
   - FAT32 limitations:
     - version 0.0 supported only (others don't seem to exist yet)
     - mirroring to all FAT copies supported only, no FAT copy selection
       (I've never seen the selective mode in use)
   - general FAT limitations:
     - 512-bytes-long sectors in FAT supported only!
     - 1 or 2 FAT copies are supported - should be OK
   - little-endian CPU (with char=8 bits,short=16 bits,long=32 bits)
     target supported only!
 
   DONE / TO TEST:
   + Propagate detected errors (including I/O errors) up
   + Implement permanent errors, when no operation on the open file/dir
     can continue (e.g. after media change)
   + logical disk ordering (disk letter assignment) DOS/Windows-like
   + media change checks for removable disks
 
   + fopen(),fclose(),fseek(),rewind(),ftell(),feof(),fread(),fwrite(),
     fsize(),fflush(),remove()/unlink()
   + getcwd(),chdir(),opendir(),closedir(),readdir(),rewinddir(),mkdir(),rmdir()
   + handle extra trailing dot in file/dir name search (like myfile. & mydir.\)
 
   + move position advancing code out of ReadDirEntry() and make it optional
   + adapt FindDirectoryEntry() to return info useful in file/dir creation
 
   + implement "get system time & date" callback
   + add time & date updating when writing files

   + implement a callback to make sure the same file isn't being
     opened (or operated on) again before it's closed
   + access(), stat()
   + chmod(), utime()

   + fix the cache problem with removable disks
   + use FAT32 FS Info Sector; currently, it's completely ignored

   + fix major performance issue/scalability bug (file copying time was
     quadratic function of file size)

   + statvfs()

   + come up with some standard tests, at 1st perform on a set of floppy images

   + move out the list of BIOS drives from the init fxn to the caller,
     floppy drives should also be put into the parameter array

   + extra validation of FS upon mounting

   TO DO:
   - rename() (check names)
   - check the correctness of the FAT & dir entry caches
   - add functions for removable disks other than floppies
     (like USB flash, which can be plugged/mounted and unplugged/unmounted
     on the go)
   - try using another cluster if one is bad, mark as bad

   Special issues:
   + multiple opens of same file: allow only multiple reads
     (file being open for writing can't be open for anything,
     file being open for reading can't be open for writing)
   - buffering & caching:
     - think of caching tracks on floppies
     - think of better file buffering (more than 1 sector) and read ahead/back,
       incapsulate file info needed for buffering into substructure/object
   - multithreading issues:
     - global FAT and directory caches: RMW operations must be atomic
     - Current disk/directory may be different for different applications (local
       vs global)
   - removable/installable drives
   - anything to deal with file's dir entry size and cluster chain size mismatch?
   - anything to deal with cycles in the cluster chains?
   - anything to deal with shared clusters?
   - anything to deal with errors during postponed writes?
   - anything to deal with mismatching FAT copies?
*******************************************************************************/

#include "FAT.h"
#include <string.h>
#include <stdio.h>
#include <limits.h>

// FAT32 writing can be disabled for reliability reasons
// (e.g. if someone runs this code on the real (not emulated) PC
// and wants to be sure nothing will happen to their disk data):
#define WRITING_TO_FAT32_ENABLED 01

//#define ERROR_LOGGING_ENABLED 0

#if ERROR_LOGGING_ENABLED

#define LOG_ERR0() \
  pFATFS->pMessageLog (pFATFS, "@" __FILE__ ":%u\n", (uint)__LINE__);

#define LOG_ERR1(__ERRMSG__) \
  pFATFS->pMessageLog (pFATFS, __ERRMSG__ "@" __FILE__ ":%u\n", (uint)__LINE__);

#define LOG_ERR2(__ERRMSG__, __ERRARG0__)                               \
  pFATFS->pMessageLog (pFATFS, __ERRMSG__ "@" __FILE__ ":%u\n",         \
  __ERRARG0__,                                                          \
  (uint)__LINE__);

#define LOG_ERR3(__ERRMSG__, __ERRARG0__, __ERRARG1__)                  \
  pFATFS->pMessageLog (pFATFS, __ERRMSG__ "@" __FILE__ ":%u\n",         \
  __ERRARG0__, __ERRARG1__,                                             \
  (uint)__LINE__);

#define LOG_ERR4(__ERRMSG__, __ERRARG0__, __ERRARG1__, __ERRARG2__)     \
  pFATFS->pMessageLog (pFATFS, __ERRMSG__ "@" __FILE__ ":%u\n",         \
  __ERRARG0__, __ERRARG1__, __ERRARG2__,                                \
  (uint)__LINE__);

#else // if ERROR_LOGGING_ENABLED

#define LOG_ERR0()

#define LOG_ERR1(__ERRMSG__)

#define LOG_ERR2(__ERRMSG__, __ERRARG0__)

#define LOG_ERR3(__ERRMSG__, __ERRARG0__, __ERRARG1__)

#define LOG_ERR4(__ERRMSG__, __ERRARG0__, __ERRARG1__, __ERRARG2__)

#endif // if ERROR_LOGGING_ENABLED

static void FAT_OnMediaHasChanged (tFATFS* pFATFS, uint8 BIOSDrive);

int IsPrimaryPartition (uint8 PartitionType)
{
  switch (PartitionType)
  {
    case pt_FAT12_PARTITION:
    case pt_FAT16_UNDER_32MB_PARTITION:
    case pt_FAT16_OVER_32MB_PARTITION:
    case pt_FAT32_PARTITION:
    case pt_FAT32_LBA_PARTITION:
    case pt_FAT16_OVER_32MB_LBA_PARTITION:
      return 1;
    default:
      return 0;
  }
}

int IsExtendedPartition (uint8 PartitionType)
{
  switch (PartitionType)
  {
    case pt_EXTENDED_PARTITION:
    case pt_EXTENDED_LBA_PARTITION:
      return 1;
    default:
      return 0;
  }
}

typedef struct tTypeAndString {
  uint8 Type;
  char* pString;
} tTypeAndString;

#if 0
static const tTypeAndString aPartitionTypeAndString[] =
{
  {pt_UNKNOWN_PARTITION            , "UNKNOWN PARTITION"},
  {pt_FAT12_PARTITION              , "DOS FAT12 PARTITION"},
  {pt_FAT16_UNDER_32MB_PARTITION   , "DOS FAT16 (<32MB) PARTITION"},
  {pt_EXTENDED_PARTITION           , "DOS EXTENDED PARTITION"},
  {pt_FAT16_OVER_32MB_PARTITION    , "DOS FAT16 (>=32MB) PARTITION"},
  {pt_FAT32_PARTITION              , "WIN FAT32 PARTITION"},
  {pt_FAT32_LBA_PARTITION          , "WIN FAT32 (LBA) PARTITION"},
  {pt_FAT16_OVER_32MB_LBA_PARTITION, "WIN FAT16 (>=32MB,LBA) PARTITION"},
  {pt_EXTENDED_LBA_PARTITION       , "WIN EXTENDED (LBA) PARTITION"}
};

const char* GetPartitionTypeString (uint8 PartitionType)
{
  int i;
  for (i=0; i<GetArrayElementCount(aPartitionTypeAndString); i++)
    if (aPartitionTypeAndString[i].Type == PartitionType)
      return aPartitionTypeAndString[i].pString;
  return GetPartitionTypeString (pt_UNKNOWN_PARTITION);
}

static const tTypeAndString aFileSystemTypeAndString[] =
{
  {ft_UNKNOWN,  "UNKNOWN"},
  {ft_FAT12,    "FAT12"},
  {ft_FAT16,    "FAT16"},
  {ft_FAT32,    "FAT32"}
};

const char* GetFileSystemTypeString (tFATType FATType)
{
  int i;
  for (i=0; i<GetArrayElementCount(aFileSystemTypeAndString); i++)
    if (aFileSystemTypeAndString[i].Type == FATType)
      return aFileSystemTypeAndString[i].pString;
  return GetFileSystemTypeString (ft_UNKNOWN);
}
#endif

// nonzero for FAT 1x only:
uint16 GetRootDirSectorsCount (const tFATBPB* pBPB)
{
  uint32 nom = pBPB->BPB1.BytesPerSector - 1 +
    ((uint32)pBPB->BPB1.RootEntriesCount * sizeof(tFATDirectoryEntry));

  // avoid division overflow and division by 0 if wrong arguments
  if ((nom >> (sizeof(pBPB->BPB1.BytesPerSector)*CHAR_BIT)) >=
      pBPB->BPB1.BytesPerSector)
    return 0;

  return nom / pBPB->BPB1.BytesPerSector;
}

uint32 GetSectorsPerFAT (const tFATBPB* pBPB)
{
  if (pBPB->BPB1.SectorsPerFAT1x)
    return pBPB->BPB1.SectorsPerFAT1x;
  else
    return pBPB->BPB2.FAT32.SectorsPerFAT32;
}

uint8 GetNumberOfFATs (const tFATBPB* pBPB)
{
  return pBPB->BPB1.NumberOfFATs;
}

uint32 GetTotalSectorsCount (const tFATBPB* pBPB)
{
  if (pBPB->BPB1.TotalSectorsCount16)
    return pBPB->BPB1.TotalSectorsCount16;
  else
    return pBPB->BPB1.TotalSectorsCount32;
}

uint32 GetDataSectorsCount (const tFATBPB* pBPB)
{
  uint16 RootDirSectorsCount;
  uint32 SectorsPerFAT;
  uint32 TotalSectorsCount;

  RootDirSectorsCount = GetRootDirSectorsCount (pBPB);
  SectorsPerFAT = GetSectorsPerFAT (pBPB);
  TotalSectorsCount = GetTotalSectorsCount (pBPB);

  return TotalSectorsCount - (pBPB->BPB1.ReservedSectorsCount +
    pBPB->BPB1.NumberOfFATs * SectorsPerFAT + RootDirSectorsCount);
}

uint32 GetClustersCount (const tFATBPB* pBPB)
{
  uint32 nom = GetDataSectorsCount (pBPB);

  // avoid division overflow and division by 0 if wrong arguments
  if ((nom >> (sizeof(pBPB->BPB1.BytesPerSector)*CHAR_BIT)) >=
      pBPB->BPB1.BytesPerSector)
    return 0;

  return nom / pBPB->BPB1.SectorsPerCluster;
}

uint32 GetFirstFATSectorLBA (const tFATBPB* pBPB)
{
  return pBPB->BPB1.ReservedSectorsCount;
}

uint32 GetFirstDataSectorLBA (const tFATBPB* pBPB)
{
  return pBPB->BPB1.ReservedSectorsCount +
    GetSectorsPerFAT (pBPB) * pBPB->BPB1.NumberOfFATs +
    GetRootDirSectorsCount (pBPB);
}

uint32 GetClusterLBA (const tFATBPB* pBPB, uint32 ClusterNo)
{
  return (ClusterNo - 2) * pBPB->BPB1.SectorsPerCluster +
    GetFirstDataSectorLBA (pBPB);
}

uint32 GetRootDirClusterNo (const tFATBPB* pBPB)
{
  // zero for FAT 1x:
  if (pBPB->BPB1.RootEntriesCount)
    return 0;
  // nonzero for FAT 32:
  else
    return pBPB->BPB2.FAT32.RootDirectoryClusterNo;
}

uint32 GetRootDirFirstSectorLBA (const tFATBPB* pBPB)
{
  if (pBPB->BPB1.RootEntriesCount)
    return pBPB->BPB1.ReservedSectorsCount +
      GetSectorsPerFAT (pBPB) * pBPB->BPB1.NumberOfFATs;
  else
    return GetClusterLBA (pBPB, pBPB->BPB2.FAT32.RootDirectoryClusterNo);
}

int IsRootDirCluster (const tFATBPB* pBPB, uint32 ClusterNo)
{
  return
  (
    // 0 is FAT 1x root dir cluster no (let's treat FAT 32 this way as well):
    (!ClusterNo) ||
    // let's check whether it's FAT32 and cluster no is really of a root dir:
    (
      (!pBPB->BPB1.RootEntriesCount) &&
      (pBPB->BPB2.FAT32.RootDirectoryClusterNo == ClusterNo)
    )
  );
}

tFATType GetFATType (const tFATBPB* pBPB)
{
  uint32 ClusterCount;
  ClusterCount = GetClustersCount (pBPB);
  if (ClusterCount < 0xFF5)
    return ft_FAT12;
  else if (ClusterCount < 0xFFF5UL)
    return ft_FAT16;
  else
    return ft_FAT32;
}

tFATEntryType GetFATEntryType (tFATType FATType, uint32 FATEntryValue)
{
  switch (FATType)
  {
    default:
    case ft_UNKNOWN:
      return fet_BAD_CLUSTER;

    case ft_FAT12:
      FATEntryValue &= 0xFFF;
      if (FATEntryValue >= 0xFF8)
        return fet_LAST_CLUSTER;
      if (FATEntryValue == 0xFF7)
        return fet_BAD_CLUSTER;
    break;

    case ft_FAT16:
      FATEntryValue &= 0xFFFFUL;
      if (FATEntryValue >= 0xFFF8UL)
        return fet_LAST_CLUSTER;
      if (FATEntryValue == 0xFFF7UL)
        return fet_BAD_CLUSTER;
    break;

    case ft_FAT32:
      FATEntryValue &= 0xFFFFFFFUL;
      if (FATEntryValue >= 0xFFFFFF8UL)
        return fet_LAST_CLUSTER;
      if (FATEntryValue == 0xFFFFFF7UL)
        return fet_BAD_CLUSTER;
    break;
  }

  if (!FATEntryValue)
    return fet_FREE_CLUSTER;
  else
    return fet_CLUSTER;
}

uint32 GetLastClusterMarkValue (tFATType FATType)
{
  switch (FATType)
  {
    default:
    case ft_UNKNOWN:
      return 0;

    case ft_FAT12:
      return 0xFFF;

    case ft_FAT16:
      return 0xFFFFU;

    case ft_FAT32:
      return 0xFFFFFFFUL;
  }
}

uint32 GetVolumeSerialNumber (const tFATBPB* pBPB)
{
  switch (GetFATType (pBPB))
  {
    case ft_FAT12:
    case ft_FAT16:
      return pBPB->BPB2.FAT1x.VolumeSerialNumber;
    case ft_FAT32:
      return pBPB->BPB2.FAT32.VolumeSerialNumber;
    case ft_UNKNOWN:
    default:
      return 0xFFFFFFFFUL;
  }
}

uint32 GetDirEntryFirstClusterNo (const tFATDirectoryEntry* pDirEntry)
{
  uint32 res;
  res = pDirEntry->FirstClusterHiWord;
  res <<= 16;
  res |= pDirEntry->FirstClusterLoWord;
  return res;
}

void SetDirEntryFirstClusterNo (tFATDirectoryEntry* pDirEntry, uint32 ClusterNo)
{
  pDirEntry->FirstClusterHiWord = (ClusterNo >> 16) & 0xFFFFu;
  pDirEntry->FirstClusterLoWord =  ClusterNo        & 0xFFFFu;
}

void SetDirEntryWriteDateAndTime (tFATDirectoryEntry* pDirEntry,
                                  uint Year, uint Month, uint Day,
                                  uint Hour, uint Minute, uint Second)
{
  pDirEntry->LastWriteTime = ((Hour&0x1f)<<11) | ((Minute&0x3f)<<5) |
                             ((Second&0x3f)>>1);
  pDirEntry->LastWriteDate = (((Year-1980)&0x7f)<<9) | ((Month&0xf)<<5) |
                             (Day&0x1f);
}

void GetDirEntryWriteDateAndTime (const tFATDirectoryEntry* pDirEntry,
                                  uint* pYear, uint* pMonth, uint* pDay,
                                  uint* pHour, uint* pMinute, uint* pSecond)
{
  *pYear   = 1980 + ((pDirEntry->LastWriteDate >> 9) & 0x7f);
  *pMonth  = ((pDirEntry->LastWriteDate >>  5) & 0x0f);
  *pDay    =   pDirEntry->LastWriteDate        & 0x1f;
  *pHour   = ((pDirEntry->LastWriteTime >> 11) & 0x1f);
  *pMinute = ((pDirEntry->LastWriteTime >>  5) & 0x3f);
  *pSecond =  (pDirEntry->LastWriteTime        & 0x1f) << 1;
}

static int IsPermanentError (tIORes Res)
{
  switch (Res)
  {
    case ior_ERR_IO:            // input/output error (read/write/etc)
    case ior_ERR_MEDIA_CHANGED: // media was changed, can't continue operation
    case ior_FS_DAMAGED:
    case ior_MEMORY_CORRUPTED:
      return 1;
    default:
      return 0;
  }
}

static const uchar aInvalid8Dot3NameChars[] = "\"*+,./:;<=>?[\\]|";

static char upcase (char c)
{
  if ((c >= 'a') && (c <= 'z'))
    c -= 'a' - 'A';
  return c;
}

// IsValid8Dot3Name() returns 0 for "." and ".."
int IsValid8Dot3Name (const char* pName)
{
  int i, j;
  int namelen=0, dots=0, extlen=0;

  if (pName == NULL)
    return 0;

  for (i=0; pName[i]; i++)
  {
    if (i > 12)                         // 8.3 name is too long
      return 0;
    if (pName[i] == '.')
      {  dots++; continue;  }
    if (!dots) namelen++;
    else extlen++;
    if (pName[i] == 0x05)               // pName[0]==0x05 is a valid 0xE5 KANJI char
      {  if (!i) continue; else return 0;  }
    if
    (
      (!i) &&
      ((uchar)pName[i] == DELETED_DIR_ENTRY_MARKER)
    )
      return 0;
    if ((uchar)pName[i] <= 0x20)        // invalid chars
      return 0;
    for (j=0; j<GetArrayElementCount(aInvalid8Dot3NameChars); j++)
      if ((uchar)pName[i] == aInvalid8Dot3NameChars[j]) // invalid chars
        return 0;
  }

  if
  (
    (dots > 1) ||
    (!namelen) ||
    (namelen > 8) ||
    (extlen > 3)
  )
    return 0;

  return 1;
}

// same as IsValid8Dot3Name() but allows a trailing slash:
static int IsValid8Dot3NameAndSlash (const char* pName)
{
  char s[13];
  uint l;

  if (((l = strlen (pName)) > 13) || !l)
    return 0;

  if (pName[l-1] == '\\')
  {
    if (!--l)
      return 0;
    memcpy (s, pName, l);
    s[l] = 0;
    return IsValid8Dot3Name (s);
  }

  return IsValid8Dot3Name (pName);
}

static int _8Dot3NameToDirEntryName (const char* pName,
                                     tFATDirectoryEntry* pDirEntry, 
                                     int IsTrailingSlashAllowed)
{
  int i, j, dots=0;

  if (IsTrailingSlashAllowed)
  {
    if (!IsValid8Dot3NameAndSlash (pName))
      return 0;
  }
  else
  {
    if (!IsValid8Dot3Name (pName))
      return 0;
  }

  memset (pDirEntry->Name, ' ', 8);
  memset (pDirEntry->Extension, ' ', 3);

  for (j=i=0; pName[i]; i++)
  {
    if (pName[i] == '\\')
      break;
    if (pName[i] == '.')
    {
      dots++;
      j = 0;
      continue;
    }
    if (!dots)
      pDirEntry->Name[j++] = upcase (pName[i]);
    else
      pDirEntry->Extension[j++] = upcase (pName[i]);
  }

  return 1;
}

char* DirEntryNameToASCIIZ (const tFATDirectoryEntry* pDirEntry, char* pBuf)
{
  char *p = pBuf;
  int i;

  if (pBuf != NULL)
    *pBuf = 0;

  if ((pBuf == NULL) || (pDirEntry == NULL))
    return pBuf;

  // Check for end of directory:
  if (!pDirEntry->Name[0])
    return pBuf;

  // Skip deleted files:
  if ((uchar)pDirEntry->Name[0] == DELETED_DIR_ENTRY_MARKER)
    return pBuf;

  for (i=0; i<8; i++)
    if (pDirEntry->Name[i] != ' ') *p++ = pDirEntry->Name[i];
    else break;

  if (pDirEntry->Extension[0] != ' ') *p++ = '.';

  for (i=0; i<3; i++)
    if (pDirEntry->Extension[i] != ' ') *p++ = pDirEntry->Extension[i];
    else break;

  *p = 0;

  return pBuf;
}

static void DummyMessageLog (tFATFS* pFATFS, const char* format, ...)
{
  (void)pFATFS;
  (void)format;
}

// AddDiskPartition() is called only from FAT_Init()
static int AddDiskPartition (tFATFS* pFATFS, uint8 BIOSDrive,
                             tPartitionEntry* pPartitionEntry)
{
  tLogicDisk* pLogicDisk;

  // HDD partitions are added before floppy drives, hence
  // the following use of the count variable is OK
  if (pFATFS->LogicDisksCount >= MAX_LOGIC_DISKS-2)
  {
    LOG_ERR0();
    return 0;
  }

  pLogicDisk = &pFATFS->aLogicDisks[2+pFATFS->LogicDisksCount++];

  pLogicDisk->Exists = 1;
  pLogicDisk->DiskLocation.PartitionEntry = *pPartitionEntry;
  pLogicDisk->DiskLocation.BIOSDrive = BIOSDrive;

  return 1;
}

// IsPartitionAlreadyAdded() is called only from FAT_Init()
static int IsPartitionAlreadyAdded (tFATFS* pFATFS, uint8 BIOSDrive,
                                    tPartitionEntry* pPartitionEntry)
{
  tLogicDisk* pLogicDisk;
  int res = 0;

  for (pLogicDisk = &pFATFS->aLogicDisks[2];
       pLogicDisk < &pFATFS->aLogicDisks[MAX_LOGIC_DISKS];
       pLogicDisk++)
  {
    tPartitionEntry* pLogicDiskPartitionEntry =
      &pLogicDisk->DiskLocation.PartitionEntry;
    if (!pLogicDisk->Exists)
      continue;
    if
    (
      (pLogicDisk->DiskLocation.BIOSDrive == BIOSDrive) &&
      (pLogicDiskPartitionEntry->PartitionStatus ==
                pPartitionEntry->PartitionStatus) &&
      (pLogicDiskPartitionEntry->StartSectorCHS.Head ==
                pPartitionEntry->StartSectorCHS.Head) &&
      (pLogicDiskPartitionEntry->StartSectorCHS.PackedCylSec ==
                pPartitionEntry->StartSectorCHS.PackedCylSec) &&
      (pLogicDiskPartitionEntry->PartitionType ==
                pPartitionEntry->PartitionType) &&
      (pLogicDiskPartitionEntry->EndSectorCHS.Head ==
                pPartitionEntry->EndSectorCHS.Head) &&
      (pLogicDiskPartitionEntry->EndSectorCHS.PackedCylSec ==
                pPartitionEntry->EndSectorCHS.PackedCylSec) &&
      (pLogicDiskPartitionEntry->StartSectorLBA ==
                pPartitionEntry->StartSectorLBA) &&
      (pLogicDiskPartitionEntry->SectorsCount ==
                pPartitionEntry->SectorsCount)
    )
    {
      res = 1;
      break;
    }
  }

  return res;
}

static void InitDiskFromBPB (tFATFS* pFATFS, tLogicDisk* pLogicDisk,
                             tFATBPB* pBPB)
{
/*
  // Should be already initialized:
  pLogicDisk->Exists
  pLogicDisk->DiskLocation
  pLogicDisk->IsFloppy
*/
  pLogicDisk->IsValid = 0;
  pLogicDisk->FATErrorsCount = 0;
  pLogicDisk->MiscErrorsCount = 0;
  pLogicDisk->BPB = *pBPB;

  // Validate BPB

  // BytesPerSector must be 512 through 4096, we require it to be 512 always:
  if (pBPB->BPB1.BytesPerSector != FAT_SECTOR_SIZE)
    goto lend;

  // SectorsPerCluster must be a power of 2 and be equal 1 through 128:
  if ((pBPB->BPB1.SectorsPerCluster < 1) ||
      (pBPB->BPB1.SectorsPerCluster > 128) ||
      (pBPB->BPB1.SectorsPerCluster & (pBPB->BPB1.SectorsPerCluster - 1)))
    goto lend;

  // Bytes per cluster must be <= 32768:
  if ((ulong)pBPB->BPB1.BytesPerSector * pBPB->BPB1.SectorsPerCluster > 32768UL)
    goto lend;

  // ReservedSectorsCount must not be 0 as among the reserved sectors
  // there's always at least one, the boot sector (usually 1 for FAT1x):
  if (!pBPB->BPB1.ReservedSectorsCount)
    goto lend;

  // NumberOfFATs must be 2 or 1:
  if ((pBPB->BPB1.NumberOfFATs != 2) && (pBPB->BPB1.NumberOfFATs != 1))
    goto lend;

  // RootEntriesCount * 32 must be a multiple of BytesPerSector:
  if (((uint32)pBPB->BPB1.RootEntriesCount * sizeof(tFATDirectoryEntry)) %
      FAT_SECTOR_SIZE)
    goto lend;

  // One of TotalSectorsCount16 and TotalSectorsCount32 must be zero
  // while the other of the two must be non-zero:
  if ((!(pBPB->BPB1.TotalSectorsCount16 || pBPB->BPB1.TotalSectorsCount32)) ||
      (pBPB->BPB1.TotalSectorsCount16 && pBPB->BPB1.TotalSectorsCount32))
    goto lend;

  // MediaType must be one of: 0xF0 (often for removable), 0xF8 (for fixed),
  // 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, and 0xFF:
  if ((pBPB->BPB1.MediaType != 0xF0) &&
      (pBPB->BPB1.MediaType < 0xF8))
    goto lend;

#if 0
  // SectorsPerTrack and HeadsPerCylinder should make sense,
  // at least should be non-zero:
  if ((!pBPB->BPB1.SectorsPerTrack) || (!pBPB->BPB1.HeadsPerCylinder))
    goto lend;
#endif

  // On FAT1x RootEntriesCount and SectorsPerFAT1x must be non-zero,
  // on FAT32 RootEntriesCount and SectorsPerFAT1x must be 0 and
  // SectorsPerFAT32 must be non-zero
  if (!(
         (pBPB->BPB1.RootEntriesCount && pBPB->BPB1.SectorsPerFAT1x) ||
         (
           (!pBPB->BPB1.RootEntriesCount) &&
           (!pBPB->BPB1.SectorsPerFAT1x) &&
           pBPB->BPB2.FAT32.SectorsPerFAT32
         )
       ))
    goto lend;

  // Assuming RootEntriesCount distinguishes between FAT1x and FAT32,
  // validate other things:
  if (pBPB->BPB1.RootEntriesCount)
  {
    // ExtendedBootSignature must be 0x29:
    if (pBPB->BPB2.FAT1x.ExtendedBootSignature != 0x29)
      goto lend;

    // There're at most 0xFFF7 elements in FAT16's FAT, each occupying
    // 16 bits, checking this:
    if (GetSectorsPerFAT (pBPB) >
        (0xFFF7UL * sizeof(uint16) + FAT_SECTOR_SIZE - 1) / FAT_SECTOR_SIZE)
      goto lend;
  }
  else
  {
    // ExtendedBootSignature must be 0x29:
    if (pBPB->BPB2.FAT32.ExtendedBootSignature != 0x29)
      goto lend;

    // There're at most 0xFFFFFF7 elements in FAT32's FAT, each occupying
    // 32 bits, checking this:
    if (GetSectorsPerFAT (pBPB) >
        (0xFFFFFF7UL * sizeof(uint32) + FAT_SECTOR_SIZE - 1) / FAT_SECTOR_SIZE)
      goto lend;

    // FAT32 version 0.0 is supported only:
    if (pBPB->BPB2.FAT32.FSVersion != 0x0000U)
      goto lend;

    // Currently we only support mirroring to all FAT32 FATs:
    if (pBPB->BPB2.FAT32.ExtendedFlags & 0x0080U)
      goto lend;

    // RootDirectoryClusterNo must be in the valid range (usually 2):
    if ((pBPB->BPB2.FAT32.RootDirectoryClusterNo < 2) ||
        (pBPB->BPB2.FAT32.RootDirectoryClusterNo >= 0xFFFFFF7UL))
      goto lend;

    // Validate FAT32 FS Info sector location in FSInfoSectorNo (usually 1):
    if ((!pBPB->BPB2.FAT32.FSInfoSectorNo) ||
        (pBPB->BPB2.FAT32.FSInfoSectorNo >= pBPB->BPB1.ReservedSectorsCount))
      goto lend;

    // Validate location of the boot sector and FAT32 FS Info sector back ups
    // if available:
    if (pBPB->BPB2.FAT32.BackupBootSectorNo)    // usually 6
    {
      // The boot sector and FAT32 FS Info sector should come together,
      // and so should their back ups:
      if (pBPB->BPB2.FAT32.BackupBootSectorNo <=
          pBPB->BPB2.FAT32.FSInfoSectorNo)
        goto lend;

      if ((uint32)pBPB->BPB2.FAT32.BackupBootSectorNo +
          pBPB->BPB2.FAT32.FSInfoSectorNo >= pBPB->BPB1.ReservedSectorsCount)
        goto lend;
    }
  }

  // Validate the total number of sectors:
  {
    uint16 RootDirSectorsCount;
    uint32 SectorsPerFAT;
    uint32 TotalSectorsCount;
    uint32 DataSectorsCount;

    RootDirSectorsCount = GetRootDirSectorsCount (pBPB);
    SectorsPerFAT = GetSectorsPerFAT (pBPB);
    TotalSectorsCount = GetTotalSectorsCount (pBPB);

    // Calculate number of sectors preceding the data/cluster sectors:
    DataSectorsCount = pBPB->BPB1.ReservedSectorsCount +
        pBPB->BPB1.NumberOfFATs * SectorsPerFAT + RootDirSectorsCount;

    // Total sectors count must be greater than reserved sectors count +
    // fat sectors count + root directory sectors count:
    if (TotalSectorsCount <= DataSectorsCount)
      goto lend;

    // There must be enough space for at least one cluster:
    DataSectorsCount = TotalSectorsCount - DataSectorsCount;
    if (DataSectorsCount <
        (ulong)pBPB->BPB1.BytesPerSector * pBPB->BPB1.SectorsPerCluster)
      goto lend;

    // Assuming RootEntriesCount distinguishes between FAT1x and FAT32,
    // validate FAT32's root directory cluster number:
    if (!pBPB->BPB1.RootEntriesCount)
    {
      uint32 ClustersCount = DataSectorsCount / pBPB->BPB1.SectorsPerCluster;
      if (pBPB->BPB2.FAT32.RootDirectoryClusterNo - 2 >=
          ClustersCount)
        goto lend;
    }
  }

  pLogicDisk->IsValid = 1;

  // Calculate/copy some handy values from the BPB contents and store in
  // the disk structure:

  pLogicDisk->VolumeSerialNumber = GetVolumeSerialNumber (&pLogicDisk->BPB);
  pLogicDisk->FATType = GetFATType (&pLogicDisk->BPB);

  // Once we figured out the type of FAT using the calculated
  // number of clusters (this is the official way of FAT type determination),
  // let's make sure it makes sense:
  if (pLogicDisk->FATType == ft_FAT32)
  {
    if (pBPB->BPB1.RootEntriesCount)
      goto lend;
  }
  else
  {
    if (!pBPB->BPB1.RootEntriesCount)
      goto lend;
  }

  pLogicDisk->FAT1LBA = GetFirstFATSectorLBA (&pLogicDisk->BPB);
  pLogicDisk->SectorsPerFAT = GetSectorsPerFAT (&pLogicDisk->BPB);
  pLogicDisk->NumberOfFATs = GetNumberOfFATs (&pLogicDisk->BPB);
  pLogicDisk->RootDirectoryLBA = GetRootDirFirstSectorLBA (&pLogicDisk->BPB);
  pLogicDisk->DataAreaLBA = GetFirstDataSectorLBA (&pLogicDisk->BPB);
  pLogicDisk->DataSectorsCount = GetDataSectorsCount (&pLogicDisk->BPB);
  pLogicDisk->ClustersCount = GetClustersCount (&pLogicDisk->BPB);
  pLogicDisk->CurrentDirClusterNo = GetRootDirClusterNo (&pLogicDisk->BPB);

  pLogicDisk->LastKnownFreeClusterCount = 0xFFFFFFFFUL;
  pLogicDisk->FirstClusterToCheckIfFree = 0xFFFFFFFFUL;

  if (pLogicDisk->IsFloppy)
    pFATFS->pIsFloppyTooLongUnaccessed (pFATFS, 0, &pLogicDisk->LastAccessTime);
  else
    pLogicDisk->LastAccessTime = 0;

  // More checks can be done: $$$

  // Boot sector:
  // aJump[] must contain 0xE9,0xXX,0xXX or 0xEB,0xXX,0x90:

  // FAT:
  // FAT entry 0 must have MediaType in bits 0 through 7 and all ones in bits 8+:
  // FAT entry 1 must contain end of cluster chain mark,
  // 2 MSBits of it on FAT16/32 may be used for "clean shutdown"&"I/O error" flags
  // should the disk be dropped if they indicate a problem?: $$$
lend:

  return;
}

static tIORes InitDisk (tFATFS* pFATFS, uint8 DiskNo)
{
  uchar sector[FAT_SECTOR_SIZE];
  tFATBootSector* pBootSect = (tFATBootSector*)sector;
  tIORes res;
  uint8 BIOSDrive;
  uint32 LBA;

  memset (sector, 0, sizeof(sector));

  pFATFS->aLogicDisks[DiskNo].IsValid = 0;

  // Read partition's boot sector:
  BIOSDrive = pFATFS->aLogicDisks[DiskNo].DiskLocation.BIOSDrive;
  LBA = pFATFS->aLogicDisks[DiskNo].DiskLocation.PartitionEntry.StartSectorLBA;
  res = pFATFS->pReadSectorLBA
  (
    pFATFS,
    NULL,
    BIOSDrive,
    LBA,
    1,
    sector
  );

  if (res != ior_OK)
  {
    LOG_ERR4 ("Read error, Disk 0x%02X, LBA: %lu, res: %u",
              (uint)BIOSDrive, (ulong)LBA, (uint)res);
  }

  // Skip boot sector if error or invalid AA55 signature:
  if
  (
    (res == ior_OK) &&
    (pBootSect->Signature0xAA55 == 0xAA55U)
  )
  {
    // Initialize logical disk's FS information from BPB:
    InitDiskFromBPB (pFATFS, &pFATFS->aLogicDisks[DiskNo], &pBootSect->BPB);
    if (!pFATFS->aLogicDisks[DiskNo].IsValid)
      return ior_FS_DAMAGED;

    // Do some more FAT32-related initialization:
    if (pFATFS->aLogicDisks[DiskNo].FATType == ft_FAT32)
    {
      tFAT32FSInfoSector* pFAT32FSInfoSector = (tFAT32FSInfoSector*)sector;

      memset (sector, 0, sizeof(sector));

      // Read FSInfo sector:
      LBA = pFATFS->aLogicDisks[DiskNo].DiskLocation.PartitionEntry.StartSectorLBA +
        pFATFS->aLogicDisks[DiskNo].BPB.BPB2.FAT32.FSInfoSectorNo;
      res = pFATFS->pReadSectorLBA
      (
        pFATFS,
        NULL,
        BIOSDrive,
        LBA,
        1,
        sector
      );

      if (res != ior_OK)
      {
        LOG_ERR4 ("Read error, Disk 0x%02X, LBA: %lu, res: %u",
                  (uint)BIOSDrive, (ulong)LBA, (uint)res);
        pFATFS->aLogicDisks[DiskNo].IsValid = 0;
        return res;
      }

      if
      (
        (pFAT32FSInfoSector->LeadingSignature0x41615252 != 0x41615252UL) ||
        (pFAT32FSInfoSector->StrucSignature0x61417272 != 0x61417272UL) ||
        (pFAT32FSInfoSector->TrailingSignature0xAA550000 != 0xAA550000UL)
      )
      {
        LOG_ERR0();
        pFATFS->aLogicDisks[DiskNo].IsValid = 0;
        return ior_FS_DAMAGED;
      }

      // Take the value of FirstClusterToCheckIfFree out of the FSInfo sector:
      pFATFS->aLogicDisks[DiskNo].FirstClusterToCheckIfFree =
        pFAT32FSInfoSector->FirstClusterToCheckIfFree;
      // $$$ Ignore LastKnownFreeClusterCount for now
    }

    return res;
  }

  if (res == ior_OK)
    res = ior_FS_DAMAGED;

  return res;
}

int FAT_Init (tFATFS* pFATFS,
              const uint8* pBIOSDrivesToScan,
              uint NumberOfBIOSDrivesToScan,
              tpDoesDriveExistFxn pDoesDriveExist,
              tpReadSectorLBAFxn pReadSectorLBA,
              tpWriteSectorLBAFxn pWriteSectorLBA,
              tpGetDateAndTime pGetDateAndTime,
              tpIsFloppyTooLongUnaccessed pIsFloppyTooLongUnaccessed,
              tpOpenFileSearchFxn pOpenFileSearch,
              tpMessageLogFxn pMessageLog,
              int FATBootDiskHint/*or -1*/)
{
  uchar sector[FAT_SECTOR_SIZE];
  tPartitionSector* pPartSect = (tPartitionSector*)sector;
  tFATBootSector* pBootSect = (tFATBootSector*)sector;
  int DrvIdx;
  uint8 BIOSDrive;
  tIORes res;
  int PartitionNo;
  int i;

  if ((pFATFS == NULL) ||
      (pDoesDriveExist == NULL) ||
      (pReadSectorLBA == NULL) ||
      (pWriteSectorLBA == NULL) ||
      (pGetDateAndTime == NULL) ||
      (pIsFloppyTooLongUnaccessed == NULL) ||
      (pOpenFileSearch == NULL) ||
      (!NumberOfBIOSDrivesToScan) ||
      (pBIOSDrivesToScan == NULL))
    return 0;

  // Make sure the BIOS drives list contains no duplicates and
  // is also ordered from lower to higher drive numbers
  for (DrvIdx=0; DrvIdx<NumberOfBIOSDrivesToScan-1; DrvIdx++)
  {
    if (pBIOSDrivesToScan[DrvIdx] >= pBIOSDrivesToScan[DrvIdx+1])
      return 0;
  }

  // Start initializing *pFATFS:
  memset (pFATFS, 0, sizeof(tFATFS));
  pFATFS->pDoesDriveExist = pDoesDriveExist;
  pFATFS->pReadSectorLBA = pReadSectorLBA;
  pFATFS->pWriteSectorLBA = pWriteSectorLBA;
  pFATFS->pGetDateAndTime = pGetDateAndTime;
  pFATFS->pIsFloppyTooLongUnaccessed = pIsFloppyTooLongUnaccessed;
  pFATFS->pOpenFileSearch = pOpenFileSearch;
  if (pMessageLog != NULL)
    pFATFS->pMessageLog = pMessageLog;
  else
    pFATFS->pMessageLog = &DummyMessageLog;

  // Partition scan begining
/*
  Disk letter assignment is done according to
  Microsoft Knowledge Base Article 93373/KB93373/Q93373,
  Default Drive Letters and Partitions in Windows NT.

  Excerpt:

  If there is a primary partition on the first hard drive marked as active,
  it gets the first drive letter (C); otherwise, the first drive letter is
  assigned to the first recognized primary partition.

  This process is repeated for all hard drives in the system. Please note that
  if you have multiple controllers in your system, the drive letter ordering
  is based on the order in which the device drivers are loaded by Windows NT.

  Once the letters have been assigned to the first primary partitions on all
  drives in the system, letters are assigned to the recognized logical disks
  in the extended partitions using the same scheme as outlined above,
  starting with the first drive in the system.

  After all of the logical disks in the extended partitions are assigned
  letters, one last scan is made of the drives, and letters are assigned
  to any remaining recognized primary partitions.
*/

  // Partition scan stage 1:

  // Find and store hard drives' valid FAT primary partitions (one from drive)
  for (DrvIdx=0; DrvIdx<NumberOfBIOSDrivesToScan; DrvIdx++)
  {
    int ActivePrimaryPartitionNo = -1;
    int InactivePrimaryPartitionNo = -1;
    BIOSDrive = pBIOSDrivesToScan[DrvIdx];

    // Skip floppies from the BIOS drives list
    if (BIOSDrive < 0x80)
      continue;
    if (!pFATFS->pDoesDriveExist (pFATFS, BIOSDrive))
      continue;

    // Read hard drive's MBR sector
    memset (sector, 0, sizeof(sector));
    res = pFATFS->pReadSectorLBA
    (
      pFATFS,
      NULL,
      BIOSDrive,
      0,
      1,
      sector
    );
    // Skip MBR if error or invalid AA55 signature
    if (res != ior_OK)
    {
      LOG_ERR4 ("Read error, Disk 0x%02X, LBA: %lu, res: %u",
                (uint)BIOSDrive, 0UL, (uint)res);
      continue;
    }
    if (pPartSect->Signature0xAA55 != 0xAA55U)
      continue;

    // First, scan MBR for active primary partition
    for (PartitionNo=0; PartitionNo<4; PartitionNo++)
      if
      (
        IsPrimaryPartition (pPartSect->aPartitions[PartitionNo].PartitionType) &&
        (pPartSect->aPartitions[PartitionNo].PartitionStatus == ps_ACTIVE_PARTITION)
      )
      {
        ActivePrimaryPartitionNo = PartitionNo;
        break;
      }
    // Second, scan MBR for inactive primary partition
    for (PartitionNo=0; PartitionNo<4; PartitionNo++)
      if (IsPrimaryPartition (pPartSect->aPartitions[PartitionNo].PartitionType))
      {
        InactivePrimaryPartitionNo = PartitionNo;
        break;
      }

    // Add either active or inactive primary partition, whichever exists
    PartitionNo = -1;
    if (ActivePrimaryPartitionNo >= 0)
      PartitionNo = ActivePrimaryPartitionNo;
    else if (InactivePrimaryPartitionNo >= 0)
      PartitionNo = InactivePrimaryPartitionNo;
    if (PartitionNo >= 0)
    {
      // Try storing a FAT partition, stop if not enough space (disk letters)
      if (!AddDiskPartition (pFATFS, BIOSDrive,
                             &pPartSect->aPartitions[PartitionNo]))
        goto LDoneWithPartitionScan;
    }
  }

  // Partition scan stage 2:

  // Find and store hard drives' valid FAT extended partitions (one from drive)
  for (DrvIdx=0; DrvIdx<NumberOfBIOSDrivesToScan; DrvIdx++)
  {
    // Current Master Boot Record (MBR) LBA
    // (hard drive's or extended partition's) and
    // LBA of extended FAT partition in hard drive's MBR are
    // needed to convert nested partitions' LBAs to absolute drive LBAs
    // in order to simplify access the logic disks
    uint32 MBRLBA = 0;
    uint32 FirstExtPartLBA = 0;

    BIOSDrive = pBIOSDrivesToScan[DrvIdx];

    // Skip floppies from the BIOS drives list
    if (BIOSDrive < 0x80)
      continue;
    if (!pFATFS->pDoesDriveExist (pFATFS, BIOSDrive))
      continue;

LNestedPartition:
    // Read hard drive's (or extended partition's) MBR sector
    memset (sector, 0, sizeof(sector));
    res = pFATFS->pReadSectorLBA
    (
      pFATFS,
      NULL,
      BIOSDrive,
      MBRLBA,
      1,
      sector
    );
    // Skip MBR if error or invalid AA55 signature
    if (res != ior_OK)
    {
      LOG_ERR4 ("Read error, Disk 0x%02X, LBA: %lu, res: %u",
                (uint)BIOSDrive, (ulong)MBRLBA, (uint)res);
      continue;
    }
    if (pPartSect->Signature0xAA55 != 0xAA55U)
      continue;

    // Scan nested MBR for primary partitions nested in it and add them (if any)
    if (FirstExtPartLBA)
      for (PartitionNo=0; PartitionNo<4; PartitionNo++)
        if (IsPrimaryPartition (pPartSect->aPartitions[PartitionNo].PartitionType))
        {
          pPartSect->aPartitions[PartitionNo].StartSectorLBA += MBRLBA;
          // Try storing a FAT partition, stop if not enough space (disk letters)
          if (!AddDiskPartition (pFATFS, BIOSDrive,
                                 &pPartSect->aPartitions[PartitionNo]))
            goto LDoneWithPartitionScan;
        }

    // Scan MBR for an extended partition and add
    // primary partitions nested in it (if any)
    for (PartitionNo=0; PartitionNo<4; PartitionNo++)
      if (IsExtendedPartition (pPartSect->aPartitions[PartitionNo].PartitionType))
      {
        if (!FirstExtPartLBA)
          // 1st extended partition (hard disk MBR's) is easy,
          // because StartSectorLBA is an absolute LBA:
          MBRLBA = FirstExtPartLBA =
            pPartSect->aPartitions[PartitionNo].StartSectorLBA;
        else
          // nested extended partition is a bit odd,
          // because StartSectorLBA is a relative LBA to
          // LBA of 1st extended partition (hard disk MBR's):
          MBRLBA = FirstExtPartLBA +
            pPartSect->aPartitions[PartitionNo].StartSectorLBA;
        goto LNestedPartition;
      }
  }

  // Partition scan stage 3:

  // Find and store remaining hard drives' valid FAT primary partitions (if any)
  for (DrvIdx=0; DrvIdx<NumberOfBIOSDrivesToScan; DrvIdx++)
  {
    BIOSDrive = pBIOSDrivesToScan[DrvIdx];

    // Skip floppies from the BIOS drives list
    if (BIOSDrive < 0x80)
      continue;
    if (!pFATFS->pDoesDriveExist (pFATFS, BIOSDrive))
      continue;

    // Read hard drive's MBR sector
    memset (sector, 0, sizeof(sector));
    res = pFATFS->pReadSectorLBA
    (
      pFATFS,
      NULL,
      BIOSDrive,
      0,
      1,
      sector
    );
    // Skip MBR if error or invalid AA55 signature
    if (res != ior_OK)
    {
      LOG_ERR4 ("Read error, Disk 0x%02X, LBA: %lu, res: %u",
                (uint)BIOSDrive, 0UL, (uint)res);
      continue;
    }
    if (pPartSect->Signature0xAA55 != 0xAA55U)
      continue;

    for (PartitionNo=0; PartitionNo<4; PartitionNo++)
      if (IsPrimaryPartition (pPartSect->aPartitions[PartitionNo].PartitionType))
      {
        if (IsPartitionAlreadyAdded (pFATFS, BIOSDrive,
                                     &pPartSect->aPartitions[PartitionNo]))
          continue;
        // Try storing a FAT partition, stop if not enough space (disk letters)
        if (!AddDiskPartition (pFATFS, BIOSDrive,
                               &pPartSect->aPartitions[PartitionNo]))
          goto LDoneWithPartitionScan;
      }
  }

  // End of partition scans

LDoneWithPartitionScan:

  // Read in boot sectors for floppies and found hard drives' FAT partitions,
  // initialize associated logical disks:
  for (DrvIdx=0; DrvIdx<MAX_LOGIC_DISKS; DrvIdx++)
  {
    pFATFS->aLogicDisks[DrvIdx].IsValid = 0;

    if (DrvIdx < 2)
    {
      pFATFS->aLogicDisks[DrvIdx].Exists = 0;
      pFATFS->aLogicDisks[DrvIdx].IsFloppy = 1;
      pFATFS->aLogicDisks[DrvIdx].DiskLocation.BIOSDrive = DrvIdx;
      pFATFS->aLogicDisks[DrvIdx].DiskLocation.PartitionEntry.StartSectorLBA = 0;
      for (i = 0; i < NumberOfBIOSDrivesToScan; i++)
      {
        // Ignore floppy drives that aren't listed:
        if (pBIOSDrivesToScan[i] == DrvIdx)
        {
          pFATFS->aLogicDisks[DrvIdx].Exists =
            pFATFS->pDoesDriveExist (pFATFS, DrvIdx);
          pFATFS->LogicDisksCount += pFATFS->aLogicDisks[DrvIdx].Exists;
        }
      }
    }

    if (!pFATFS->aLogicDisks[DrvIdx].Exists)
      continue;

    // Don't read floppy now, read when needed:
    if (pFATFS->aLogicDisks[DrvIdx].IsFloppy)
      continue;

    // Read partition's boot sector and
    // initialize logical disk's FS information from BPB:
    InitDisk (pFATFS, DrvIdx);

    (void)pBootSect;
  }

  // Set current disk (try the global variable containing the boot disk number):
  if ((FATBootDiskHint >= 0) &&
      (FATBootDiskHint < MAX_LOGIC_DISKS) &&
      pFATFS->aLogicDisks[FATBootDiskHint].Exists)
  {
    pFATFS->CurrentLogicDiskNo = FATBootDiskHint;
  }
  else
  {
    // if no boot disk available, use the first existing disk as the current:
    pFATFS->CurrentLogicDiskNo = -1;
    for (DrvIdx=0; DrvIdx<MAX_LOGIC_DISKS; DrvIdx++)
      if (pFATFS->aLogicDisks[DrvIdx].Exists)
      {
        pFATFS->CurrentLogicDiskNo = DrvIdx;
        break;
      }
  }

  // Init cached FAT images:
  for (i=0; i<MAX_FAT_IMAGES_AT_ONCE; i++)
  {
    Cache_InitEntry (&pFATFS->aFATCacheEntries[i], &pFATFS->aFATImages[i]);
    pFATFS->aFATImages[i].pFATFS = pFATFS;
  }

  // Init cached directory images:
  for (i=0; i<MAX_DIR_IMAGES_AT_ONCE; i++)
  {
    Cache_InitEntry (&pFATFS->aDirCacheEntries[i], &pFATFS->aDirImages[i]);
    pFATFS->aDirImages[i].pFATFS = pFATFS;
  }

  // Error if couldn't set current disk:
  if (pFATFS->CurrentLogicDiskNo < 0)
  {
    LOG_ERR0();
    return 0;
  }

  return 1;
}

static int FATImageFlush (tCacheEntry* pCacheEntry);

static int DirImageFlush (tCacheEntry* pCacheEntry);

static tIORes FindNextFreeFATEntry (tFATFS* pFATFS, uint8 DiskNo,
                                    uint32* pEntryValue);

static tIORes GetFreeFATEntriesCount (tFATFS* pFATFS, uint8 DiskNo,
                                      uint32* pCount);


int FAT_Flush (tFATFS* pFATFS)
{
  int DiskNo, i, fres = 1;
  uchar sector[FAT_SECTOR_SIZE];
  tIORes res;

  if (pFATFS == NULL)
    return 0;

  // Update FAT32 FSInfo sectors:
  for (DiskNo=0; DiskNo<MAX_LOGIC_DISKS; DiskNo++)
    if
    (
      (pFATFS->aLogicDisks[DiskNo].Exists) &&
      (pFATFS->aLogicDisks[DiskNo].IsValid) &&
      (pFATFS->aLogicDisks[DiskNo].FATType == ft_FAT32)
    )
    {
      tFAT32FSInfoSector* pFAT32FSInfoSector = (tFAT32FSInfoSector*)sector;
      uint8 BIOSDrive;
      uint32 LBA;

      memset (sector, 0, sizeof(sector));

      // Read FSInfo sector:
      BIOSDrive = pFATFS->aLogicDisks[DiskNo].DiskLocation.BIOSDrive;
      LBA = pFATFS->aLogicDisks[DiskNo].DiskLocation.PartitionEntry.StartSectorLBA +
          pFATFS->aLogicDisks[DiskNo].BPB.BPB2.FAT32.FSInfoSectorNo,
      res = pFATFS->pReadSectorLBA
      (
        pFATFS,
        NULL,
        BIOSDrive,
        LBA,
        1,
        sector
      );

      if (res != ior_OK)
      {
        LOG_ERR4 ("Read error, Disk 0x%02X, LBA: %lu, res: %u",
                  (uint)BIOSDrive, (ulong)LBA, (uint)res);
        fres = 0;
        continue;
      }

      if
      (
        (pFAT32FSInfoSector->LeadingSignature0x41615252 != 0x41615252UL) ||
        (pFAT32FSInfoSector->StrucSignature0x61417272 != 0x61417272UL) ||
        (pFAT32FSInfoSector->TrailingSignature0xAA550000 != 0xAA550000UL)
      )
      {
        LOG_ERR0();
        fres = 0;
        continue;
      }

      // Update the value of FirstClusterToCheckIfFree:
      pFATFS->aLogicDisks[DiskNo].FirstClusterToCheckIfFree = 2;
      res = FindNextFreeFATEntry
      (
        pFATFS,
        DiskNo,
        &pFAT32FSInfoSector->FirstClusterToCheckIfFree
      );

      if (res != ior_OK)
      {
        pFAT32FSInfoSector->FirstClusterToCheckIfFree = 0xFFFFFFFFUL;
        fres = 0;
      }

      // Update the value of LastKnownFreeClusterCount:
      res = GetFreeFATEntriesCount
      (
        pFATFS,
        DiskNo,
        &pFAT32FSInfoSector->LastKnownFreeClusterCount
      );

      if (res != ior_OK)
      {
        pFAT32FSInfoSector->LastKnownFreeClusterCount = 0xFFFFFFFFUL;
        fres = 0;
      }

      // Write FSInfo sector:
      res = pFATFS->pWriteSectorLBA
      (
        pFATFS,
        NULL,
        BIOSDrive,
        LBA,
        1,
        sector
      );

      if (res != ior_OK)
      {
        LOG_ERR4 ("Write error, Disk 0x%02X, LBA: %lu, res: %u",
                  (uint)BIOSDrive, (ulong)LBA, (uint)res);
        fres = 0;
      }

      if (pFATFS->aLogicDisks[DiskNo].BPB.BPB2.FAT32.BackupBootSectorNo)
      {
        // Write copy of the FSInfo sector:
        LBA = pFATFS->aLogicDisks[DiskNo].DiskLocation.PartitionEntry.StartSectorLBA +
          pFATFS->aLogicDisks[DiskNo].BPB.BPB2.FAT32.BackupBootSectorNo +
          pFATFS->aLogicDisks[DiskNo].BPB.BPB2.FAT32.FSInfoSectorNo;
        res = pFATFS->pWriteSectorLBA
        (
          pFATFS,
          NULL,
          BIOSDrive,
          LBA,
          1,
          sector
        );

        if (res != ior_OK)
        {
          LOG_ERR4 ("Write error, Disk 0x%02X, LBA: %lu, res: %u",
                    (uint)BIOSDrive, (ulong)LBA, (uint)res);
          fres = 0;
        }
      }
    }

  // Flush cached directory images:
  for (i=0; i<MAX_DIR_IMAGES_AT_ONCE; i++)
  {
    int r = DirImageFlush (&pFATFS->aDirCacheEntries[i]);
    if (!r)
      LOG_ERR0();
    fres &= r;
  }

  // Flush cached FAT images:
  for (i=0; i<MAX_FAT_IMAGES_AT_ONCE; i++)
  {
    int r = FATImageFlush (&pFATFS->aFATCacheEntries[i]);
    if (!r)
      LOG_ERR0();
    fres &= r;
  }

  return fres;
}

int FAT_Done (tFATFS* pFATFS)
{
  // Flush the caches:
  return FAT_Flush (pFATFS);
}

static tIORes CheckFloppyChange (tFATFS* pFATFS, uint8 DiskNo)
{
  uchar aSector[FAT_SECTOR_SIZE];
  tFATBootSector* pFATBootSector = (tFATBootSector*)aSector;
  tLogicDisk* pLogicDisk = &pFATFS->aLogicDisks[DiskNo];
  tIORes res;
  uint8 BIOSDrive;

  if (!pLogicDisk->IsFloppy)
    return ior_OK;

  // if the floppy has been successfully accessed (read/written) recently,
  // it hasn't changed:
  if (!pFATFS->pIsFloppyTooLongUnaccessed (pFATFS, pLogicDisk->LastAccessTime, NULL))
    return ior_OK;

  // check for the serial number change:
  BIOSDrive = pFATFS->aLogicDisks[DiskNo].DiskLocation.BIOSDrive;
  res = pFATFS->pReadSectorLBA
  (
    pFATFS,
    &pFATFS->aLogicDisks[DiskNo].BPB,
    BIOSDrive,
    0,
    1,
    aSector
  );

  if ((res == ior_ERR_MEDIA_CHANGED) || (res != ior_OK))
  {
    if (res != ior_ERR_MEDIA_CHANGED)
    {
      LOG_ERR4 ("Read error, Disk 0x%02X, LBA: %lu, res: %u",
                (uint)BIOSDrive, 0UL, (uint)res);
    }
    return res;
  }

  if (GetVolumeSerialNumber (&pFATBootSector->BPB) !=
      GetVolumeSerialNumber (&pLogicDisk->BPB))
  {
    LOG_ERR0();
    return ior_ERR_MEDIA_CHANGED;
  }

  // if no change, update the last access time:
  pFATFS->pIsFloppyTooLongUnaccessed (pFATFS, 0, &pLogicDisk->LastAccessTime);

  return ior_OK;
}

static tIORes DiskReadSectorLBA (tFATFS* pFATFS, uint8 DiskNo, uint32 LBA,
                                 uint8 SectorCount, void *pBuf)
{
  tIORes res;
  tPartitionEntry* pPartitionEntry;

  // Can't read from disk using invalid disk info,
  // so (re)reading bootsector and initializing logical disk:
  if (!pFATFS->aLogicDisks[DiskNo].IsValid)
  {
    int attempts = 5;
    while (attempts--)
    {
      // Read boot sector and
      // initialize logical disk's FS information from BPB:
      res = InitDisk (pFATFS, DiskNo);
      if (res == ior_OK)
        break;
    }
    if (res != ior_OK)
      return res;
  }

  pPartitionEntry = &pFATFS->aLogicDisks[DiskNo].DiskLocation.PartitionEntry;

  if ((LBA >= GetTotalSectorsCount(&pFATFS->aLogicDisks[DiskNo].BPB)) ||
     (GetTotalSectorsCount(&pFATFS->aLogicDisks[DiskNo].BPB) - LBA < SectorCount))
  {
    // Probably the file's/dir's first cluster number was too big to be
    // right
    LOG_ERR0();
    return ior_FS_DAMAGED;
  }

  res = pFATFS->pReadSectorLBA
  (
    pFATFS,
    &pFATFS->aLogicDisks[DiskNo].BPB,
    pFATFS->aLogicDisks[DiskNo].DiskLocation.BIOSDrive,
    pPartitionEntry->StartSectorLBA + LBA,
    SectorCount,
    pBuf
  );

  if ((res == ior_ERR_MEDIA_CHANGED) ||
      (CheckFloppyChange (pFATFS, DiskNo) == ior_ERR_MEDIA_CHANGED))
  {
    FAT_OnMediaHasChanged (pFATFS,
                           pFATFS->aLogicDisks[DiskNo].DiskLocation.BIOSDrive);
    res = ior_ERR_MEDIA_CHANGED;
  }

  if (res != ior_OK)
  {
    pFATFS->aLogicDisks[DiskNo].MiscErrorsCount++;
    LOG_ERR4 ("Read error, disk %c:, LBA: %lu, res: %u",
              (char)('A'+DiskNo), (ulong)LBA, (uint)res);
  }
  else if (pFATFS->aLogicDisks[DiskNo].IsFloppy)
  {
    pFATFS->pIsFloppyTooLongUnaccessed
    (
      pFATFS,
      0,
      &pFATFS->aLogicDisks[DiskNo].LastAccessTime
    );
  }

  return res;
}

static tIORes DiskWriteSectorLBA (tFATFS* pFATFS, uint8 DiskNo, uint32 LBA,
                                  uint8 SectorCount, const void *pBuf)
{
  tIORes res;
  tPartitionEntry* pPartitionEntry;

  // Can't write to disk using invalid disk info:
  if (!pFATFS->aLogicDisks[DiskNo].IsValid)
//    return ior_ERR_IO;
    return ior_ERR_MEDIA_CHANGED;

#if !WRITING_TO_FAT32_ENABLED
  if (pFATFS->aLogicDisks[DiskNo].FATType == ft_FAT32)
  {
    return ior_ERR_IO;
  }
#endif

  if (CheckFloppyChange (pFATFS, DiskNo) == ior_ERR_MEDIA_CHANGED)
  {
    FAT_OnMediaHasChanged (pFATFS,
                           pFATFS->aLogicDisks[DiskNo].DiskLocation.BIOSDrive);
    res = ior_ERR_MEDIA_CHANGED;
  }
  else
  {
    pPartitionEntry = &pFATFS->aLogicDisks[DiskNo].DiskLocation.PartitionEntry;

  if ((LBA >= GetTotalSectorsCount(&pFATFS->aLogicDisks[DiskNo].BPB)) ||
     (GetTotalSectorsCount(&pFATFS->aLogicDisks[DiskNo].BPB) - LBA < SectorCount))
    {
      // Probably the file's/dir's first cluster number was too big to be
      // right
      LOG_ERR0();
      return ior_FS_DAMAGED;
    }

    res = pFATFS->pWriteSectorLBA
    (
      pFATFS,
      &pFATFS->aLogicDisks[DiskNo].BPB,
      pFATFS->aLogicDisks[DiskNo].DiskLocation.BIOSDrive,
      pPartitionEntry->StartSectorLBA + LBA,
      SectorCount,
      pBuf
    );

    if (res == ior_ERR_MEDIA_CHANGED)
    {
      FAT_OnMediaHasChanged (pFATFS,
                             pFATFS->aLogicDisks[DiskNo].DiskLocation.BIOSDrive);
      res = ior_ERR_MEDIA_CHANGED;
    }
  }

  if (res != ior_OK)
  {
    pFATFS->aLogicDisks[DiskNo].MiscErrorsCount++;
    LOG_ERR4 ("Write error, disk %c:, LBA: %lu, res: %u",
              (char)('A'+DiskNo), (ulong)LBA, (uint)res);
  }
  else if (pFATFS->aLogicDisks[DiskNo].IsFloppy)
  {
    pFATFS->pIsFloppyTooLongUnaccessed
    (
      pFATFS,
      0,
      &pFATFS->aLogicDisks[DiskNo].LastAccessTime
    );
  }

  return res;
}

typedef struct tFATImageReuseInfo {
  int           DiskNo;
  uint32        FirstFATSectorNeededNo;
} tFATImageReuseInfo;

static int FATImageIsReusable (tCacheEntry* pCacheEntry, void* pReuseInfo)
{
  tFATImage* pFATImage = (tFATImage*)pCacheEntry->pAttachedData;
  tFATImageReuseInfo* pImageReuseInfo = (tFATImageReuseInfo*)pReuseInfo;

  if
  (
    (pFATImage->DiskNo == pImageReuseInfo->DiskNo) &&
    (pFATImage->FirstLoadedFATSectorNo == pImageReuseInfo->FirstFATSectorNeededNo)
  )
  {
    tFATFS* pFATFS = pFATImage->pFATFS;
    int DiskNo = pFATImage->DiskNo;
    tLogicDisk* pLogicDisk = &pFATFS->aLogicDisks[DiskNo];
    tIORes res;

    res = CheckFloppyChange (pFATFS, DiskNo);

    if (res == ior_ERR_MEDIA_CHANGED)
      FAT_OnMediaHasChanged (pFATFS,
                             pLogicDisk->DiskLocation.BIOSDrive);

    // if no error, the cache is up to date:
    if (res == ior_OK)
      return 1;

    pFATFS->aLogicDisks[DiskNo].MiscErrorsCount++;
    LOG_ERR3 ("Read error, disk %c:, res: %u",
              (char)('A'+DiskNo), (uint)res);
  }

  return 0;
}

static int FATImageRefill (tCacheEntry* pCacheEntry, void* pReuseInfo)
{
  tFATImage* pFATImage = (tFATImage*)pCacheEntry->pAttachedData;
  tFATImageReuseInfo* pImageReuseInfo = (tFATImageReuseInfo*)pReuseInfo;
  tFATFS* pFATFS = pFATImage->pFATFS;
  int DiskNo = pImageReuseInfo->DiskNo;
  int i;
  tIORes res, FinalRes=ior_MEMORY_CORRUPTED;

  // Don't refill a valid reusable entry:
  if (Cache_GetIsValid (pCacheEntry) && FATImageIsReusable (pCacheEntry, pReuseInfo))
    return 1;

  // Load FAT image from offcache storage:
  for (i=0; i<pFATFS->aLogicDisks[DiskNo].NumberOfFATs; i++)
  {
    res = DiskReadSectorLBA
    (
      pFATFS,
      DiskNo,
      pFATFS->aLogicDisks[DiskNo].FAT1LBA +
        pImageReuseInfo->FirstFATSectorNeededNo +
        i * pFATFS->aLogicDisks[DiskNo].SectorsPerFAT,
      MIN (3, pFATFS->aLogicDisks[DiskNo].SectorsPerFAT -
                pImageReuseInfo->FirstFATSectorNeededNo),
      pFATImage->a3FATSectors
    );
    // If media changed, exit immediately:
    if (res == ior_ERR_MEDIA_CHANGED)
    {
      FinalRes = res;
      break;
    }
    // Log other read errors (if any) and try reading next FAT copy:
    if (res != ior_OK)
    {
      pFATFS->aLogicDisks[DiskNo].FATErrorsCount++;
      LOG_ERR3 ("FAT read error, disk %c:, res: %u",
                (char)('A'+DiskNo), (uint)res);
      FinalRes = ior_ERR_IO;
    }
    // Use successfully read FAT copy:
    if (res == ior_OK)
    {
      FinalRes = res; // final result is OK if at least one FAT copy read
      break;
    }
  }

  // Use successfully read FAT copy:
  if (FinalRes == ior_OK)
  {
    pFATImage->DiskNo = DiskNo;
    pFATImage->FirstLoadedFATSectorNo = pImageReuseInfo->FirstFATSectorNeededNo;
  }

  // Set error code:
  Cache_SetErrorCode (pCacheEntry, FinalRes);

  // Return value (refill success flag) will affect Cache_GetIsValid():
  return FinalRes == ior_OK;
}

static int FATImageFlush (tCacheEntry* pCacheEntry)
{
  tFATImage* pFATImage = (tFATImage*)pCacheEntry->pAttachedData;
  int DiskNo = pFATImage->DiskNo;
  tFATFS* pFATFS = pFATImage->pFATFS;
  tIORes res, FinalRes=ior_MEMORY_CORRUPTED;
  int i;

  // Don't flush a clean entry:
  if (!Cache_GetIsDirty (pCacheEntry))
    return 1;

  // Save entry to offcache storage:
  for (i=0; i<pFATFS->aLogicDisks[pFATImage->DiskNo].NumberOfFATs; i++)
  {
    res = DiskWriteSectorLBA
    (
      pFATFS,
      DiskNo,
      pFATFS->aLogicDisks[DiskNo].FAT1LBA +
        pFATImage->FirstLoadedFATSectorNo +
        i * pFATFS->aLogicDisks[DiskNo].SectorsPerFAT,
      MIN (3, pFATFS->aLogicDisks[DiskNo].SectorsPerFAT -
                pFATImage->FirstLoadedFATSectorNo),
      pFATImage->a3FATSectors
    );
    // If media changed, exit immediately:
    if (res == ior_ERR_MEDIA_CHANGED)
    {
      FinalRes = res;
      break;
    }
    // Log other write errors (if any) and continue writing to next FAT copy:
    if (res != ior_OK)
    {
      pFATFS->aLogicDisks[DiskNo].FATErrorsCount++;
      LOG_ERR3 ("FAT write error, disk %c:, res: %u",
                (char)('A'+DiskNo), (uint)res);
    }
    else
    {
      FinalRes = ior_OK; // final result is OK if at least one FAT copy written
    }
  }

  // Set error code:
  Cache_SetErrorCode (pCacheEntry, FinalRes);

  // Reset the dirty flag:
  if (FinalRes == ior_OK)
  {
    Cache_ResetIsDirty (pCacheEntry);
  }

  // Return value (flush success flag) will be ignored:
  return FinalRes == ior_OK;
}

static tIORes GetFATEntry (tFATFS* pFATFS, uint8 DiskNo, uint32 EntryNo,
                           uint32* pEntryValue)
{
  uint32 FirstFATSectorNeededNo=0;
  uint OffsetWithinSector=0;
  tCacheEntry* pCacheEntry;
  tFATImageReuseInfo FATImageReuseInfo;
  tFATImage* pFATImage;
  tIORes res = ior_OK;

  if (EntryNo > pFATFS->aLogicDisks[DiskNo].ClustersCount + 1)
  {
    LOG_ERR0();
    return ior_FS_DAMAGED;
  }

  switch (pFATFS->aLogicDisks[DiskNo].FATType)
  {
    case ft_FAT12:
      // 3 sectors contain 1024 FAT12 entries:
      FirstFATSectorNeededNo = 3 * (EntryNo >> 10);
      OffsetWithinSector = (uint)(((EntryNo & 0x3FF) * 3) >> 1);
    break;
    case ft_FAT16:
      // 3 sectors contain 768 FAT16 entries:
      FirstFATSectorNeededNo = 3 * (EntryNo / 768);
      OffsetWithinSector = (uint)((EntryNo % 768) << 1);
    break;
    case ft_FAT32:
      // 3 sectors contain 384 FAT32 entries:
      FirstFATSectorNeededNo = 3 * (EntryNo / 384);
      OffsetWithinSector = (uint)((EntryNo % 384) << 2);
    break;
    case ft_UNKNOWN:
      LOG_ERR0();
      return ior_MEMORY_CORRUPTED;
  }

  // Find needed cache entry:
  FATImageReuseInfo.DiskNo                 = DiskNo;
  FATImageReuseInfo.FirstFATSectorNeededNo = FirstFATSectorNeededNo;
  pCacheEntry = Cache_FindEntryForUse
  (
    pFATFS->aFATCacheEntries,
    MAX_FAT_IMAGES_AT_ONCE,
    &FATImageReuseInfo,
    &FATImageRefill,
    &FATImageFlush,
    &FATImageIsReusable
  );

  // Check validity of found entry:
  if (!Cache_GetIsValid (pCacheEntry))
  {
    res = (tIORes)Cache_GetErrorCode (pCacheEntry);

    LOG_ERR2("%u", (uint)res);
    return res;
  }

  // Return attached data
  pFATImage = (tFATImage*)pCacheEntry->pAttachedData;

  switch (pFATFS->aLogicDisks[DiskNo].FATType)
  {
    case ft_FAT12:
      *pEntryValue = *(uint16*)&pFATImage->a3FATSectors[OffsetWithinSector];
      if (EntryNo & 1)
        (*pEntryValue) >>= 4;
      else
        (*pEntryValue) &= 0xFFF;
    break;
    case ft_FAT16:
      *pEntryValue = *(uint16*)&pFATImage->a3FATSectors[OffsetWithinSector];
    break;
    case ft_FAT32:
      *pEntryValue = *(uint32*)&pFATImage->a3FATSectors[OffsetWithinSector];
    break;
    case ft_UNKNOWN:
      LOG_ERR0();
      return ior_MEMORY_CORRUPTED;
  }

  return res;
}

static tIORes FindPrevFATEntry (tFATFS* pFATFS, uint8 DiskNo,
                                uint32 StartFromEntryNo, uint32 EntryNo,
                                uint32* pEntryValue)
{
  tIORes res;
  uint32 i;

  if
  (
    (StartFromEntryNo < 2) ||
    (StartFromEntryNo >= 2 + pFATFS->aLogicDisks[DiskNo].ClustersCount)
  )
    StartFromEntryNo = 2;

  // Search starting with entry specified as the one where to start:
  for
  (
    i = StartFromEntryNo;
    i < 2 + pFATFS->aLogicDisks[DiskNo].ClustersCount;
    i++
  )
  {
    res = GetFATEntry (pFATFS, DiskNo, i, pEntryValue);
    if (res != ior_OK)
      return res;
    if (EntryNo == *pEntryValue)
    {
       // Found:
       *pEntryValue = i;
       return ior_OK;
    }
  }

  //  Check if already searched in entire FAT:
  if (StartFromEntryNo == 2)
    goto lend;

  // Search starting at FAT beginning till
  // the entry specified as the one where to start:
  for
  (
    i = 2;
    i < StartFromEntryNo;
    i++
  )
  {
    res = GetFATEntry (pFATFS, DiskNo, i, pEntryValue);
    if (res != ior_OK)
      return res;
    if (EntryNo == *pEntryValue)
    {
       // Found:
       *pEntryValue = i;
       return ior_OK;
    }
  }

lend:
  // Searched all FAT and didn't find:
  *pEntryValue = 0;
  return ior_OK;
}

static tIORes SetFATEntry (tFATFS* pFATFS, uint8 DiskNo, uint32 EntryNo,
                           uint32 EntryValue)
{
  uint32 FirstFATSectorNeededNo=0;
  uint OffsetWithinSector=0;
  tCacheEntry* pCacheEntry;
  tFATImageReuseInfo FATImageReuseInfo;
  tFATImage* pFATImage;
  tIORes res = ior_OK;

  if (EntryNo > pFATFS->aLogicDisks[DiskNo].ClustersCount + 1)
  {
    LOG_ERR0();
    return ior_FS_DAMAGED;
  }

  switch (pFATFS->aLogicDisks[DiskNo].FATType)
  {
    case ft_FAT12:
      // 3 sectors contain 1024 FAT12 entries:
      FirstFATSectorNeededNo = 3 * (EntryNo >> 10);
      OffsetWithinSector = (uint)(((EntryNo & 0x3FF) * 3) >> 1);
    break;
    case ft_FAT16:
      // 3 sectors contain 768 FAT16 entries:
      FirstFATSectorNeededNo = 3 * (EntryNo / 768);
      OffsetWithinSector = (uint)((EntryNo % 768) << 1);
    break;
    case ft_FAT32:
      // 3 sectors contain 384 FAT32 entries:
      FirstFATSectorNeededNo = 3 * (EntryNo / 384);
      OffsetWithinSector = (uint)((EntryNo % 384) << 2);
    break;
    case ft_UNKNOWN:
      LOG_ERR0();
      return ior_MEMORY_CORRUPTED;
  }

  // Find needed cache entry:
  FATImageReuseInfo.DiskNo                 = DiskNo;
  FATImageReuseInfo.FirstFATSectorNeededNo = FirstFATSectorNeededNo;
  pCacheEntry = Cache_FindEntryForUse
  (
    pFATFS->aFATCacheEntries,
    MAX_FAT_IMAGES_AT_ONCE,
    &FATImageReuseInfo,
    &FATImageRefill,
    &FATImageFlush,
    &FATImageIsReusable
  );

  // Check validity of found entry:
  if (!Cache_GetIsValid (pCacheEntry))
    res = (tIORes)Cache_GetErrorCode (pCacheEntry);

  if (res != ior_OK)
  {
    LOG_ERR2("%u", (uint)res);
    return res;
  }

  // Return attached data
  pFATImage = (tFATImage*)pCacheEntry->pAttachedData;

  switch (pFATFS->aLogicDisks[DiskNo].FATType)
  {
    case ft_FAT12:
    {
      uint16* pEntryValue = (uint16*)&pFATImage->a3FATSectors[OffsetWithinSector];
      if (EntryNo & 1)
        *pEntryValue = (*pEntryValue & 0xF) | ((EntryValue & 0xFFF) << 4);
      else
        *pEntryValue = (*pEntryValue & 0xF000U) | (EntryValue & 0xFFF);
    }
    break;
    case ft_FAT16:
    {
      uint16* pEntryValue = (uint16*)&pFATImage->a3FATSectors[OffsetWithinSector];
      *pEntryValue = EntryValue & 0xFFFFU;
    }
    break;
    case ft_FAT32:
    {
      uint32* pEntryValue = (uint32*)&pFATImage->a3FATSectors[OffsetWithinSector];
      *pEntryValue = EntryValue & 0xFFFFFFFUL;
    }
    break;
    case ft_UNKNOWN:
      LOG_ERR0();
      return ior_MEMORY_CORRUPTED;
  }

  Cache_SetIsDirty (pCacheEntry);

  return res;
}

static tIORes FindNextFreeFATEntry (tFATFS* pFATFS, uint8 DiskNo,
                                    uint32* pEntryValue)
{
  tIORes res = ior_OK;
  uint32 i, EntryNo;
  int found = 0;

  // Check last known search point, if any:
  if
  (
    (pFATFS->aLogicDisks[DiskNo].FirstClusterToCheckIfFree == 0xFFFFFFFFUL) ||
    (pFATFS->aLogicDisks[DiskNo].FirstClusterToCheckIfFree < 2) ||
    (pFATFS->aLogicDisks[DiskNo].FirstClusterToCheckIfFree >=
     2+pFATFS->aLogicDisks[DiskNo].ClustersCount)
  )
    pFATFS->aLogicDisks[DiskNo].FirstClusterToCheckIfFree = 2;

  // Search from last known point to the end of FAT:
  for
  (
    i = pFATFS->aLogicDisks[DiskNo].FirstClusterToCheckIfFree;
    i < 2 + pFATFS->aLogicDisks[DiskNo].ClustersCount;
    i++
  )
  {
    res = GetFATEntry (pFATFS, DiskNo, i, &EntryNo);
    if (res != ior_OK)
      return res;
    if (GetFATEntryType (pFATFS->aLogicDisks[DiskNo].FATType, EntryNo) ==
        fet_FREE_CLUSTER)
    {
      EntryNo = i;
      found = 1;
      break;
    }
  }

  // Done searching if found or searched through entire FAT:
  if (found || (pFATFS->aLogicDisks[DiskNo].FirstClusterToCheckIfFree == 2))
    goto lend;

  // Search from the beginning of FAT to last known point:
  for
  (
    i = 2;
    i < pFATFS->aLogicDisks[DiskNo].FirstClusterToCheckIfFree;
    i++
  )
  {
    res = GetFATEntry (pFATFS, DiskNo, i, &EntryNo);
    if (res != ior_OK)
      return res;
    if (GetFATEntryType (pFATFS->aLogicDisks[DiskNo].FATType, EntryNo) ==
        fet_FREE_CLUSTER)
    {
      EntryNo = i;
      found = 1;
      break;
    }
  }

lend:
  if (found)
  {
    i = EntryNo + 1;
    if (i >= 2 + pFATFS->aLogicDisks[DiskNo].ClustersCount)
      i = 2;
    pFATFS->aLogicDisks[DiskNo].FirstClusterToCheckIfFree = i;
  }

  *pEntryValue = EntryNo;

  return found ? ior_OK : ior_NO_SPACE;
}

static tIORes GetFreeFATEntriesCount (tFATFS* pFATFS, uint8 DiskNo,
                                      uint32* pCount)
{
  tIORes res = ior_OK;
  uint32 i, EntryNo;
  uint32 FirstClusterToCheckIfFree;

  *pCount = 0;

  // Check last known search point, if any:
  FirstClusterToCheckIfFree =
    pFATFS->aLogicDisks[DiskNo].FirstClusterToCheckIfFree;

  if
  (
    (pFATFS->aLogicDisks[DiskNo].FirstClusterToCheckIfFree == 0xFFFFFFFFUL) ||
    (pFATFS->aLogicDisks[DiskNo].FirstClusterToCheckIfFree < 2) ||
    (pFATFS->aLogicDisks[DiskNo].FirstClusterToCheckIfFree >=
     2+pFATFS->aLogicDisks[DiskNo].ClustersCount)
  )
    FirstClusterToCheckIfFree = 2;

  // Search from last known point to the end of FAT:
  for
  (
    i = FirstClusterToCheckIfFree;
    i < 2 + pFATFS->aLogicDisks[DiskNo].ClustersCount;
    i++
  )
  {
    res = GetFATEntry (pFATFS, DiskNo, i, &EntryNo);
    if (res != ior_OK)
      return res;
    if (GetFATEntryType (pFATFS->aLogicDisks[DiskNo].FATType, EntryNo) ==
        fet_FREE_CLUSTER)
      (*pCount)++;
  }

  // Search from the beginning of FAT to last known point:
  for
  (
    i = 2;
    i < FirstClusterToCheckIfFree;
    i++
  )
  {
    res = GetFATEntry (pFATFS, DiskNo, i, &EntryNo);
    if (res != ior_OK)
      return res;
    if (GetFATEntryType (pFATFS->aLogicDisks[DiskNo].FATType, EntryNo) ==
        fet_FREE_CLUSTER)
      (*pCount)++;
  }

  return res;
}

static tIORes GrowClusterChain (tFATFS* pFATFS, uint8 DiskNo,
                                uint32* pFirstClusterNo,
                                uint32* pLastClusterNo,
                                uint32 GrowthAmount)
{
  tIORes res, FinalRes;
  uint32 CurrentClusterNo, InitialLastClusterNo, AllocatedClustersCnt,
         FirstAllocatedClusterNo, i;
  uint32 LastClusterMarkValue;

  if (!GrowthAmount)
    return ior_OK;

  // Skip to the end of the chain (if chain exists):
  if ((CurrentClusterNo = *pFirstClusterNo) != 0)
    for(;;)
    {
      uint32 NextClusterNo;
      res = GetFATEntry (pFATFS, DiskNo, CurrentClusterNo,
                         &NextClusterNo);
      if (res != ior_OK)
        return res;
      switch (GetFATEntryType (pFATFS->aLogicDisks[DiskNo].FATType,
                               NextClusterNo))
      {
        case fet_CLUSTER:
          CurrentClusterNo = NextClusterNo;
          continue;
        case fet_LAST_CLUSTER:
          break;
        default:
          // error:
          LOG_ERR0();
          return ior_FS_DAMAGED;
      }
      break;
    }

  // Remember last cluster to which we'll be chaining more clusters:
  InitialLastClusterNo = CurrentClusterNo;

  LastClusterMarkValue = GetLastClusterMarkValue(pFATFS->aLogicDisks[DiskNo].FATType);

  // Now, try allocating more clusters and chaining them together:
  for (AllocatedClustersCnt=i=0; i<GrowthAmount; i++)
  {
    uint32 NextClusterNo;
    res = FindNextFreeFATEntry (pFATFS, DiskNo, &NextClusterNo);

    if (res == ior_OK)
    {
      // Remember the 1st new cluster:
      if (!AllocatedClustersCnt)
        FirstAllocatedClusterNo = NextClusterNo;

      // Mark the cluster found free as being a last cluster:
      res = SetFATEntry (pFATFS, DiskNo, NextClusterNo, LastClusterMarkValue);

      if (res == ior_OK)
      {
        // Chain previously allocated cluster with one, just allocated:
        if (AllocatedClustersCnt++)
        {
          res = SetFATEntry (pFATFS, DiskNo, CurrentClusterNo, NextClusterNo);
          if (res != ior_OK)
            break;                              // I/O error
        }
        CurrentClusterNo = NextClusterNo;       // remember last allocated cluster
      }
      else
        break;                                  // I/O error
    }
    else
      break;                                    // no space or I/O error
  }

  // Try finising, if everything's been OK so far:
  if (res == ior_OK)
  {
    // Chain the two subchains:
    if (InitialLastClusterNo)
      res = SetFATEntry (pFATFS, DiskNo, InitialLastClusterNo,
                         FirstAllocatedClusterNo);
    else
      *pFirstClusterNo = FirstAllocatedClusterNo;
    if (res == ior_OK)
    {
      // Return last cluster
      if (pLastClusterNo != NULL)
        *pLastClusterNo = CurrentClusterNo;
      return res;                               // OK
    }
  }

  // Now handle errors & roll allocation back as far as possible:
  FinalRes = res;       // 1st detected error will be result of growth

  for
  (
    i = 0, CurrentClusterNo = FirstAllocatedClusterNo;
    i < AllocatedClustersCnt;
    i++
  )
  {
    uint32 NextClusterNo;
    res = GetFATEntry (pFATFS, DiskNo, CurrentClusterNo, &NextClusterNo);
    if (res != ior_OK)
      break;                                    // I/O error

    // Free a cluster:
    res = SetFATEntry (pFATFS, DiskNo, CurrentClusterNo, 0);
    if (res != ior_OK)
      break;                                    // I/O error

    // Stop, if can't follow the chain:
    CurrentClusterNo = NextClusterNo;
    if
    (
      GetFATEntryType (pFATFS->aLogicDisks[DiskNo].FATType, CurrentClusterNo) !=
      fet_CLUSTER
    )
    {
      LOG_ERR0();
      break;                                    // FAT may be damaged
    }
  }

  return FinalRes;
}

static tIORes TruncateClusterChain (tFATFS* pFATFS, uint8 DiskNo,
                                    uint32 FirstClusterNo, uint32 AmountToKeep)
{
  tIORes res;
  uint32 CurrentClusterNo, LastClusterNo, i;
  uint32 NextClusterNo;

  // Nothing to truncate in the empty file:
  if (!FirstClusterNo)
    return ior_OK;

  LastClusterNo = 0;
  CurrentClusterNo = FirstClusterNo;

  // Skip at most AmountToKeep clusters:
  for (i=0; i<AmountToKeep; i++)
  {
    LastClusterNo = CurrentClusterNo;
    res = GetFATEntry (pFATFS, DiskNo, CurrentClusterNo,
                       &NextClusterNo);
    if (res != ior_OK)
      return res;
    switch (GetFATEntryType (pFATFS->aLogicDisks[DiskNo].FATType,
                             NextClusterNo))
    {
      case fet_CLUSTER:
        CurrentClusterNo = NextClusterNo;
        continue;
      case fet_LAST_CLUSTER:
        // Requested to keep not fewer clusters than exist in the chain,
        // so, we're done ($$$ should it be >= or == ?, e.g. error if !=):
        return ior_OK;
      default:
        // error:
        LOG_ERR0();
        return ior_FS_DAMAGED;
    }
  }

  if (LastClusterNo)
  {
    // The new end of the cluster chain must be marked as such:
    uint32 LastClusterMarkValue =
      GetLastClusterMarkValue(pFATFS->aLogicDisks[DiskNo].FATType);

    res = SetFATEntry (pFATFS, DiskNo, LastClusterNo, LastClusterMarkValue);

    if (res != ior_OK)
      return res;
  }
  else
  {
    // If the chain is to be truncated to zero size, the 1st cluster to be freed
    // is the 1st cluster of the chain:
    NextClusterNo = FirstClusterNo;
  }

  // Free all remaining clusters till the end of the old chain:
  for (CurrentClusterNo=NextClusterNo;;)
  {
    // Find the next cluster after the one being freed:
    res = GetFATEntry (pFATFS, DiskNo, CurrentClusterNo,
                       &NextClusterNo);
    if (res != ior_OK)
      return res;

    // Free the cluster:
    res = SetFATEntry (pFATFS, DiskNo, CurrentClusterNo, 0);
    if (res != ior_OK)
      return res;

    switch (GetFATEntryType (pFATFS->aLogicDisks[DiskNo].FATType,
                             NextClusterNo))
    {
      case fet_CLUSTER:
        CurrentClusterNo = NextClusterNo;
        continue;
      case fet_LAST_CLUSTER:
        // done:
        return ior_OK;
      default:
        // error:
        LOG_ERR0();
        return ior_FS_DAMAGED;
    }
  }
}

typedef struct tDirImageReuseInfo {
  int                   DiskNo;
  uint32                CurrentClusterNo;
  uint32                CurrentSectorNo;
} tDirImageReuseInfo;

static int DirImageIsReusable (tCacheEntry* pCacheEntry, void* pReuseInfo)
{
  tDirImage* pDirImage = (tDirImage*)pCacheEntry->pAttachedData;
  tDirImageReuseInfo* pImageReuseInfo = (tDirImageReuseInfo*)pReuseInfo;

  if
  (
    (pDirImage->DiskNo           == pImageReuseInfo->DiskNo) &&
    (pDirImage->CurrentClusterNo == pImageReuseInfo->CurrentClusterNo) &&
    (pDirImage->CurrentSectorNo  == pImageReuseInfo->CurrentSectorNo)
  )
  {
    tFATFS* pFATFS = pDirImage->pFATFS;
    int DiskNo = pDirImage->DiskNo;
    tLogicDisk* pLogicDisk = &pFATFS->aLogicDisks[DiskNo];
    tIORes res;

    res = CheckFloppyChange (pFATFS, DiskNo);

    if (res == ior_ERR_MEDIA_CHANGED)
      FAT_OnMediaHasChanged (pFATFS,
                             pLogicDisk->DiskLocation.BIOSDrive);

    // if no error, the cache is up to date:
    if (res == ior_OK)
      return 1;

    pFATFS->aLogicDisks[DiskNo].MiscErrorsCount++;
    LOG_ERR3 ("Read error, disk %c:, res: %u",
              (char)('A'+DiskNo), (uint)res);
  }

  return 0;
}

static int DirImageFlush (tCacheEntry* pCacheEntry)
{
  tDirImage* pDirImage = (tDirImage*)pCacheEntry->pAttachedData;
  tFATFS* pFATFS = pDirImage->pFATFS;
  tIORes res;
  uint32 LBA;

  // Don't flush a clean entry:
  if (!Cache_GetIsDirty (pCacheEntry))
    return 1;

  // Calculate sector's LBA:
  if (!pDirImage->CurrentClusterNo)
  {
    LBA = pFATFS->aLogicDisks[pDirImage->DiskNo].RootDirectoryLBA +
      pDirImage->CurrentSectorNo;
  }
  else
  {
    LBA = GetClusterLBA (&pFATFS->aLogicDisks[pDirImage->DiskNo].BPB,
      pDirImage->CurrentClusterNo) + pDirImage->CurrentSectorNo;
  }

  res = DiskWriteSectorLBA
  (
    pFATFS,
    pDirImage->DiskNo,
    LBA,
    1,
    pDirImage->aSector
  );

  // Set error code:
  Cache_SetErrorCode (pCacheEntry, res);

  // Reset the dirty flag:
  if (res == ior_OK)
  {
    Cache_ResetIsDirty (pCacheEntry);
  }
  else
  {
    LOG_ERR0();
  }

  // Return value (flush success flag) will be ignored:
  return res == ior_OK;
}

// ReadDirImage() is called only from DirImageRefill()
static tIORes ReadDirImage (tFATFS* pFATFS, tDirImage* pDirImage)
{
  tIORes res;
  uint32 LBA;

  // Can't read from disk using invalid disk info,
  // so (re)reading bootsector and initializing logical disk:
  if (!pFATFS->aLogicDisks[pDirImage->DiskNo].IsValid)
  {
    int attempts = 5;
    while (attempts--)
    {
      // Read boot sector and
      // initialize logical disk's FS information from BPB:
      res = InitDisk (pFATFS, pDirImage->DiskNo);
      if (res == ior_OK)
        break;
    }
    if (res != ior_OK)
      return res;
  }

  // Calculate sector's LBA:
  if (!pDirImage->CurrentClusterNo)
  {
    LBA = pFATFS->aLogicDisks[pDirImage->DiskNo].RootDirectoryLBA +
      pDirImage->CurrentSectorNo;
  }
  else
  {
    LBA = GetClusterLBA (&pFATFS->aLogicDisks[pDirImage->DiskNo].BPB,
      pDirImage->CurrentClusterNo) + pDirImage->CurrentSectorNo;
  }

  res = DiskReadSectorLBA
  (
    pFATFS,
    pDirImage->DiskNo,
    LBA,
    1,
    pDirImage->aSector
  );

  return res;
}

static int DirImageRefill (tCacheEntry* pCacheEntry, void* pReuseInfo)
{
  tDirImage* pDirImage = (tDirImage*)pCacheEntry->pAttachedData;
  tDirImageReuseInfo* pImageReuseInfo = (tDirImageReuseInfo*)pReuseInfo;
  tFATFS* pFATFS = pDirImage->pFATFS;
  int DiskNo = pImageReuseInfo->DiskNo;
  tIORes res;

  // Don't refill a valid reusable entry:
  if (Cache_GetIsValid (pCacheEntry) && DirImageIsReusable (pCacheEntry, pReuseInfo))
    return 1;

  // Load Dir image from offcache storage:
  pDirImage->DiskNo = DiskNo;
  pDirImage->CurrentClusterNo = pImageReuseInfo->CurrentClusterNo;
  pDirImage->CurrentSectorNo  = pImageReuseInfo->CurrentSectorNo;
  res = ReadDirImage (pFATFS, pDirImage);

  Cache_SetErrorCode (pCacheEntry, res);

  // Return value (refill success flag) will affect Cache_GetIsValid():
  return res == ior_OK;
}

// InitReadDirState() must be called before first ReadDirEntry() call
static void InitReadDirState (tFATFS* pFATFS, tReadDirState* pReadDirState,
                              uint8 DiskNo, uint32 DirCluster)
{
  pReadDirState->DiskNo = DiskNo;
  pReadDirState->VolumeSerialNumber = pFATFS->aLogicDisks[DiskNo].VolumeSerialNumber;
  pReadDirState->FirstClusterNo = DirCluster;
  pReadDirState->CurrentClusterNo = pReadDirState->FirstClusterNo;
  pReadDirState->CurrentSectorNo = 0;
  pReadDirState->SectorPosition = 0;
  pReadDirState->IsPermanentError = 0;
  pReadDirState->ErrorCode = ior_OK;
  pReadDirState->IsEndReached = 0;

  // FAT32 root directory has nonzero cluster no:
  if
  (
    (!pReadDirState->CurrentClusterNo) &&
    (pFATFS->aLogicDisks[DiskNo].FATType == ft_FAT32)
  )
    pReadDirState->CurrentClusterNo =
      pFATFS->aLogicDisks[DiskNo].BPB.BPB2.FAT32.RootDirectoryClusterNo;
}

// AdvanceDirPosition() may be called between subsequent
// ReadDirEntry() and WriteDirEntry() calls to move forward/backward
// in the directory.
static tIORes AdvanceDirPosition (tFATFS* pFATFS, tReadDirState* pReadDirState,
                                  int AdvanceAmount/*-1,0,+1*/)
{
  tReadDirState ReadDirState;
  tIORes res;

  (void)pFATFS;

  if (!AdvanceAmount)
    return ior_OK;

  // Have a local copy of the state to work on to avoid restoring:
  ReadDirState = *pReadDirState;

  if (AdvanceAmount > 0)
  {
    // Can't go beyond the end of directory $$$ can be adjusted for growing dirs
    if (ReadDirState.IsEndReached)
      return ior_NOT_FOUND;

    // Advance position within sector:
    ReadDirState.SectorPosition += sizeof(tFATDirectoryEntry);

    if (ReadDirState.SectorPosition >= FAT_SECTOR_SIZE)
    {
      // Try advancing to the next sector:
      ReadDirState.CurrentSectorNo++;
      ReadDirState.SectorPosition = 0;

      if (!ReadDirState.CurrentClusterNo)
      {
        // Check for an attempt to read past the end of fixed-size
        // FAT12/16 root directory:
        if (ReadDirState.CurrentSectorNo >=
            GetRootDirSectorsCount(&pFATFS->aLogicDisks[ReadDirState.DiskNo].BPB))
        {
          // Don't advance the position (it will be handy to have a valid last
          // position when going backwards), just mark:
          pReadDirState->IsEndReached = 1;
          return ior_OK;
        }
      }
      else // elseof if (!ReadDirState.CurrentClusterNo)
      {
        // Check for an attempt to read past the end of the current cluster:
        if (ReadDirState.CurrentSectorNo >=
            pFATFS->aLogicDisks[ReadDirState.DiskNo].BPB.BPB1.SectorsPerCluster)
        {
          uint32 NextClusterNo;
          res = GetFATEntry (pFATFS, ReadDirState.DiskNo,
                             ReadDirState.CurrentClusterNo, &NextClusterNo);
          if (res != ior_OK)
            return res;

          switch (GetFATEntryType(pFATFS->aLogicDisks[ReadDirState.DiskNo].FATType,
                                  NextClusterNo))
          {
            case fet_CLUSTER:
              break;

            case fet_LAST_CLUSTER:
              // Don't advance the position (it will be handy to have a valid last
              // position when going backwards), just mark:
              pReadDirState->IsEndReached = 1;
              return ior_OK;

            default:
              // FAT error (FAT or dir entry damaged):
              LOG_ERR0();
              return ior_FS_DAMAGED;
          }

          ReadDirState.CurrentClusterNo = NextClusterNo;
          ReadDirState.CurrentSectorNo = 0;
        }
      } // endof if (!ReadDirState.CurrentClusterNo)
    } // endof if (ReadDirState.SectorPosition >= FAT_SECTOR_SIZE)
  }
  else // elseof if (AdvanceAmount > 0)
  {
    if (ReadDirState.IsEndReached)
    {
      // Don't advance the position (here it's handy to have a valid last
      // position), just unmark:
      pReadDirState->IsEndReached = 0;
      return ior_OK;
    }

    // Advance position within sector:
    if (ReadDirState.SectorPosition)
      ReadDirState.SectorPosition -= sizeof(tFATDirectoryEntry);
    else
    {
      // Try advancing to the previous sector:
      ReadDirState.SectorPosition = FAT_SECTOR_SIZE-sizeof(tFATDirectoryEntry);

      if (!ReadDirState.CurrentSectorNo--)
      {
        if (!ReadDirState.CurrentClusterNo)
        {
          // Check for an attempt to read before the beginning of
          // a fixed-size FAT12/16 root directory:
          return ior_NOT_FOUND;
        }
        else // elseof if (!ReadDirState.CurrentClusterNo)
        {
          // Check for an attempt to read before the beginning of
          // the current cluster:
          uint32 PrevClusterNo;
          // Check if were at the beginning of the directory:
          if (ReadDirState.CurrentClusterNo == ReadDirState.FirstClusterNo)
            return ior_NOT_FOUND;
          // Check if were at the beginning of a FAT32 root directory:
          if (ReadDirState.CurrentClusterNo ==
              pFATFS->aLogicDisks[ReadDirState.DiskNo].BPB.BPB2.FAT32.RootDirectoryClusterNo)
            return ior_NOT_FOUND;

          res = FindPrevFATEntry (pFATFS, ReadDirState.DiskNo,
                                  ReadDirState.FirstClusterNo,
                                  ReadDirState.CurrentClusterNo, &PrevClusterNo);
          if (res != ior_OK)
            return res;

          switch (GetFATEntryType (pFATFS->aLogicDisks[ReadDirState.DiskNo].FATType,
                                   PrevClusterNo))
          {
            case fet_CLUSTER:
              break;

            default:
              // FAT error (FAT damaged):
              LOG_ERR0();
              return ior_FS_DAMAGED;
          }

          ReadDirState.CurrentClusterNo = PrevClusterNo;
          ReadDirState.CurrentSectorNo =
            pFATFS->aLogicDisks[ReadDirState.DiskNo].BPB.BPB1.SectorsPerCluster - 1;
        } // elseof if (!ReadDirState.CurrentClusterNo)
      } // endof if (!ReadDirState.CurrentSectorNo--)
    } // endof if (ReadDirState.SectorPosition)
  } // endof if (AdvanceAmount < 0)

  // Having successfully advanced the position, update the state:
  *pReadDirState = ReadDirState;

  return ior_OK;
}

// Helper function for ReadDirEntry() and WriteDirEntry()
static tIORes ReadDirFindCacheEntry (tFATFS* pFATFS,
                                     tReadDirState* pReadDirState,
                                     tCacheEntry** ppCacheEntry)
{
  tDirImageReuseInfo DirImageReuseInfo;
  tIORes res = ior_OK;

  // Don't work with directories in state of permanent error:
  if (pReadDirState->IsPermanentError)
    return pReadDirState->ErrorCode;

  // Check VolumeSerialNumber change:
  if (pReadDirState->VolumeSerialNumber !=
      pFATFS->aLogicDisks[pReadDirState->DiskNo].VolumeSerialNumber)
  {
    pReadDirState->ErrorCode = ior_ERR_MEDIA_CHANGED;
    pReadDirState->IsPermanentError = IsPermanentError (pReadDirState->ErrorCode);
    return pReadDirState->ErrorCode;
  }

  // Find needed cache entry:
  DirImageReuseInfo.DiskNo           = pReadDirState->DiskNo;
  DirImageReuseInfo.CurrentClusterNo = pReadDirState->CurrentClusterNo;
  DirImageReuseInfo.CurrentSectorNo  = pReadDirState->CurrentSectorNo;
  *ppCacheEntry = Cache_FindEntryForUse
  (
    pFATFS->aDirCacheEntries,
    MAX_DIR_IMAGES_AT_ONCE,
    &DirImageReuseInfo,
    &DirImageRefill,
    &DirImageFlush,
    &DirImageIsReusable
  );

  // Check validity of found entry:
  if (!Cache_GetIsValid (*ppCacheEntry))
    res = (tIORes)Cache_GetErrorCode (*ppCacheEntry);

  // If it was the first read on this disk, update the serial number
  // in the structure:
  if (res == ior_OK)
  {
    pReadDirState->VolumeSerialNumber =
      pFATFS->aLogicDisks[pReadDirState->DiskNo].VolumeSerialNumber;
  }
  else
  {
    LOG_ERR2("%u", (uint)res);
  }

  return res;
}

// InitReadDirState() must be called before first ReadDirEntry() call
// AdvanceDirPosition() may be called between subsequent
// ReadDirEntry() calls to move forward/backward in the directory
static tIORes ReadDirEntry (tFATFS* pFATFS, tReadDirState* pReadDirState,
                            tFATDirectoryEntry* pDirEntry,
                            int AdvanceAmount/*-1,0,1*/)
{
  tCacheEntry* pCacheEntry;
  tDirImage* pDirImage;
  tIORes res;

  if (pReadDirState->IsEndReached)
    return ior_NOT_FOUND;

  res = ReadDirFindCacheEntry (pFATFS, pReadDirState, &pCacheEntry);

  pReadDirState->ErrorCode = res;
  pReadDirState->IsPermanentError = IsPermanentError (pReadDirState->ErrorCode);

  if (res == ior_OK)
  {
    // Return attached data
    pDirImage = (tDirImage*)pCacheEntry->pAttachedData;

    // Return dir entry:
    if ((pReadDirState->SectorPosition >= FAT_SECTOR_SIZE) ||
        (pReadDirState->SectorPosition % sizeof(tFATDirectoryEntry)))
    {
      LOG_ERR0();
      res = ior_MEMORY_CORRUPTED;
      pReadDirState->ErrorCode = res;
      pReadDirState->IsPermanentError = IsPermanentError (pReadDirState->ErrorCode);
      return res;
    }
    *pDirEntry = *(tFATDirectoryEntry*)&pDirImage->aSector[pReadDirState->SectorPosition];

    res = AdvanceDirPosition (pFATFS, pReadDirState, AdvanceAmount);
  }

  return res;
}

// InitReadDirState() must be called before first WriteDirEntry() call
// AdvanceDirPosition() may be called between subsequent
// WriteDirEntry() calls to move forward/backward in the directory
static tIORes WriteDirEntry (tFATFS* pFATFS, tReadDirState* pReadDirState,
                             const tFATDirectoryEntry* pDirEntry,
                             int AdvanceAmount/*-1,0,1*/)
{
  tCacheEntry* pCacheEntry;
  tDirImage* pDirImage;
  tIORes res;

  if (pReadDirState->IsEndReached)
    return ior_NOT_FOUND;

  res = ReadDirFindCacheEntry (pFATFS, pReadDirState, &pCacheEntry);

  pReadDirState->ErrorCode = res;
  pReadDirState->IsPermanentError = IsPermanentError (pReadDirState->ErrorCode);

  if (res == ior_OK)
  {
    tFATDirectoryEntry* pFATDirectoryEntry;
    // Return attached data
    if ((pReadDirState->SectorPosition >= FAT_SECTOR_SIZE) ||
        (pReadDirState->SectorPosition % sizeof(tFATDirectoryEntry)))
    {
      LOG_ERR0();
      res = ior_MEMORY_CORRUPTED;
      pReadDirState->ErrorCode = res;
      pReadDirState->IsPermanentError = IsPermanentError (pReadDirState->ErrorCode);
      return res;
    }
    pDirImage = (tDirImage*)pCacheEntry->pAttachedData;

    pFATDirectoryEntry =
      (tFATDirectoryEntry*)&pDirImage->aSector[pReadDirState->SectorPosition];

    // Set dir entry:
    *pFATDirectoryEntry = *pDirEntry;
    Cache_SetIsDirty (pCacheEntry);

    res = AdvanceDirPosition (pFATFS, pReadDirState, AdvanceAmount);
  }

  return res;
}

static tIORes CreateNewDirEntry (tFATFS* pFATFS, uint8 DiskNo, uint32 DirClusterNo,
                                 const tFATDirectoryEntry* pDirEntry,
                                 tReadDirState* pReadDirState)
{
  tFATDirectoryEntry DirEntry;
  tIORes res;
  int found = 0, zeroingNeeded = 0;

  InitReadDirState (pFATFS, pReadDirState, DiskNo, DirClusterNo);

  while
  (
    (res = ReadDirEntry (pFATFS, pReadDirState, &DirEntry, 1)) == ior_OK
  )
  {
    if
    (
      (!DirEntry.Name[0]) ||
      ((uchar)DirEntry.Name[0] == DELETED_DIR_ENTRY_MARKER)
    )
    {
      res = AdvanceDirPosition (pFATFS, pReadDirState, -1);
      if (res != ior_OK)
        return res;
      found = 1;
      break;
    }
  }

  if ((res != ior_OK) && (res != ior_NOT_FOUND))
    return res;

  if (!found)
  {
    // no vacant directory entry found, need to grow the directory:
    uint32 NextClusterNo;

    // can't grow fixed-size FAT12/16 root directories:
    if
    (
      (!pReadDirState->CurrentClusterNo) &&
      (
        (pFATFS->aLogicDisks[DiskNo].FATType == ft_FAT12) ||
        (pFATFS->aLogicDisks[DiskNo].FATType == ft_FAT16)
      )
    )
      return ior_NO_SPACE;

    res = GrowClusterChain (pFATFS,
                            DiskNo,
                            &pReadDirState->FirstClusterNo,
                            NULL,
                            1);
    if (res != ior_OK)
      return res;

    res = GetFATEntry (pFATFS, DiskNo,
                       pReadDirState->CurrentClusterNo, &NextClusterNo);
    // this would be a weird error, not sure what to do:
    if (res != ior_OK)
    {
      LOG_ERR2("%u", (uint)res);
      return res;
    }

    switch (GetFATEntryType(pFATFS->aLogicDisks[DiskNo].FATType,
                            NextClusterNo))
    {
      case fet_CLUSTER:
        break;

      default:
        // this would be a weird error too,
        // FAT error (FAT or dir entry damaged):
        LOG_ERR2("%u", (uint)res);
        return ior_FS_DAMAGED;
    }

    // Advance the position in the directory:
    pReadDirState->CurrentClusterNo = NextClusterNo;
    pReadDirState->CurrentSectorNo = 0;
    pReadDirState->SectorPosition = 0;

    // Clear the flag saying that the end of the directory has been reached:
    pReadDirState->IsEndReached = 0;

    // the end of the grown directory needs zeroing:
    zeroingNeeded = 1;
  }

  // now, write the newly created directory entry:
  res = WriteDirEntry (pFATFS, pReadDirState, pDirEntry, 0);
  if (res != ior_OK)
    return res;

  // zero up the rest of the last cluster of the grown directory:
  if (zeroingNeeded)
  {
    tReadDirState ReadDirState = *pReadDirState;
    memset (&DirEntry, 0, sizeof(DirEntry));
    res = AdvanceDirPosition (pFATFS, &ReadDirState, 1);
    // this would be a weird error:
    if (res != ior_OK)
    {
      LOG_ERR2("%u", (uint)res);
      return res;
    }
    while
    (
      (res = WriteDirEntry (pFATFS, &ReadDirState, &DirEntry, 1)) == ior_OK
    );
  }

  if ((res != ior_OK) && (res != ior_NOT_FOUND))
    return res;

  return ior_OK;
}

// GetCurDirRecursive() is called only from FAT_getcwd()
static char* GetCurDirRecursive (tFATFS* pFATFS, uint8 DiskNo, uint32 DirClusterNo,
                                 char* pBuf, char* pAfterBuf, tIORes* pErrorCode)
{
  tReadDirState ReadDirState;
  tFATDirectoryEntry DirEntry;
  uint32 ThisDirClusterNo=0, UpDirClusterNo=0;
  int IsRoot=1;
  int i;
  char* p;

  InitReadDirState (pFATFS, &ReadDirState, DiskNo, DirClusterNo);

  *pErrorCode = ReadDirEntry (pFATFS, &ReadDirState, &DirEntry, 1);
  if (*pErrorCode != ior_OK)
    return pBuf;

  if
  (
    (DirEntry.Name[0] == '.') &&
    (DirEntry.Name[1] == ' ') &&
    (DirEntry.Attribute & dea_DIRECTORY) &&
    (DirEntry.Attribute != dea_LONG_NAME)
  )
  {
    ThisDirClusterNo = GetDirEntryFirstClusterNo (&DirEntry);

    *pErrorCode = ReadDirEntry (pFATFS, &ReadDirState, &DirEntry, 1);
    if (*pErrorCode != ior_OK)
      return pBuf;

    if
    (
      (DirEntry.Name[0] == '.') &&
      (DirEntry.Name[1] == '.') &&
      (DirEntry.Attribute & dea_DIRECTORY) &&
      (DirEntry.Attribute != dea_LONG_NAME)
    )
    {
      UpDirClusterNo = GetDirEntryFirstClusterNo (&DirEntry);

      InitReadDirState (pFATFS, &ReadDirState, DiskNo, UpDirClusterNo);
      while
      (
        (*pErrorCode = ReadDirEntry (pFATFS, &ReadDirState, &DirEntry, 1)) ==
        ior_OK
      )
        if
        (
          (DirEntry.Name[0]) &&
          ((uchar)DirEntry.Name[0] != DELETED_DIR_ENTRY_MARKER) &&
          (DirEntry.Attribute & dea_DIRECTORY) &&
          (DirEntry.Attribute != dea_LONG_NAME) &&
          (GetDirEntryFirstClusterNo (&DirEntry) == ThisDirClusterNo)
        )
        {
          IsRoot = 0;
          break;
        }
        else 
          if (!DirEntry.Name[0])
            break;
      if (*pErrorCode != ior_OK)
        return pBuf;
      if (IsRoot)
      {
        *pErrorCode = ior_NOT_FOUND;
        return pBuf;
      }
    }
    else
    {
      // FAT error (FAT or dir entry damaged because ".." must follow "."):
      LOG_ERR0();
      *pErrorCode = ior_FS_DAMAGED;
      return pBuf;
    }
  }

  if (!IsRoot)
  {
    p = GetCurDirRecursive
    (
      pFATFS,
      DiskNo,
      UpDirClusterNo,
      pBuf,
      pAfterBuf,
      pErrorCode
    );

    if (*pErrorCode != ior_OK)
      return pBuf;

    if (p < pAfterBuf)
      *p++ = '\\';

    for (i=0;i<8;i++)
      if ((p < pAfterBuf) && (DirEntry.Name[i] != ' '))
        *p++ = DirEntry.Name[i];
      else
        break;
    if ((p < pAfterBuf) && (DirEntry.Extension[0] != ' '))
      *p++ = '.';
    for (i=0;i<3;i++)
      if ((p < pAfterBuf) && (DirEntry.Extension[i] != ' '))
        *p++ = DirEntry.Extension[i];
      else
        break;

    if (p < pAfterBuf)
      *p = 0;
  }
  else
  {
    p = pBuf;

    if (p < pAfterBuf)
      *p++ = 'A'+DiskNo;
    if (p < pAfterBuf)
      *p++ = ':';

    if (p < pAfterBuf)
      *p = '\\';

    if (p+1 < pAfterBuf)
      p[1] = 0;
  }

  return p;
}

char* FAT_getcwd (tFATFS* pFATFS, tIORes* pIORes,
                  char* pBuf, size_t BufLen)
{
  int DiskNo;

  if (pIORes == NULL)
    return NULL;

  if (pFATFS == NULL)
  {
    *pIORes = ior_INVALID_ARGUMENT;
    return NULL;
  }

  DiskNo = pFATFS->CurrentLogicDiskNo;
  *pIORes = ior_OK;

  // The buffer must be allocated by the caller:
  if (pBuf == NULL)
  {
    *pIORes = ior_NO_MEM;
    return NULL;
  }
  if (!BufLen)
  {
    *pIORes = ior_ARGUMENT_OUT_OF_RANGE;
    return NULL;
  }

  // Control use of the buffer:
  pBuf[BufLen-1] = 0;

  GetCurDirRecursive
  (
    pFATFS,
    DiskNo,
    pFATFS->aLogicDisks[DiskNo].CurrentDirClusterNo,
    pBuf,
    pBuf+BufLen,
    pIORes
  );

  if (*pIORes != ior_OK)
    return NULL;

  if (pBuf[BufLen-1])
  {
    // Buffer was too small:
    *pIORes = ior_ARGUMENT_OUT_OF_RANGE;
    return NULL;
  }

  return pBuf;
}

typedef struct tFindDirEntryRes {
  // general-purpose info:
  uint8                 DiskNo;
  uint32                DirClusterNo;
  tReadDirState         ReadDirState;
  tFATDirectoryEntry    DirEntry;
  // info for file/dir creation:
  const char*           pLastNamePart;
} tFindDirEntryRes;

static int IsHarmlessLastDot (const char* pName, int len)
{
  switch (len)
  {
    case 0:     // nothing to check
    case 1:     // single-letter name or dot, can't drop last dot
      return 0;
    case 2:
      if ((pName[0] == '.') && (pName[1] == '.')) // can't drop dot from dot-dot
        return 0;
    default:
      return pName[len-1] == '.';
  }
}

// returned pDirEntry->Name[0] will be 0 for root dir
static tIORes FindDirectoryEntry (tFATFS* pFATFS, const char* pPath,
                                  tFindDirEntryRes* pFindDirEntryRes)
{
/*
  Path format diagram:

     /------------>[ \ ]-----\
     |              ^ ^      |
     |              | |      v
  [begin]-->[D:]----*-|--->[end]
     |              | |      ^
     |              v v      |
     \------------>[8.3]-----/

  In other words:
  - the path begins with either of [D:] and [\] and [8.3]
  - [D:] may be followed by either of [\] and [8.3]
  - [\] may be followed by [8.3]
  - [8.3] may be followed by [\]
  - the path ends   with either of [D:] and [\] and [8.3]

  - [D:] is a disk letter (from A to Z) followed by a colon character
  - [\] is a backslash character
  - [8.3] - standard DOS' 8 dot 3 filename format (including . and ..)

  - [D:] changes current disk to the (new) one specified
  - [\] separates (sub/up)directory and file names;
    [\] in the beginning (or immediately after [D:]) changes current directory
    to the root of the current (possibly new) disk
  - [8.3] is a (sub/up)directory or file name;
    [8.3] when equal to . (dot) means staying in the current directory,
    [8.3] when equal to .. (dot-dot) means going up to the parent (up)directory
*/
  enum {
    s_START,
    s_LETTER,
    s_COLON,
    s_SLASH
  } state = s_START;

  enum {
    es_INVALID,
    es_FINISHED_NOT_FOUND,
    es_FINISHED_FOUND,
    es_NOT_FOUND_BEFORE_SLASH,
    es_FINISHED_AFTER_COLON,
    es_FINISHED_AFTER_SLASH
  } endState = es_INVALID;

  tIORes                Res;

  tFATDirectoryEntry    DirEntry;
  tReadDirState         ReadDirState;

  int                   FirstLetterIndex, LastLetterIndex;

  int                   DiskNo;
  uint32                DirClusterNo;

  int                   PathLen, i;

  int                   IsBreakFromLoopNeeded = 0;
  const char*           pLastNamePart = NULL;

  uint32                DirClusterNoToSearch;

  // zero up the structure to be returned:
  memset (pFindDirEntryRes, 0, sizeof(tFindDirEntryRes));

  // Will try to start from the current disk's root directory:
  DiskNo = pFATFS->CurrentLogicDiskNo;
  DirClusterNo = pFATFS->aLogicDisks[DiskNo].CurrentDirClusterNo;

  if (pPath == NULL)
    return ior_BAD_NAME;

  PathLen = strlen(pPath);
  if (!PathLen)
    return ior_BAD_NAME;

  for (i=0; i<=PathLen; i++)
  {
    switch (state)
    {
      // Just started:
      case s_START:
        if (pPath[i] == '\\')
        {
          // Will start from the current disk's root directory:
          DiskNo = pFATFS->CurrentLogicDiskNo;
          DirClusterNo = GetRootDirClusterNo(&pFATFS->aLogicDisks[DiskNo].BPB);
          state = s_SLASH;
        }
        else
        {
          LastLetterIndex = FirstLetterIndex = i;
          state = s_LETTER;
        }
      break; // case s_START

      // After a letter:
      case s_LETTER:
        if ((pPath[i] == '\\') || !pPath[i])
        {
          int found = 0;

          // Simulate existence of dot-entry (".") in the root directory:
          if
          (
            (IsRootDirCluster(&pFATFS->aLogicDisks[DiskNo].BPB,DirClusterNo)) &&
            (LastLetterIndex == FirstLetterIndex) &&
            (pPath[LastLetterIndex] == '.')
          )
          {
            if (!pPath[i])
            {
              // this case is the same as endloop #5:
              state = s_SLASH;
              endState = es_FINISHED_AFTER_SLASH;
            }
            else
            {
              // continue as if dot entry really existed in the root directory:
              DirClusterNo =
                GetRootDirClusterNo (&pFATFS->aLogicDisks[DiskNo].BPB);
              state = s_SLASH;
            }
            break; // case s_LETTER:
          }

          // Now, try finding dir or file by next part of the path:
          InitReadDirState (pFATFS, &ReadDirState, DiskNo, DirClusterNo);
          while
          (
            (Res=ReadDirEntry (pFATFS, &ReadDirState, &DirEntry, 1)) == ior_OK
          )
          {
            int d, len, j;
            char aName[13];

            // Compare names:
            if (!DirEntry.Name[0])
            {
              // break the dir search loop if end of dir found
              // (neither dir nor file found):
              break; // while()
            }
            // skip deleted entries and parts of long names:
            if
            (
              ((uchar)DirEntry.Name[0] == DELETED_DIR_ENTRY_MARKER) ||
              (DirEntry.Attribute == dea_LONG_NAME)
            )
              continue; // while()
            DirEntryNameToASCIIZ (&DirEntry, aName);
            len = strlen (aName);
            // if (!len) break;
            // ignore harmless last dot (if present) and compare lengths:
            d = LastLetterIndex-FirstLetterIndex+1 -
                IsHarmlessLastDot (pPath+FirstLetterIndex,
                                   LastLetterIndex-FirstLetterIndex+1) - len;
            // skip if name lengths differ:
            if (d)
              continue; // while()

            found = 1;
            // compare names:
            for (j=0; j<len; j++)
              if (upcase(pPath[FirstLetterIndex+j]) != aName[j])
              {
                found = 0;
                break; // for()
              }

            // break the dir search loop if dir or file found;
            if (found)
              break; // while()
          }
          // return on error:
          if ((Res != ior_OK) && (Res != ior_NOT_FOUND))
            return Res;

          if (!pPath[i])
          {
            // stopped at the end:
            if (!found)
            {
              // tell the last name part for which nothing was found:
              pLastNamePart = pPath + FirstLetterIndex;
              // endloop #1
              endState = es_FINISHED_NOT_FOUND;
            }
            else
            {
              // endloop #2
              endState = es_FINISHED_FOUND;
            }
            break; // case s_LETTER
          }

          if (!found)
          {
            // name part before the slash wasn't found,
            // breaking the main loop early:
            IsBreakFromLoopNeeded = 1;
            // endloop #3
            endState = es_NOT_FOUND_BEFORE_SLASH;
            break; // case s_LETTER
          }

          // A dir or file was found, find out what...
          if (DirEntry.Attribute & dea_DIRECTORY)
          {
            // if it's a dir and there's something after the slash,
            // will try to change to it:
            if (pPath[i+1])
            {
              DirClusterNo = GetDirEntryFirstClusterNo (&DirEntry);
              // correct cluster no of a dir if dir is root
              // (if we followed a dot-dot):
              if (!DirClusterNo)
                DirClusterNo =
                  GetRootDirClusterNo (&pFATFS->aLogicDisks[DiskNo].BPB);
              state = s_SLASH;
            }
            else
            {
              // it's a dir and there's nothing after the slash,
              // breaking out of the main loop early:
              IsBreakFromLoopNeeded = 1;
              // endloop #2
              endState = es_FINISHED_FOUND;
              break; // case s_LETTER
            }
          }
          else
          {
            // the path contains a file name and a slash after it,
            // as if it were a dir, failing with error:
            return ior_BAD_NAME;
          }
        } // if ((pPath[i] == '\\') || !pPath[i])
        else if
        (
          (pPath[i] == ':') &&
          (i == 1)
        )
        {
          // New disk specified:
          DiskNo = (uchar)upcase(pPath[i-1]) - 'A';
          if (DiskNo >= MAX_LOGIC_DISKS)
            return ior_BAD_NAME;
          if (!pFATFS->aLogicDisks[DiskNo].Exists)
            return ior_BAD_NAME;
          // Will try to start from new disk's current directory:
          DirClusterNo = pFATFS->aLogicDisks[DiskNo].CurrentDirClusterNo;
          state = s_COLON;
        }
        else
        {
          LastLetterIndex = i;
        }
      break; // case s_LETTER

      // After a colon:
      case s_COLON:
        if (pPath[i] == '\\')
        {
          // Will start from new disk's root directory:
          DirClusterNo = GetRootDirClusterNo(&pFATFS->aLogicDisks[DiskNo].BPB);
          state = s_SLASH;
        }
        else if (pPath[i])
        {
          LastLetterIndex = FirstLetterIndex = i;
          state = s_LETTER;
        }
        else
        {
          // endloop #4
          endState = es_FINISHED_AFTER_COLON;
        }
      break; // case s_COLON

      // After a slash:
      case s_SLASH:
        if (pPath[i])
        {
          LastLetterIndex = FirstLetterIndex = i;
          state = s_LETTER;
        }
        else
        {
          // endloop #5
          endState = es_FINISHED_AFTER_SLASH;
        }
      break; // case s_SLASH
    }

    if (IsBreakFromLoopNeeded)
      break;
  }

  // handle all endloop cases here:
  // endloop #1: es_FINISHED_NOT_FOUND
  //            state==s_LETTER, pPath[i]==0,
  //            IsBreakFromLoopNeeded==0, pLastNamePart!=NULL, found==0
  // endloop #2: es_FINISHED_FOUND
  //            state==s_LETTER, pPath[i]==0,
  //            IsBreakFromLoopNeeded==0, pLastNamePart==NULL, found==1
  // endloop #3: es_NOT_FOUND_BEFORE_SLASH
  //            state==s_LETTER, pPath[i]=='\',
  //            IsBreakFromLoopNeeded==1, pLastNamePart==NULL, found==0
  // endloop #4: es_FINISHED_AFTER_COLON
  //            state==s_COLON,  pPath[i]==0,
  //            IsBreakFromLoopNeeded==0, pLastNamePart==NULL, found==X
  // endloop #5: es_FINISHED_AFTER_SLASH
  //            state==s_SLASH,  pPath[i]==0,
  //            IsBreakFromLoopNeeded==0, pLastNamePart==NULL, found==X
  switch (endState)
  {
    case es_NOT_FOUND_BEFORE_SLASH:
      if (pPath[i+1])
      {
        // last processed '\' wasn't the last character, so the path is
        // wrong somewhere in the middle, failing with error:
        return ior_BAD_NAME;
      }
      // else (stopped at the last '\') fall through to es_FINISHED_NOT_FOUND:
      // tell the last name part for which nothing was found:
      pLastNamePart = pPath + FirstLetterIndex;

    case es_FINISHED_NOT_FOUND:
      // only the last part of the path name doesn't exist:
      pFindDirEntryRes->DiskNo = DiskNo;
      pFindDirEntryRes->DirClusterNo = DirClusterNo;
      //pFindDirEntryRes->ReadDirState
      //pFindDirEntryRes->DirEntry
      pFindDirEntryRes->pLastNamePart = pLastNamePart;
    return ior_NOT_FOUND;

    case es_FINISHED_FOUND:
      // the pathname exists and corresponds to an existing file or dir
      // (don't handle dot and dot-dot here):
      if
      (
        (!(DirEntry.Attribute & dea_DIRECTORY)) ||
        (DirEntry.Name[0] != '.')
      )
      {
        AdvanceDirPosition (pFATFS, &ReadDirState, -1);
        pFindDirEntryRes->DiskNo = DiskNo;
        pFindDirEntryRes->DirClusterNo = DirClusterNo;
        pFindDirEntryRes->ReadDirState = ReadDirState;
        pFindDirEntryRes->DirEntry = DirEntry;
        pFindDirEntryRes->pLastNamePart = NULL;
        return (DirEntry.Attribute & dea_DIRECTORY) ?
               // existing directory was found (neither dot, nor dot-dot):
               ior_DIR_FOUND:
               // existing file was found:
               ior_FILE_FOUND;
      }
      // fall through to es_FINISHED_AFTER_COLON if
      // it's a dot or dot-dot dir:

    case es_FINISHED_AFTER_COLON:
      // the pathname exists and corresponds to an existing directory:
      if
      (!(
        (
          (endState == es_FINISHED_AFTER_COLON) &&
          (IsRootDirCluster(&pFATFS->aLogicDisks[DiskNo].BPB,DirClusterNo))
        ) ||
        (
          (endState == es_FINISHED_FOUND) &&
          (IsRootDirCluster(&pFATFS->aLogicDisks[DiskNo].BPB,DirClusterNo)) &&
          (DirEntry.Name[0] == '.') && (DirEntry.Name[1] == ' ')
        )
      ))
      {
        break;
      }
      // fall through to es_FINISHED_AFTER_SLASH if
      // it's a root dir:

    case es_FINISHED_AFTER_SLASH:
lRoot:
      // the pathname exists and corresponds to an existing root directory:
      pFindDirEntryRes->DiskNo = DiskNo;
      pFindDirEntryRes->DirClusterNo =
        GetRootDirClusterNo (&pFATFS->aLogicDisks[DiskNo].BPB);
      //pFindDirEntryRes->ReadDirState = ReadDirState;
      SetDirEntryFirstClusterNo (&pFindDirEntryRes->DirEntry, pFindDirEntryRes->DirClusterNo);
      // Root dir cannot be deleted, so it's read-only, mark as dir as well:
      pFindDirEntryRes->DirEntry.Attribute = dea_READ_ONLY | dea_DIRECTORY;
      // Root dir has no date and time, so setting it to the earliest
      // possible DOS date&time, just in case someone needs it valid:
      SetDirEntryWriteDateAndTime
      (
        &pFindDirEntryRes->DirEntry,
        1980, 1, 1,
        0, 0, 0
      );
      pFindDirEntryRes->pLastNamePart = NULL;
    return ior_DIR_FOUND;

    case es_INVALID:
    default:
      // error, shouldn't happen, just in case:
      LOG_ERR0();
    return ior_MEMORY_CORRUPTED;
  }

  if (endState == es_FINISHED_AFTER_COLON)
  {
    // D: found, find dot entry for it:
    InitReadDirState (pFATFS, &ReadDirState, DiskNo, DirClusterNo);
    Res = ReadDirEntry (pFATFS, &ReadDirState, &DirEntry, 1);
    // return on error:
    if (Res != ior_OK)
      return Res;
    if
    (!(
      (DirEntry.Name[0] == '.') &&
      (DirEntry.Name[1] == ' ')
    ))
    {
      // FAT is damaged because dot entry must be first in non-root directory:
      LOG_ERR0();
      return ior_FS_DAMAGED;
    }
  }
  else if ((DirEntry.Name[0] == '.') && (DirEntry.Name[1] == '.'))
  {
    // dot-dot entry found, go to updir and find dot entry for it:
    DirClusterNo = GetDirEntryFirstClusterNo (&DirEntry);
    if (IsRootDirCluster(&pFATFS->aLogicDisks[DiskNo].BPB,DirClusterNo))
      goto lRoot;
    InitReadDirState (pFATFS, &ReadDirState, DiskNo, DirClusterNo);
    Res = ReadDirEntry (pFATFS, &ReadDirState, &DirEntry, 1);
    // return on error:
    if (Res != ior_OK)
      return Res;
    if
    (!(
      (DirEntry.Name[0] == '.') &&
      (DirEntry.Name[1] == ' ')
    ))
    {
      // FAT is damaged because dot entry must be first in non-root directory:
      LOG_ERR0();
      return ior_FS_DAMAGED;
    }
  }

  // dot entry found, find the 8.3 name for it:
  DirClusterNoToSearch = DirClusterNo;

  Res = ReadDirEntry (pFATFS, &ReadDirState, &DirEntry, 1);
  // return on error:
  if (Res != ior_OK)
    return Res;
  if
  (!(
    (DirEntry.Name[0] == '.') &&
    (DirEntry.Name[1] == '.')
  ))
  {
    // FAT is damaged because dot-dot entry must be second in non-root directory:
    LOG_ERR0();
    return ior_FS_DAMAGED;
  }

  DirClusterNo = GetDirEntryFirstClusterNo (&DirEntry);
  InitReadDirState (pFATFS, &ReadDirState, DiskNo, DirClusterNo);
  while
  (
    (Res=ReadDirEntry (pFATFS, &ReadDirState, &DirEntry, 1)) == ior_OK
  )
  {
    if
    (
      ((uchar)DirEntry.Name[0] != DELETED_DIR_ENTRY_MARKER) &&
      (DirEntry.Attribute & dea_DIRECTORY) &&
      (DirEntry.Attribute != dea_LONG_NAME) &&
      (GetDirEntryFirstClusterNo (&DirEntry) == DirClusterNoToSearch)
    )
    {
      AdvanceDirPosition (pFATFS, &ReadDirState, -1);
      pFindDirEntryRes->DiskNo = DiskNo;
      pFindDirEntryRes->DirClusterNo = DirClusterNo;
      pFindDirEntryRes->ReadDirState = ReadDirState;
      pFindDirEntryRes->DirEntry = DirEntry;
      pFindDirEntryRes->pLastNamePart = NULL;
      return ior_DIR_FOUND;
    }
  }

  // return on error:
  if (Res != ior_OK)
    return Res;

  // FAT is damaged because there's no link to a directory,
  // whose dot entry was found:
  LOG_ERR0();
  return ior_FS_DAMAGED;
}

int FAT_opendir (tFATFS* pFATFS, tIORes* pIORes,
                 const char* pPath, tReadDirState* pReadDirState)
{
  tFindDirEntryRes FindDirEntryRes;

  if (pIORes == NULL)
    return 0;

  if ((pFATFS == NULL) || (pPath == NULL) || (pReadDirState == NULL))
  {
    *pIORes = ior_INVALID_ARGUMENT;
    return 0;
  }

  // Find directory entry for specified directory:
  *pIORes = FindDirectoryEntry (pFATFS, pPath, &FindDirEntryRes);
  if (*pIORes != ior_DIR_FOUND)
  {
    *pReadDirState = FindDirEntryRes.ReadDirState;
    return 0;
  }

  InitReadDirState
  (
    pFATFS,
    &FindDirEntryRes.ReadDirState,
    FindDirEntryRes.DiskNo,
    GetDirEntryFirstClusterNo (&FindDirEntryRes.DirEntry)
  );
  *pReadDirState = FindDirEntryRes.ReadDirState;

  *pIORes = ior_OK;
  return 1;
}

int FAT_closedir (tFATFS* pFATFS, tIORes* pIORes,
                  tReadDirState* pReadDirState)
{
  if (pIORes == NULL)
    return 0;

  if ((pFATFS == NULL) || (pReadDirState == NULL))
  {
    *pIORes = ior_INVALID_ARGUMENT;
    return 0;
  }

  *pIORes = ior_OK;
  return 1;
}

int FAT_rewinddir (tFATFS* pFATFS, tIORes* pIORes,
                   tReadDirState* pReadDirState)
{
  if (pIORes == NULL)
    return 0;

  if ((pFATFS == NULL) || (pReadDirState == NULL))
  {
    *pIORes = ior_INVALID_ARGUMENT;
    return 0;
  }

  InitReadDirState (pFATFS, pReadDirState,
                    pReadDirState->DiskNo, pReadDirState->FirstClusterNo);
  *pIORes = ior_OK;
  return 1;
}

int FAT_readdir (tFATFS* pFATFS, tIORes* pIORes,
                 tReadDirState* pReadDirState, tFATDirectoryEntry* pDirEntry)
{
  if (pIORes == NULL)
    return 0;

  if ((pFATFS == NULL) || (pReadDirState == NULL) || (pDirEntry == NULL))
  {
    *pIORes = ior_INVALID_ARGUMENT;
    return 0;
  }

  while ((*pIORes=ReadDirEntry (pFATFS, pReadDirState, pDirEntry, 1)) == ior_OK)
  {
    if (!pDirEntry->Name[0])
    {
      *pIORes = ior_NO_MORE_FILES;
      break;
    }
    if
    (
      ((uchar)pDirEntry->Name[0] != DELETED_DIR_ENTRY_MARKER) &&
      (pDirEntry->Attribute != dea_LONG_NAME)
    )
      return 1;
  }

  if (*pIORes == ior_NOT_FOUND)
    *pIORes = ior_NO_MORE_FILES;

  return 0;
}

int FAT_chdir (tFATFS* pFATFS, tIORes* pIORes,
               const char* pPath, int IsDiskChangeAllowed)
{
  tFindDirEntryRes FindDirEntryRes;

  if (pIORes == NULL)
    return 0;

  if ((pFATFS == NULL) || (pPath == NULL))
  {
    *pIORes = ior_INVALID_ARGUMENT;
    return 0;
  }

  // Find directory entry for specified directory:
  *pIORes = FindDirectoryEntry (pFATFS, pPath, &FindDirEntryRes);
  if (*pIORes != ior_DIR_FOUND)
    return 0;

  pFATFS->aLogicDisks[FindDirEntryRes.DiskNo].CurrentDirClusterNo =
    GetDirEntryFirstClusterNo (&FindDirEntryRes.DirEntry);

  // Stupid MSDOS and MSWindows won't change the current disk
  // if the path contains a disk letter, so the change is made optionally:
  if (IsDiskChangeAllowed)
    pFATFS->CurrentLogicDiskNo = FindDirEntryRes.DiskNo;

  *pIORes = ior_OK;
  return 1;
}

void FAT_dos_setdrive (tFATFS* pFATFS, tIORes* pIORes, uint DiskNo, uint* pTotalDisks)
{
  int DiskCount = 0, i;

  if (pIORes == NULL)
    return;

  if (pFATFS == NULL)
  {
    *pIORes = ior_INVALID_ARGUMENT;
    return;
  }

  *pIORes = ior_ARGUMENT_OUT_OF_RANGE;

  if (!DiskNo--)
  {
    return;
  }

  for (i=0; i<MAX_LOGIC_DISKS; i++)
  {
    if (pFATFS->aLogicDisks[i].Exists)
    {
      DiskCount++;
      if (i == DiskNo)
        *pIORes = ior_OK;
    }
  }

  if (*pIORes == ior_OK)
    pFATFS->CurrentLogicDiskNo = DiskNo;

  if (pTotalDisks != NULL)
    *pTotalDisks = DiskCount;
}

void FAT_dos_getdrive (tFATFS* pFATFS, tIORes* pIORes, uint* pDiskNo)
{
  if (pDiskNo != NULL)
    *pDiskNo = 1;

  if (pIORes == NULL)
    return;

  if ((pFATFS == NULL) || (pDiskNo == NULL))
  {
    *pIORes = ior_INVALID_ARGUMENT;
    return;
  }
          
  *pIORes = ior_OK;
  *pDiskNo = 1 + pFATFS->CurrentLogicDiskNo;
}

int FAT_mkdir (tFATFS* pFATFS, tIORes* pIORes, const char* pPath, tModeT mode)
{
  tFindDirEntryRes FindDirEntryRes;
  int DiskNo;
  uint32 ClusterNo, i, LBA;
  tFATDirectoryEntry DirEntry;
  uchar sector[FAT_SECTOR_SIZE];

  if (pIORes == NULL)
    return 0;

  if ((pFATFS == NULL) || (pPath == NULL))
  {
    *pIORes = ior_INVALID_ARGUMENT;
    return 0;
  }

  // Find directory entry for specified pathname:
  *pIORes = FindDirectoryEntry (pFATFS, pPath, &FindDirEntryRes);

  // Fail if error or anything found by specified pathname:
  if (*pIORes != ior_NOT_FOUND)
    return 0;

  DiskNo = FindDirEntryRes.DiskNo;

  // Fail if path is wrong:
  if (FindDirEntryRes.pLastNamePart == NULL)
  {
    // shouldn't happen, just in case:
    LOG_ERR0();
    *pIORes = ior_MEMORY_CORRUPTED;
    return 0;
  }

  // Fail if name is wrong:
  if
  (
    !_8Dot3NameToDirEntryName
    ( 
      FindDirEntryRes.pLastNamePart,
      &FindDirEntryRes.DirEntry,
      1
    )
  )
  {
    *pIORes = ior_BAD_NAME;
    return 0;
  }

  // Now, try allocating a cluster:
  *pIORes = FindNextFreeFATEntry (pFATFS, DiskNo, &ClusterNo);
  if (*pIORes != ior_OK)
    return 0;

  // Mark the cluster found free as being a last cluster:
  *pIORes = SetFATEntry
  (
    pFATFS,
    DiskNo,
    ClusterNo,
    GetLastClusterMarkValue(pFATFS->aLogicDisks[DiskNo].FATType)
  );
  if (*pIORes != ior_OK)
    return 0;

  // Fill the directory entry with some basic info:
  FindDirEntryRes.DirEntry.Attribute = dea_DIRECTORY;
  if (!(mode & mt_S_IWUSR))
    FindDirEntryRes.DirEntry.Attribute |= dea_READ_ONLY;
  FindDirEntryRes.DirEntry.WinNTreserved = 0;
  FindDirEntryRes.DirEntry.CreationTimeSecTenths = 0;  // ignore
  FindDirEntryRes.DirEntry.CreationTime2Secs = 0;      // ignore
  FindDirEntryRes.DirEntry.CreationDate = 0;           // ignore
  FindDirEntryRes.DirEntry.LastAccessDate = 0;         // ignore
  {
    uint Year, Month, Day, Hour, Minute, Second;
    pFATFS->pGetDateAndTime
    (
      pFATFS,
      &Year, &Month, &Day,
      &Hour, &Minute, &Second
    );
    SetDirEntryWriteDateAndTime
    (
      &FindDirEntryRes.DirEntry,
      Year, Month, Day,
      Hour, Minute, Second
    );
  }
  SetDirEntryFirstClusterNo (&FindDirEntryRes.DirEntry, ClusterNo);
  FindDirEntryRes.DirEntry.Size = 0;                   // zero-length file

  // Fill the cluster:
  LBA = GetClusterLBA (&pFATFS->aLogicDisks[DiskNo].BPB, ClusterNo);

  memset (sector, 0, sizeof(sector));

  DirEntry = FindDirEntryRes.DirEntry;
  memcpy (DirEntry.Name, ".       ", 8);
  memset (DirEntry.Extension, ' ', 3);
  // set this(dot) dir cluster no:
  SetDirEntryFirstClusterNo (&DirEntry, ClusterNo);
  ((tFATDirectoryEntry*)sector)[0] = DirEntry;

  DirEntry.Name[1] = '.';
  // set parent(dot-dot) dir cluster no (awlays must set to 0 if parent is root, even on FAT32):
  if (IsRootDirCluster (&pFATFS->aLogicDisks[DiskNo].BPB, FindDirEntryRes.DirClusterNo))
    SetDirEntryFirstClusterNo (&DirEntry, 0);
  else
    SetDirEntryFirstClusterNo (&DirEntry, FindDirEntryRes.DirClusterNo);
  ((tFATDirectoryEntry*)sector)[1] = DirEntry;

  // Fill in the cluster with all entries empty but the dot and dot-dot:
  for
  (
    i = 0;
    i < pFATFS->aLogicDisks[DiskNo].BPB.BPB1.SectorsPerCluster;
    i++, LBA++
  )
  {
    *pIORes = DiskWriteSectorLBA
    (
      pFATFS,
      DiskNo,
      LBA,
      1,
      sector
    );
    if (*pIORes != ior_OK)
      break;

    if (!i)
      memset (sector, 0, 2*sizeof(tFATDirectoryEntry));
  }

  // Finally, try to create a new directory entry:
  if (*pIORes == ior_OK)
  {
    *pIORes = CreateNewDirEntry
    (
      pFATFS,
      FindDirEntryRes.DiskNo,
      FindDirEntryRes.DirClusterNo,
      &FindDirEntryRes.DirEntry,
      &FindDirEntryRes.ReadDirState
    );
  }

  // Free the allocated cluster if failed to create the directory entry
  if (*pIORes != ior_OK)
  {
    tIORes res = SetFATEntry (pFATFS, DiskNo, ClusterNo, 0);
    // ignore possible error here:
    if (res != ior_OK)
      LOG_ERR0();
  }

  return *pIORes != ior_OK;
}

int FAT_rmdir (tFATFS* pFATFS, tIORes* pIORes, const char* pPath)
{
  tFindDirEntryRes FindDirEntryRes;
  tReadDirState ReadDirState;
  tFATDirectoryEntry DirEntry;
  uint32 FirstDirCluster;
  int IsEmpty=1;

  if (pIORes == NULL)
    return 0;

  if ((pFATFS == NULL) || (pPath == NULL))
  {
    *pIORes = ior_INVALID_ARGUMENT;
    return 0;
  }

  // Find directory entry for specified pathname:
  *pIORes = FindDirectoryEntry (pFATFS, pPath, &FindDirEntryRes);

  // Fail if file found by that name or there's some other error:
  if (*pIORes != ior_DIR_FOUND)
    return 0;

  FirstDirCluster = GetDirEntryFirstClusterNo (&FindDirEntryRes.DirEntry);

  // Cannot delete the root and current directory:
  if
  (
    (IsRootDirCluster (&pFATFS->aLogicDisks[FindDirEntryRes.DiskNo].BPB,
                       FirstDirCluster)) ||
    (pFATFS->aLogicDisks[FindDirEntryRes.DiskNo].CurrentDirClusterNo ==
     FirstDirCluster)
  )
  {
    *pIORes = ior_PROHIBITED;
    return 0;
  }

  // Fail deleting read only directories:
  if (FindDirEntryRes.DirEntry.Attribute & dea_READ_ONLY)
  {
    *pIORes = ior_PROHIBITED;
    return 0;
  }

  // Check whether the directory is empty:
  InitReadDirState
  (
    pFATFS,
    &ReadDirState,
    FindDirEntryRes.DiskNo,
    FirstDirCluster
  );
  while ((*pIORes=ReadDirEntry(pFATFS, &ReadDirState, &DirEntry, 1)) == ior_OK)
  {
    if (!DirEntry.Name[0])
      break;
    if
    (
      (DirEntry.Name[0] == '.') ||
      ((uchar)DirEntry.Name[0] == DELETED_DIR_ENTRY_MARKER) ||
      (DirEntry.Attribute == dea_LONG_NAME)
    )
      continue;
    IsEmpty = 0;
    break;
  }

  // Check if some error prevented reading till the end of the directory:
  if ((*pIORes != ior_NOT_FOUND) && (*pIORes != ior_OK))
    return 0;

  if (!IsEmpty)
  {
    // the dir isn't empty, fail:
    *pIORes = ior_PROHIBITED;
    return 0;
  }

  // Update the directory entry by marking the entry as deleted:
  FindDirEntryRes.DirEntry.Name[0] = DELETED_DIR_ENTRY_MARKER;
  *pIORes = WriteDirEntry
  (
    pFATFS,
    &FindDirEntryRes.ReadDirState,
    &FindDirEntryRes.DirEntry,
    0
  );
  if (*pIORes != ior_OK)
    return 0;

  // Truncate the cluster chain:
  *pIORes = TruncateClusterChain
  (
    pFATFS,
    FindDirEntryRes.DiskNo,
    FirstDirCluster,
    0
  );

  return (*pIORes == ior_OK) ? 1 : 0;
}

static const tTypeAndString aFileOpenModeTypeAndString[] =
{
  {fom_FAIL_IF_NOT_EXIST,                                            "r"  },
  {fom_FAIL_IF_NOT_EXIST,                                            "rb" },
  {fom_FAIL_IF_NOT_EXIST,                                            "rt" },
  {fom_FAIL_IF_NOT_EXIST|fom_OPEN_FOR_WRITE,                         "r+" },
  {fom_FAIL_IF_NOT_EXIST|fom_OPEN_FOR_WRITE,                         "rb+"},
  {fom_FAIL_IF_NOT_EXIST|fom_OPEN_FOR_WRITE,                         "rt+"},
  {fom_FAIL_IF_NOT_EXIST|fom_OPEN_FOR_WRITE,                         "r+b"},
  {fom_FAIL_IF_NOT_EXIST|fom_OPEN_FOR_WRITE,                         "r+t"},
  {fom_CREATE_IF_NOT_EXIST|fom_TRUNCATE_IF_EXIST|fom_OPEN_FOR_WRITE, "w"  },
  {fom_CREATE_IF_NOT_EXIST|fom_TRUNCATE_IF_EXIST|fom_OPEN_FOR_WRITE, "wb" },
  {fom_CREATE_IF_NOT_EXIST|fom_TRUNCATE_IF_EXIST|fom_OPEN_FOR_WRITE, "wt" },
  {fom_CREATE_IF_NOT_EXIST|fom_TRUNCATE_IF_EXIST|fom_OPEN_FOR_WRITE, "w+" },
  {fom_CREATE_IF_NOT_EXIST|fom_TRUNCATE_IF_EXIST|fom_OPEN_FOR_WRITE, "wb+"},
  {fom_CREATE_IF_NOT_EXIST|fom_TRUNCATE_IF_EXIST|fom_OPEN_FOR_WRITE, "wt+"},
  {fom_CREATE_IF_NOT_EXIST|fom_TRUNCATE_IF_EXIST|fom_OPEN_FOR_WRITE, "w+b"},
  {fom_CREATE_IF_NOT_EXIST|fom_TRUNCATE_IF_EXIST|fom_OPEN_FOR_WRITE, "w+t"}
};

tFATFile* FAT_fopen (tFATFS* pFATFS, tIORes* pIORes,
                     tFATFile* pFATFile, const char* filename, const char* mode)
{
  tFindDirEntryRes FindDirEntryRes;
  int i, OpenMode=-1;
  tFATFile* pOtherFATFile;

  if (pIORes == NULL)
    return NULL;

  if ((pFATFS == NULL) || (filename == NULL) || (mode == NULL))
  {
    *pIORes = ior_INVALID_ARGUMENT;
    return NULL;
  }

  // The buffer must be allocated by the caller:
  if (pFATFile == NULL)
  {
    *pIORes = ior_NO_MEM;
    return NULL;
  }

  for (i=0; i<GetArrayElementCount(aFileOpenModeTypeAndString); i++)
    if (!strcmp (aFileOpenModeTypeAndString[i].pString, mode))
    {
      OpenMode = aFileOpenModeTypeAndString[i].Type;
      break;
    }

  // Fail if invalid open mode:
  if (OpenMode < 0)
  {
    *pIORes = ior_INVALID_ARGUMENT;
    return NULL;
  }

  pFATFile->OpenMode = (tFileOpenMode)OpenMode;

  // Find directory entry for specified file:
  *pIORes = FindDirectoryEntry (pFATFS, filename, &FindDirEntryRes);

  // Fail if file not found (when required):
  if ((*pIORes == ior_NOT_FOUND) && (OpenMode & fom_FAIL_IF_NOT_EXIST))
    return NULL;

  // Fail if directory found by that name or there's some other error:
  if ((*pIORes != ior_FILE_FOUND) && (*pIORes != ior_NOT_FOUND))
    return NULL;

  // Can't open volume IDs:
  if ((*pIORes == ior_FILE_FOUND) &&
      (FindDirEntryRes.DirEntry.Attribute & dea_VOLUME_ID))
  {
    *pIORes = ior_PROHIBITED;
    return NULL;
  }

  // Fail opening read only files for write:
  if
  (
    (*pIORes == ior_FILE_FOUND) &&
    (OpenMode & fom_OPEN_FOR_WRITE) &&
    (FindDirEntryRes.DirEntry.Attribute & dea_READ_ONLY)
  )
  {
    *pIORes = ior_PROHIBITED;
    return NULL;
  }

  // Fail opening existing files that are already open:
  if
  (
    (*pIORes == ior_FILE_FOUND) &&
    (
      (pOtherFATFile = pFATFS->pOpenFileSearch
      (
        pFATFS,
        &FindDirEntryRes.ReadDirState,
        &FAT_OpenFilesMatch
      )) != NULL
    )
  )
  {
    // Allow opening a file several times for read only:
    if ((pFATFile->OpenMode != fom_FAIL_IF_NOT_EXIST) ||
        (pOtherFATFile->OpenMode != fom_FAIL_IF_NOT_EXIST))
    {
      *pIORes = ior_PROHIBITED;
      return NULL;
    }
  }

  // Create new file if needed:
  if ((*pIORes == ior_NOT_FOUND) && (OpenMode & fom_CREATE_IF_NOT_EXIST))
  {
    // Fail if path is wrong:
    if (FindDirEntryRes.pLastNamePart == NULL)
    {
      // shouldn't happen, just in case:
      LOG_ERR0();
      *pIORes = ior_MEMORY_CORRUPTED;
      return NULL;
    }

    // Fail if filename is wrong:
    if
    (
      !_8Dot3NameToDirEntryName
      ( 
        FindDirEntryRes.pLastNamePart,
        &FindDirEntryRes.DirEntry,
        0
      )
    )
    {
      *pIORes = ior_BAD_NAME;
      return NULL;
    }

    // Fill the directory entry with some basic info:
    FindDirEntryRes.DirEntry.Attribute = dea_ARCHIVE;    // archive=1 by default
    FindDirEntryRes.DirEntry.WinNTreserved = 0;
    FindDirEntryRes.DirEntry.CreationTimeSecTenths = 0;  // ignore
    FindDirEntryRes.DirEntry.CreationTime2Secs = 0;      // ignore
    FindDirEntryRes.DirEntry.CreationDate = 0;           // ignore
    FindDirEntryRes.DirEntry.LastAccessDate = 0;         // ignore
    {
      uint Year, Month, Day, Hour, Minute, Second;
      pFATFS->pGetDateAndTime
      (
        pFATFS,
        &Year, &Month, &Day,
        &Hour, &Minute, &Second
      );
      SetDirEntryWriteDateAndTime
      (
        &FindDirEntryRes.DirEntry,
        Year, Month, Day,
        Hour, Minute, Second
      );
    }
    SetDirEntryFirstClusterNo (&FindDirEntryRes.DirEntry, 0);// zero-length file
    FindDirEntryRes.DirEntry.Size = 0;                   // zero-length file

    // Try to create a new file:
    *pIORes = CreateNewDirEntry
    (
      pFATFS,
      FindDirEntryRes.DiskNo,
      FindDirEntryRes.DirClusterNo,
      &FindDirEntryRes.DirEntry,
      &FindDirEntryRes.ReadDirState
    );
    if (*pIORes != ior_OK)
      return NULL;
  }
  else if ((*pIORes == ior_FILE_FOUND) && (OpenMode & fom_TRUNCATE_IF_EXIST))
  // Truncate existing file if needed:
  {
    // Truncate the cluster chain:
    *pIORes = TruncateClusterChain
    (
      pFATFS,
      FindDirEntryRes.DiskNo,
      GetDirEntryFirstClusterNo (&FindDirEntryRes.DirEntry),
      0
    );
    if (*pIORes != ior_OK)
      return NULL;
    // Update the directory entry:
    {
      uint Year, Month, Day, Hour, Minute, Second;
      pFATFS->pGetDateAndTime
      (
        pFATFS,
        &Year, &Month, &Day,
        &Hour, &Minute, &Second
      );
      SetDirEntryWriteDateAndTime
      (
        &FindDirEntryRes.DirEntry,
        Year, Month, Day,
        Hour, Minute, Second
      );
    }
    SetDirEntryFirstClusterNo (&FindDirEntryRes.DirEntry, 0);
    FindDirEntryRes.DirEntry.Size = 0;
    *pIORes = WriteDirEntry
    (
      pFATFS,
      &FindDirEntryRes.ReadDirState,
      &FindDirEntryRes.DirEntry,
      0
    );
    if (*pIORes != ior_OK)
      return NULL;
  }

  // Init file structure:
  pFATFile->IsOpen = 1;
  pFATFile->BufferedDataState = fbds_INVALID;
  pFATFile->IsTimeUpdateNeeded = 0;
  pFATFile->DiskNo = FindDirEntryRes.DiskNo;
  pFATFile->VolumeSerialNumber =
    pFATFS->aLogicDisks[FindDirEntryRes.DiskNo].VolumeSerialNumber;
  pFATFile->FileSize = FindDirEntryRes.DirEntry.Size;
  pFATFile->FilePosition = 0;
  pFATFile->LastClusterNo = 0;
  pFATFile->CurClusterOrdinalNo = 0;
  pFATFile->FirstClusterNo =
    GetDirEntryFirstClusterNo (&FindDirEntryRes.DirEntry);
  pFATFile->CurrentClusterNo = 0;
  pFATFile->CurrentSectorNo = 0;
  pFATFile->SectorPosition = 0;
  pFATFile->DirClusterNo = FindDirEntryRes.DirClusterNo;
  pFATFile->ReadDirState = FindDirEntryRes.ReadDirState;
  pFATFile->DirectoryEntry = FindDirEntryRes.DirEntry;
  pFATFile->IsPermanentError = 0;
  pFATFile->ErrorCode = ior_OK;

  // Seek to the beginning of the file:
  FAT_fseek (pFATFS, pIORes, pFATFile, 0);

  return (*pIORes == ior_OK) ? pFATFile : NULL;
}

int FAT_fclose (tFATFS* pFATFS, tIORes* pIORes,
                tFATFile* pFATFile)
{
  int res;

  if (pIORes == NULL)
    return 0;

  if ((pFATFS == NULL) || (pFATFile == NULL))
  {
    *pIORes = ior_INVALID_ARGUMENT;
    return 0;
  }

  if (!pFATFile->IsOpen)
  {
    *pIORes = ior_INVALID_ARGUMENT;
    return 0;
  }

  res = FAT_fflush (pFATFS, pIORes, pFATFile);
  if (*pIORes != ior_OK)
    goto lend;

  if (pFATFile->IsTimeUpdateNeeded)
  {
    uint Year, Month, Day, Hour, Minute, Second;
    pFATFS->pGetDateAndTime
    (
      pFATFS,
      &Year, &Month, &Day,
      &Hour, &Minute, &Second
    );
    SetDirEntryWriteDateAndTime
    (
      &pFATFile->DirectoryEntry,
      Year, Month, Day,
      Hour, Minute, Second
    );
    *pIORes = pFATFile->ErrorCode =
      WriteDirEntry
      (
        pFATFS,
        &pFATFile->ReadDirState,
        &pFATFile->DirectoryEntry,
        0
      );
    if (*pIORes != ior_OK)
    {
      res = 0;
      goto lend;
    }
  }

lend:
  // mark as closed anyway, even on error:
  if (*pIORes != ior_OK)
    LOG_ERR2 ("%u", (uint)res);
  pFATFile->IsOpen = 0;
  return res;
}

int FAT_fflush (tFATFS* pFATFS, tIORes* pIORes,
                tFATFile* pFATFile)
{
  (void)pFATFS;

  if (pIORes == NULL)
    return 0;

  if ((pFATFS == NULL) || (pFATFile == NULL))
  {
    *pIORes = ior_INVALID_ARGUMENT;
    return 0;
  }

  if (!pFATFile->IsOpen)
  {
    *pIORes = ior_INVALID_ARGUMENT;
    return 0;
  }

  // Don't work with files in state of permanent error:
  if (pFATFile->IsPermanentError)
  {
    *pIORes = pFATFile->ErrorCode;
    return 0;
  }

  if (pFATFile->BufferedDataState != fbds_VALID_UNFLUSHED)
  {
    // No changes or nothing to flush:
    *pIORes = pFATFile->ErrorCode = ior_OK;
    return 1;
  }

  // Now, write the sector:
  pFATFile->ErrorCode = *pIORes = DiskWriteSectorLBA
  (
    pFATFS,
    pFATFile->DiskNo,
    GetClusterLBA(&pFATFS->aLogicDisks[pFATFile->DiskNo].BPB,
      pFATFile->CurrentClusterNo)+pFATFile->CurrentSectorNo,
    1,
    pFATFile->aSector
  );

  pFATFile->IsPermanentError = IsPermanentError (pFATFile->ErrorCode);

  if (*pIORes != ior_OK)
    return 0;

  pFATFile->BufferedDataState = fbds_VALID;

  return 1;
}

static tIORes GrowFile (tFATFS* pFATFS, tFATFile* pFATFile, uint32 FileGrowthAmount)
{
  tIORes res = ior_OK;
  tFATDirectoryEntry DirEntry;
  uint8 SectorsPerCluster;
  uint32 BytesPerCluster;
  uint32 FirstClusterNo, LastClusterNo, ClusterChainGrowthAmount;

  if (!FileGrowthAmount)
    return res;

  // since the API has 32-bit values for the file size and position,
  // growing beyond 2**32-1 isn't allowed
  if (0xFFFFFFFFUL - pFATFile->FileSize < FileGrowthAmount)
    return ior_ARGUMENT_OUT_OF_RANGE;

  SectorsPerCluster =
    pFATFS->aLogicDisks[pFATFile->DiskNo].BPB.BPB1.SectorsPerCluster;

  BytesPerCluster = (uint32)SectorsPerCluster * FAT_SECTOR_SIZE;

  LastClusterNo = pFATFile->LastClusterNo;

  // If the file's last cluster is known, don't search for it
  // from the very beginning in GrowClusterChain():
  if (LastClusterNo)
    FirstClusterNo = LastClusterNo;
  else
    FirstClusterNo = pFATFile->FirstClusterNo;

  if (!SectorsPerCluster)
  {
    LOG_ERR0();
    return ior_MEMORY_CORRUPTED;
  }

  ClusterChainGrowthAmount = (pFATFile->FileSize + FileGrowthAmount) /
                               BytesPerCluster -
                             pFATFile->FileSize / BytesPerCluster;

  ClusterChainGrowthAmount += ((pFATFile->FileSize + FileGrowthAmount) %
                               BytesPerCluster) != 0;

  ClusterChainGrowthAmount -= (pFATFile->FileSize % BytesPerCluster) != 0;

  // Try growing the file's cluster chain if needed:
  if (ClusterChainGrowthAmount)
  {
    res = pFATFile->ErrorCode =
      GrowClusterChain (pFATFS,
                        pFATFile->DiskNo,
                        &FirstClusterNo,
                        &LastClusterNo,
                        ClusterChainGrowthAmount);

    pFATFile->IsPermanentError = IsPermanentError (pFATFile->ErrorCode);

    if (res != ior_OK)
      return res;
  }

  // Now, try updating the corresponding directory entry:
  DirEntry = pFATFile->DirectoryEntry;
  DirEntry.Size = pFATFile->FileSize + FileGrowthAmount;
  if (!pFATFile->FirstClusterNo)
  {
    // We've grown a zero-length file.
    // Must now set non-zero first cluster value:
    SetDirEntryFirstClusterNo (&DirEntry, FirstClusterNo);
  }

  res = pFATFile->ErrorCode =
    WriteDirEntry (pFATFS, &pFATFile->ReadDirState, &DirEntry, 0);
  pFATFile->IsPermanentError = IsPermanentError (pFATFile->ErrorCode);

  if (res != ior_OK)
    return res;

  // Now, update file information:
  if (!pFATFile->FirstClusterNo)
  {
    // Initialize file structure a bit more,
    // if the file had been zero-length before we grew it:
    pFATFile->FirstClusterNo = FirstClusterNo;
    pFATFile->CurrentClusterNo = FirstClusterNo;
    pFATFile->CurrentSectorNo = 0;
  }
  pFATFile->DirectoryEntry = DirEntry;
  pFATFile->FileSize = DirEntry.Size;
  pFATFile->LastClusterNo = LastClusterNo;

  return res;
}

// FAT_fseek also reads in file data into the file's sector buffer and
// whenever needed flushes data written to the sector buffer
int FAT_fseek (tFATFS* pFATFS, tIORes* pIORes,
               tFATFile* pFATFile, uint32 Position)
{
  uint32 CurrentClusterNo, CurrentSectorNo;
  uint16 SectorPosition;
  uint8 SectorsPerCluster;
  uint32 ClusterPos;
  tIORes res;

  if (pIORes == NULL)
    return 0;

  if ((pFATFS == NULL) || (pFATFile == NULL))
  {
    *pIORes = ior_INVALID_ARGUMENT;
    return 0;
  }

  if (!pFATFile->IsOpen)
  {
    *pIORes = ior_INVALID_ARGUMENT;
    return 0;
  }

  // Don't work with files in state of permanent error:
  if (pFATFile->IsPermanentError)
  {
    *pIORes = pFATFile->ErrorCode;
    return 0;
  }

  // Check VolumeSerialNumber change:
  if (pFATFile->VolumeSerialNumber !=
      pFATFS->aLogicDisks[pFATFile->DiskNo].VolumeSerialNumber)
  {
    // Error, media changed:
    *pIORes = pFATFile->ErrorCode = ior_ERR_MEDIA_CHANGED;
    pFATFile->IsPermanentError = IsPermanentError (pFATFile->ErrorCode);
    return 0;
  }

  // Handle seeking past the last byte of the file:
  if (Position > pFATFile->FileSize)
  {
    if (!(pFATFile->OpenMode & fom_OPEN_FOR_WRITE))
    {
      // If the file is open for reading,
      // can't seek farther than one byte after the last byte of the file:
      *pIORes = pFATFile->ErrorCode = ior_ARGUMENT_OUT_OF_RANGE;
      pFATFile->IsPermanentError = IsPermanentError (pFATFile->ErrorCode);
      return 0;
    }
    else
    {
      // Flush the file if seeking to a different sector:
      if ((Position^pFATFile->FilePosition) >= FAT_SECTOR_SIZE)
      {
        FAT_fflush (pFATFS, pIORes, pFATFile);
        if (*pIORes != ior_OK)
          return 0;
      }

      // Try growing the file if needed:
      *pIORes = GrowFile (pFATFS, pFATFile, Position - pFATFile->FileSize);

      if (*pIORes != ior_OK)
        return 0;

      pFATFile->IsTimeUpdateNeeded = 1;

      // invalidate the buffered data because a different sector needed:
      if ((Position^pFATFile->FilePosition) >= FAT_SECTOR_SIZE)
        pFATFile->BufferedDataState = fbds_INVALID;
      pFATFile->FilePosition = Position;
      pFATFile->SectorPosition = Position & (FAT_SECTOR_SIZE-1);
      return 1;
    }
  }

  // Handle seeking precisely at one byte after the last byte of the file:
  if (Position == pFATFile->FileSize)
  {
    if ((Position^pFATFile->FilePosition) >= FAT_SECTOR_SIZE)
    {
      FAT_fflush (pFATFS, pIORes, pFATFile);
      if (*pIORes != ior_OK)
        return 0;
      // invalidate the buffered data because a different sector needed:
      pFATFile->BufferedDataState = fbds_INVALID;
    }
    pFATFile->FilePosition = Position;
    pFATFile->SectorPosition = Position & (FAT_SECTOR_SIZE-1);
    *pIORes = pFATFile->ErrorCode = ior_OK;
    return 1;
  }

  // If data are valid and the same sector is needed, we're done:
  if
  (
    (pFATFile->BufferedDataState != fbds_INVALID) &&
    ((Position^pFATFile->FilePosition) < FAT_SECTOR_SIZE)
  )
  {
    pFATFile->FilePosition = Position;
    pFATFile->SectorPosition = Position & (FAT_SECTOR_SIZE-1);
    *pIORes = pFATFile->ErrorCode = ior_OK;
    return 1;
  }

  // Different sector needed (or data are invalid), flush the file:
  FAT_fflush (pFATFS, pIORes, pFATFile);
  if (*pIORes != ior_OK)
    return 0;

  SectorsPerCluster =
    pFATFS->aLogicDisks[pFATFile->DiskNo].BPB.BPB1.SectorsPerCluster;

  if (!SectorsPerCluster)
  {
    LOG_ERR0();
    return ior_MEMORY_CORRUPTED;
  }

  // Different sector needed (or data are invalid), calculate its location:
  SectorPosition = Position & (FAT_SECTOR_SIZE-1);
  CurrentSectorNo = Position / FAT_SECTOR_SIZE;
  ClusterPos = CurrentSectorNo / SectorsPerCluster;
  CurrentSectorNo %= SectorsPerCluster;
  CurrentClusterNo = pFATFile->FirstClusterNo;

  // Optimize forward seeking by following the cluster chain from the last
  // known (current) file cluster:
  if ((ClusterPos >= pFATFile->CurClusterOrdinalNo) && 
      pFATFile->CurClusterOrdinalNo)
  {
    CurrentClusterNo = pFATFile->CurrentClusterNo;
    ClusterPos -= pFATFile->CurClusterOrdinalNo;
  }

  // Follow the cluster chain, if needed:
  while (ClusterPos--)
  {
    uint32 NextClusterNo;
    res = GetFATEntry (pFATFS,pFATFile->DiskNo,CurrentClusterNo,&NextClusterNo);
    if (res == ior_OK)
    {
      if (GetFATEntryType (pFATFS->aLogicDisks[pFATFile->DiskNo].FATType,
                           NextClusterNo) == fet_CLUSTER)
        CurrentClusterNo = NextClusterNo;
      else
      {
        // FAT error (FAT or dir entry of file damaged -- wrong cluster):
        LOG_ERR0();
        *pIORes = pFATFile->ErrorCode = ior_FS_DAMAGED;
        pFATFile->IsPermanentError = IsPermanentError (res);
        return 0;
      }
    }
    else
    {
      // Some FAT read error:
      *pIORes = pFATFile->ErrorCode = res;
      pFATFile->IsPermanentError = IsPermanentError (res);
      return 0;
    }
  }

  // Now, read the needed sector:
  *pIORes = res = DiskReadSectorLBA
  (
    pFATFS,
    pFATFile->DiskNo,
    GetClusterLBA(&pFATFS->aLogicDisks[pFATFile->DiskNo].BPB,CurrentClusterNo)+
      CurrentSectorNo,
    1,
    pFATFile->aSector
  );

  pFATFile->BufferedDataState = (res == ior_OK) ? fbds_VALID : fbds_INVALID;

  if (res == ior_OK)
  {
    pFATFile->CurrentClusterNo = CurrentClusterNo;
    pFATFile->CurrentSectorNo = CurrentSectorNo;
    pFATFile->SectorPosition = SectorPosition;
    pFATFile->FilePosition = Position;
    pFATFile->CurClusterOrdinalNo = Position /
      (FAT_SECTOR_SIZE * (uint32)SectorsPerCluster);
  }

  pFATFile->ErrorCode = res;
  pFATFile->IsPermanentError = IsPermanentError (pFATFile->ErrorCode);

  return res == ior_OK;
}

void FAT_rewind (tFATFS* pFATFS, tIORes* pIORes,
                 tFATFile* pFATFile)
{
  FAT_fseek (pFATFS, pIORes, pFATFile, 0);
}

int FAT_ftell (tFATFS* pFATFS, tIORes* pIORes,
               tFATFile* pFATFile, uint32* pPosition)
{
  if (pPosition != NULL)
    *pPosition = 0;

  if (pIORes == NULL)
    return 0;

  if ((pFATFS == NULL) || (pFATFile == NULL) || (pPosition == NULL))
  {
    *pIORes = ior_INVALID_ARGUMENT;
    return 0;
  }

  if (!pFATFile->IsOpen)
  {
    *pIORes = ior_INVALID_ARGUMENT;
    return 0;
  }

  // Don't work with files in state of permanent error:
  if (pFATFile->IsPermanentError)
  {
    *pIORes = pFATFile->ErrorCode;
    return 0;
  }

  *pPosition = pFATFile->FilePosition;

  *pIORes = pFATFile->ErrorCode = ior_OK;
  return 1;
}

int FAT_feof (tFATFS* pFATFS, tIORes* pIORes,
              tFATFile* pFATFile, int* pIsEOF)
{
  uint32 Position;
  int res = FAT_ftell (pFATFS, pIORes, pFATFile, &Position);

  if (pIsEOF != NULL)
  {
    *pIsEOF = 1;
    if (res)
      *pIsEOF = Position >= pFATFile->FileSize;
  }
  else
    res = 0;

  return res;
}

int FAT_fsize (tFATFS* pFATFS, tIORes* pIORes,
               tFATFile* pFATFile, uint32* pSize)
{
  if (pSize != NULL)
    *pSize = 0;

  if (pIORes == NULL)
    return 0;

  if ((pFATFS == NULL) || (pFATFile == NULL) || (pSize == NULL))
  {
    *pIORes = ior_INVALID_ARGUMENT;
    return 0;
  }

  if (!pFATFile->IsOpen)
  {
    *pIORes = ior_INVALID_ARGUMENT;
    return 0;
  }

  // Don't work with files in state of permanent error:
  if (pFATFile->IsPermanentError)
  {
    *pIORes = pFATFile->ErrorCode;
    return 0;
  }

  *pSize = pFATFile->FileSize;

  *pIORes = pFATFile->ErrorCode = ior_OK;
  return 1;
}

size_t FAT_fread (tFATFS* pFATFS, tIORes* pIORes,
                  void* ptr, size_t size, size_t n, tFATFile* pFATFile)
{
  size_t BytesToRead;
  size_t BytesRead=0;

  if (pIORes == NULL)
    return 0;

  if ((pFATFS == NULL) || (pFATFile == NULL) || (ptr == NULL))
  {
    *pIORes = ior_INVALID_ARGUMENT;
    return 0;
  }

  if (!pFATFile->IsOpen)
  {
    *pIORes = ior_INVALID_ARGUMENT;
    return 0;
  }

  // Don't work with files in state of permanent error:
  if (pFATFile->IsPermanentError)
  {
    *pIORes = pFATFile->ErrorCode;
    return 0;
  }

  // Calculate read size in bytes:
  BytesToRead = n * size;
  if (size && n && (BytesToRead / size != n))
  {
    *pIORes = ior_ARGUMENT_OUT_OF_RANGE;
    return 0;
  }
  if (BytesToRead > pFATFile->FileSize - pFATFile->FilePosition)
  {
    BytesToRead = (size_t)(pFATFile->FileSize - pFATFile->FilePosition);
    // BytesToRead must be a multiple of size:
    BytesToRead -= BytesToRead % size;
  }
  if (!BytesToRead)
  {
    *pIORes = pFATFile->ErrorCode = ior_OK;
    return 0;
  }

  // Read in first sector if needed:
  FAT_fseek (pFATFS, pIORes, pFATFile, pFATFile->FilePosition);
  if (*pIORes != ior_OK)
    return 0;

  do
  {
    size_t BytesJustRead;

    if (BytesToRead <= FAT_SECTOR_SIZE - pFATFile->SectorPosition)
      BytesJustRead = BytesToRead;
    else
      BytesJustRead = FAT_SECTOR_SIZE - pFATFile->SectorPosition;

    memcpy (ptr, &pFATFile->aSector[pFATFile->SectorPosition], BytesJustRead);
    { char* cptr = ptr; cptr += BytesJustRead; ptr = cptr; }

    BytesRead += BytesJustRead;
    BytesToRead -= BytesJustRead;

    // Advance file position and read next sector if needed.
    if (!FAT_fseek (pFATFS, pIORes, pFATFile, pFATFile->FilePosition+BytesJustRead))
      break;
  }
  while (BytesToRead);

  return BytesRead/size;
}

size_t FAT_fwrite (tFATFS* pFATFS, tIORes* pIORes,
                   const void* ptr, size_t size, size_t n, tFATFile* pFATFile)
{
  size_t BytesToWrite;
  size_t BytesWritten=0;
  int IsPartialWrite=0;

  if (pIORes == NULL)
    return 0;

  if ((pFATFS == NULL) || (pFATFile == NULL) || (ptr == NULL))
  {
    *pIORes = ior_INVALID_ARGUMENT;
    return 0;
  }

  if (!pFATFile->IsOpen)
  {
    *pIORes = ior_INVALID_ARGUMENT;
    return 0;
  }

  // Don't work with files in state of permanent error:
  if (pFATFile->IsPermanentError)
  {
    *pIORes = pFATFile->ErrorCode;
    return 0;
  }

  // Can't write to read-only files:
  if (pFATFile->DirectoryEntry.Attribute & dea_READ_ONLY)
  {
    *pIORes = ior_PROHIBITED;
    return 0;
  }

  // Calculate write size in bytes:
  BytesToWrite = n * size;
  if (size && n && (BytesToWrite / size != n))
  {
    *pIORes = ior_ARGUMENT_OUT_OF_RANGE;
    return 0;
  }
  // We must limit the file size by 2**32-1 max
  if (BytesToWrite > 0xFFFFFFFFUL - pFATFile->FilePosition)
  {
    IsPartialWrite = 1;
    BytesToWrite = (size_t)(0xFFFFFFFFUL - pFATFile->FilePosition);
    // BytesToWrite must be a multiple of size:
    BytesToWrite -= BytesToWrite % size;
  }
  if (!BytesToWrite)
  {
    if (IsPartialWrite)
    {
      // If it was an attempt to grow the file beyond 2**32-1 size
      // and not a single item could be written because of that, fail:
      *pIORes = ior_ARGUMENT_OUT_OF_RANGE;
      return 0;
    }
    else
    {
      // Succeed if the write size was 0
      *pIORes = pFATFile->ErrorCode = ior_OK;
      return 0;
    }
  }

  // First, grow the file if needed:
  if (pFATFile->FilePosition+BytesToWrite > pFATFile->FileSize)
  {
    *pIORes = GrowFile (pFATFS, pFATFile,
                        pFATFile->FilePosition+BytesToWrite - pFATFile->FileSize);
    if (*pIORes != ior_OK)
      return 0;
  }

  // Read in a sector if needed:
  FAT_fseek (pFATFS, pIORes, pFATFile, pFATFile->FilePosition);
  if (*pIORes != ior_OK)
    return 0;

  do
  {
    size_t BytesJustWritten;

    if (BytesToWrite <= FAT_SECTOR_SIZE - pFATFile->SectorPosition)
      BytesJustWritten = BytesToWrite;
    else
      BytesJustWritten = FAT_SECTOR_SIZE - pFATFile->SectorPosition;

    memcpy (&pFATFile->aSector[pFATFile->SectorPosition], ptr, BytesJustWritten);

    pFATFile->BufferedDataState = fbds_VALID_UNFLUSHED;
    pFATFile->IsTimeUpdateNeeded = 1;

    { const char* cptr = ptr; cptr += BytesJustWritten; ptr = cptr; }

    BytesWritten += BytesJustWritten;
    BytesToWrite -= BytesJustWritten;

    // Advance file position and if needed write current sector and read next.
    if (!FAT_fseek (pFATFS, pIORes, pFATFile, pFATFile->FilePosition+BytesJustWritten))
      break;
  }
  while (BytesToWrite);

  if ((*pIORes == ior_OK) && IsPartialWrite)
  {
    // If it was an attempt to grow the file beyond 2**32-1 size
    // and not all items could be written because of that, semi-fail:
    *pIORes = pFATFile->ErrorCode = ior_ARGUMENT_OUT_OF_RANGE;
  }

  return BytesWritten/size;
}

int FAT_unlink (tFATFS* pFATFS, tIORes* pIORes, const char* filename)
{
  tFindDirEntryRes FindDirEntryRes;

  if (pIORes == NULL)
    return 0;

  if ((pFATFS == NULL) || (filename == NULL))
  {
    *pIORes = ior_INVALID_ARGUMENT;
    return 0;
  }

  // Find directory entry for specified file:
  *pIORes = FindDirectoryEntry (pFATFS, filename, &FindDirEntryRes);

  // Fail if file not found (when required):
  if (*pIORes == ior_NOT_FOUND)
    return 0;

  // Fail if directory found by that name or there's some other error:
  if (*pIORes != ior_FILE_FOUND)
    return 0;

  // Can't delete volume IDs:
  if (FindDirEntryRes.DirEntry.Attribute & dea_VOLUME_ID)
  {
    *pIORes = ior_PROHIBITED;
    return 0;
  }

  // Fail deleting read only files:
  if (FindDirEntryRes.DirEntry.Attribute & dea_READ_ONLY)
  {
    *pIORes = ior_PROHIBITED;
    return 0;
  }

  // Fail deleting open files:
  if
  (
    pFATFS->pOpenFileSearch
    (
      pFATFS,
      &FindDirEntryRes.ReadDirState,
      &FAT_OpenFilesMatch
    ) != NULL
  )
  {
    *pIORes = ior_PROHIBITED;
    return 0;
  }

  // Update the directory entry by marking the entry as deleted:
  FindDirEntryRes.DirEntry.Name[0] = DELETED_DIR_ENTRY_MARKER;
  *pIORes = WriteDirEntry
  (
    pFATFS,
    &FindDirEntryRes.ReadDirState,
    &FindDirEntryRes.DirEntry,
    0
  );
  if (*pIORes != ior_OK)
    return 0;

  // Truncate the cluster chain:
  *pIORes = TruncateClusterChain
  (
    pFATFS,
    FindDirEntryRes.DiskNo,
    GetDirEntryFirstClusterNo (&FindDirEntryRes.DirEntry),
    0
  );

  return (*pIORes == ior_OK) ? 1 : 0;
}

int FAT_access (tFATFS* pFATFS, tIORes* pIORes,
                const char *filename, tAccessMode amode)
{
  tFindDirEntryRes FindDirEntryRes;
  int res = 1;

  if (pIORes == NULL)
    return 0;

  if ((pFATFS == NULL) || (filename == NULL))
  {
    *pIORes = ior_INVALID_ARGUMENT;
    return 0;
  }

  // Find directory entry for specified file:
  *pIORes = FindDirectoryEntry (pFATFS, filename, &FindDirEntryRes);

  // Fail if file/dir not found or there's some other error:
  if
  (
    (*pIORes != ior_FILE_FOUND) &&
    (*pIORes != ior_DIR_FOUND)
  )
  {
    return 0;
  }

  // if checking for existence only, file/directory exists
  if (amode == am_F_OK)
  {
    *pIORes = ior_OK;
    return 1;
  }

#if 01
  // Can't read/write/execute volume IDs:
  if (FindDirEntryRes.DirEntry.Attribute & dea_VOLUME_ID)
    res = 0;
#endif

  // Can't write/delete read-only files/directories:
  if
  (
    (amode & am_W_OK) &&
    (FindDirEntryRes.DirEntry.Attribute & dea_READ_ONLY)
  )
    res = 0;

  // am_R_OK and am_X_OK are ignored because DOS FAT isn't UNIX-like FS:

  if (res)
    *pIORes = ior_OK;
  else
    *pIORes = ior_PROHIBITED;

  return res;
}

int FAT_stat (tFATFS* pFATFS, tIORes* pIORes,
              const char *path, tFATFileStatus* pStatus)
{
  tFindDirEntryRes FindDirEntryRes;

  if (pIORes == NULL)
    return 0;

  if ((pFATFS == NULL) || (path == NULL) || (pStatus == NULL))
  {
    *pIORes = ior_INVALID_ARGUMENT;
    return 0;
  }

  // Find directory entry for specified file:
  *pIORes = FindDirectoryEntry (pFATFS, path, &FindDirEntryRes);

  // Fail if file/dir not found or there's some other error:
  if
  (
    (*pIORes != ior_FILE_FOUND) &&
    (*pIORes != ior_DIR_FOUND)
  )
    return 0;

  pStatus->DiskNo = FindDirEntryRes.DiskNo;
  pStatus->FileSize = FindDirEntryRes.DirEntry.Size;
  pStatus->Attribute = (tFATDirEntryAttribute)FindDirEntryRes.DirEntry.Attribute;
  GetDirEntryWriteDateAndTime
  (
    &FindDirEntryRes.DirEntry,
    &pStatus->Year,
    &pStatus->Month,
    &pStatus->Day,
    &pStatus->Hour,
    &pStatus->Minute,
    &pStatus->Second
  );

  *pIORes = ior_OK;
  return 1;
}

int FAT_chmod (tFATFS* pFATFS, tIORes* pIORes,
               const char *path, tModeT mode)
{
  tFindDirEntryRes FindDirEntryRes;
  uint8 NewAttribute;

  if (pIORes == NULL)
    return 0;

  if ((pFATFS == NULL) || (path == NULL))
  {
    *pIORes = ior_INVALID_ARGUMENT;
    return 0;
  }

  // Find directory entry for specified file:
  *pIORes = FindDirectoryEntry (pFATFS, path, &FindDirEntryRes);

  // Fail if file/dir not found or there's some other error:
  if
  (
    (*pIORes != ior_FILE_FOUND) &&
    (*pIORes != ior_DIR_FOUND)
  )
  {
    return 0;
  }

  // Can't change root directory attributes:
  if (!FindDirEntryRes.DirEntry.Name[0])
  {
    *pIORes = ior_PROHIBITED;
    return 0;
  }

  // Fail changing mode of currently open files:
  if
  (
    pFATFS->pOpenFileSearch
    (
      pFATFS,
      &FindDirEntryRes.ReadDirState,
      &FAT_OpenFilesMatch
    ) != NULL
  )
  {
    *pIORes = ior_PROHIBITED;
    return 0;
  }

  NewAttribute = FindDirEntryRes.DirEntry.Attribute & ~dea_READ_ONLY;
  NewAttribute |= (mode & mt_S_IWUSR) ? 0 : dea_READ_ONLY;
  if (NewAttribute == FindDirEntryRes.DirEntry.Attribute)
  {
    *pIORes = ior_OK;
    return 1;
  }

  FindDirEntryRes.DirEntry.Attribute = NewAttribute;
  *pIORes = WriteDirEntry
  (
    pFATFS,
    &FindDirEntryRes.ReadDirState,
    &FindDirEntryRes.DirEntry,
    0
  );

  return (*pIORes == ior_OK) ? 1 : 0;
}

int FAT_utime (tFATFS* pFATFS, tIORes* pIORes, const char *path,
               uint Year, uint Month, uint Day,
               uint Hour, uint Minute, uint Second)
{
  tFindDirEntryRes FindDirEntryRes;

  if (pIORes == NULL)
    return 0;

  if ((pFATFS == NULL) || (path == NULL))
  {
    *pIORes = ior_INVALID_ARGUMENT;
    return 0;
  }

  // Find directory entry for specified file:
  *pIORes = FindDirectoryEntry (pFATFS, path, &FindDirEntryRes);

  // Fail if file/dir not found or there's some other error:
  if
  (
    (*pIORes != ior_FILE_FOUND) &&
    (*pIORes != ior_DIR_FOUND)
  )
  {
    return 0;
  }

  // Can't change root directory attributes:
  if (!FindDirEntryRes.DirEntry.Name[0])
  {
    *pIORes = ior_PROHIBITED;
    return 0;
  }

  // Fail with error if the file is read-only:
  if (FindDirEntryRes.DirEntry.Attribute & dea_READ_ONLY)
  {
    *pIORes = ior_PROHIBITED;
    return 0;
  }

  // Fail changing times of currently open files:
  if
  (
    pFATFS->pOpenFileSearch
    (
      pFATFS,
      &FindDirEntryRes.ReadDirState,
      &FAT_OpenFilesMatch
    ) != NULL
  )
  {
    *pIORes = ior_PROHIBITED;
    return 0;
  }

  SetDirEntryWriteDateAndTime
  (
    &FindDirEntryRes.DirEntry,
    Year, Month, Day,
    Hour, Minute, Second
  );

  *pIORes = WriteDirEntry
  (
    pFATFS,
    &FindDirEntryRes.ReadDirState,
    &FindDirEntryRes.DirEntry,
    0
  );

  return (*pIORes == ior_OK) ? 1 : 0;
}

int FAT_statvfs (tFATFS* pFATFS, tIORes* pIORes,
                 const char *path, tFATFSStatus* pStatus)
{
  tFindDirEntryRes FindDirEntryRes;
  int DiskNo;

  if (pIORes == NULL)
    return 0;

  if ((pFATFS == NULL) || (path == NULL) || (pStatus == NULL))
  {
    *pIORes = ior_INVALID_ARGUMENT;
    return 0;
  }

  memset (pStatus, 0, sizeof(*pStatus));

  // Find directory entry for specified file:
  *pIORes = FindDirectoryEntry (pFATFS, path, &FindDirEntryRes);

  // Fail if file/dir not found or there's some other error:
  if
  (
    (*pIORes != ior_FILE_FOUND) &&
    (*pIORes != ior_DIR_FOUND)
  )
    return 0;

  DiskNo = FindDirEntryRes.DiskNo;

  // The root directory can always be found even if the disk isn't valid or
  // initialized (if no read operations have been done on it so far).
  // To get the disk info, the disk must be initialized anyway:
  if (!pFATFS->aLogicDisks[DiskNo].IsValid)
  {
    *pIORes = InitDisk (pFATFS, DiskNo);
    if (*pIORes != ior_OK)
      return 0;
  }

  pStatus->ClusterSize = (uint32)FAT_SECTOR_SIZE *
                         pFATFS->aLogicDisks[DiskNo].BPB.BPB1.SectorsPerCluster;

  pStatus->TotalClusterCount = pFATFS->aLogicDisks[DiskNo].ClustersCount;

  *pIORes = GetFreeFATEntriesCount
  (
    pFATFS,
    DiskNo,
    &pStatus->TotalFreeClusterCount
  );

  return *pIORes == ior_OK;
}

static void FAT_OnMediaHasChanged (tFATFS* pFATFS, uint8 BIOSDrive)
{
  int DiskNo;

  for (DiskNo=0; DiskNo<MAX_LOGIC_DISKS; DiskNo++)
    if
    (
      pFATFS->aLogicDisks[DiskNo].Exists &&
      (pFATFS->aLogicDisks[DiskNo].DiskLocation.BIOSDrive == BIOSDrive)
    )
    {
      int i;
      // Invalidate internal cache entries associated with this disk:

      // Invalidate cached FAT images:
      for (i=0; i<MAX_FAT_IMAGES_AT_ONCE; i++)
        if (((tFATImage*)pFATFS->aFATCacheEntries[i].pAttachedData)->DiskNo == DiskNo)
          Cache_ResetIsValid (&pFATFS->aFATCacheEntries[i]);

      // Invalidate cached directory images:
      for (i=0; i<MAX_DIR_IMAGES_AT_ONCE; i++)
        if (((tDirImage*)pFATFS->aDirCacheEntries[i].pAttachedData)->DiskNo == DiskNo)
          Cache_ResetIsValid (&pFATFS->aDirCacheEntries[i]);

      // Reinit disk:
      InitDisk (pFATFS, DiskNo);
    }
}

int FAT_OpenFilesMatch (tFATFS* pFATFS, tFATFile* pFATFile,
                        tReadDirState* pReadDirState)
{
  (void)pFATFS;
  return
  (
    (pFATFile->ReadDirState.DiskNo == pReadDirState->DiskNo) &&
    (pFATFile->ReadDirState.CurrentClusterNo == pReadDirState->CurrentClusterNo) &&
    (pFATFile->ReadDirState.CurrentSectorNo == pReadDirState->CurrentSectorNo) &&
    (pFATFile->ReadDirState.SectorPosition == pReadDirState->SectorPosition)
  );
}

/*
  tbd:
  - limit dir size
  +/- support big files up to 4GB-1 size
  - revise support for far-away partitions (beyond 32-bit LBA)
  +/- review all code that handles fat1x and fat32 differently (everything about root dir)
  - review floppy change code and test it
  - review all code that creates, deletes, grows and truncates file/dir,
    analyze error paths
  +/- review & fix all fxns that may not init the disk but use its properties
    (like in FAT_statvfs()) or may try to change root dir properties
  +/- weired errors and extra errors during error handling should be logged -
    review relevant code
  - review the uses of pFATFile->ErrorCode, IsPermanentError
    (who uses its value? is it only for permanent errors?)
  - review functions that advance positions but may fail -
    for example, should on failure (e.g. I/O error) the file position advance and
    data in the buffers be overwritten?
  + check LBA limits when reading/writing from/to a logical disk
  +/- check if damaged FS can cause:
    +/- memory access violations
    + division overflow & divisions by 0
    (hanging (due to cycles in lists) or damaging FS further not considered)
  + review the use of ior_ERR_IO
*/
