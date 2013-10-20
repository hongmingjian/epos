#ifndef _IMAGE_H_
#define _IMAGE_H_

#include "FAT.h"

typedef struct tBIOSDriveAndImageFName {
  uint8 BIOSDrive;
  const char* pImageFileName;
} tBIOSDriveAndImageFName;

extern tFATFS stdio_FATFS;
extern int stdio_IsFATFSInit;

extern int stdio_InitFATFS(const tBIOSDriveAndImageFName* pBIOSDriveAndImageFileName,
                           unsigned BIOSDrivesCount,
                           int _FATBootDiskHint);
extern int stdio_FlushFATFS (void);
extern int stdio_DoneFATFS (void);

#endif // _IMAGE_H_
