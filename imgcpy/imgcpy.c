#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include "FAT.h"
#include "image.h"
#include "testcase.h"

typedef enum
{
  OP_INVALID           = 0,
  OP_COPY_TO_IMAGE     = 1,
  OP_COPY_FROM_IMAGE   = 2,
  OP_MAKE_DIR_TO_IMAGE = 3
} tOperation;

tBIOSDriveAndImageFName BIOSDriveAndImageFileName[] =
{
  {0x00, NULL}
};

int main (int argc, char* argv[])
{
  tIORes IOres;
  tFATFile file;
  int IsFileInImageOpen = 0;
  FILE* f = NULL;
  int Operation = OP_INVALID;
  const char* pImageNames;
  const char* pFileName;
  char* pImageName = NULL;
  char* pImageFileName;
  int res = 1;

  if ((argc < 2) || (argc > 3))
  {
lusage:
    printf ("\nCopies files to/from FAT disk images.\n"
            "Usage:\n  imgcpy <source> <destination> - copies a file to/from an image\n"
            "OR\n  imgcpy <destination> - creates a directory in an image\n");
    printf ("where <source> or <destination> should be of this form:\n"
            "  <image file>=<drive letter>:<path> -- specifies the disk image\n"
            "  to \"mount\" and a file(dir) in it to access\n"
            "and the other of the two should be a normal file name.\n\n");
    printf ("Note:\n  If <drive letter> is A or B, then the image is mounted as floppy,\n"
            "  otherwise (e.g. C and so on) as hard disk.\n\n");
    goto lend;
  }

  if (strchr (argv[1], '=') != NULL)
    Operation |= OP_COPY_FROM_IMAGE;

  if ((argc == 3) && (strchr (argv[2], '=') != NULL))
    Operation |= OP_COPY_TO_IMAGE;

  if ((Operation != OP_COPY_FROM_IMAGE) &&
      (Operation != OP_COPY_TO_IMAGE))
  {
    printf ("Invalid arguments.\n");
    goto lusage;
  }

  switch (Operation)
  {
  case OP_COPY_FROM_IMAGE:
    pImageNames = argv[1];
    if (argc == 3)
    {
      pFileName = argv[2];
    }
    else
    {
      pFileName = NULL;
      Operation = OP_MAKE_DIR_TO_IMAGE;
    }
    break;

  case OP_COPY_TO_IMAGE:
    pImageNames = argv[2];
    pFileName = argv[1];
    break;
  }

  // Work out the image name and the name of the file within the image:
  pImageName = strdup (pImageNames);
  if (pImageName == NULL)
  {
    printf ("out of RAM\n.");
    goto lend;
  }
  pImageName = strtok (pImageName, "=");
  pImageFileName = strtok (NULL, "=");
  if ((pImageFileName == NULL) ||
      (strlen(pImageFileName) < 3) ||
      (pImageFileName[1] != ':'))
  {
    printf ("Invalid <image file>=<drive letter>:<path>.\n");
    goto lusage;
  }
  strupr (pImageFileName);
  if ((pImageFileName[0] < 'A') || (pImageFileName[0] > 'Z'))
  {
    printf ("Invalid <drive letter>.\n");
    goto lusage;
  }

  // Find out if the image is for floppy or hard disk:
  if (pImageFileName[0] >= 'C')
    BIOSDriveAndImageFileName[0].BIOSDrive = 0x80;
  else
    BIOSDriveAndImageFileName[0].BIOSDrive = pImageFileName[0] - 'A';

  BIOSDriveAndImageFileName[0].pImageFileName = pImageName;

  // Try to "mount" the image:
  if (!stdio_InitFATFS(BIOSDriveAndImageFileName,
        sizeof(BIOSDriveAndImageFileName)/sizeof(BIOSDriveAndImageFileName[0]),
        0))
  {
    printf("Failed to mount image file \'%s\' as FAT disk!\n", pImageName);
    goto lend;
  }

  switch (Operation)
  {
  case OP_COPY_FROM_IMAGE:
    FAT_fopen (&stdio_FATFS, &IOres, &file, pImageFileName, "rb");
    if (IOres != ior_OK)
    {
      printf ("can't open file \'%s\' in \'%s\', error: %u!\n",
              pImageFileName, pImageName, (uint)IOres);
      goto lend;
    }

    IsFileInImageOpen = 1;

    f = fopen (pFileName, "wb");

    if (f == NULL)
    {
      printf ("can't create file \'%s\'!\n", pFileName);
      goto lend;
    }

    for(;;)
    {
      char buf[4096];
      size_t readcnt, writecnt;

      readcnt = FAT_fread (&stdio_FATFS, &IOres, buf, 1, sizeof(buf), &file);
      if (IOres != ior_OK)
      {
        printf ("can't read file \'%s\' in \'%s\', error: %u\n"
                "tried to read: %u bytes, read: %u bytes!\n",
                pImageFileName, pImageName, (uint)IOres,
                (uint)sizeof(buf), (uint)readcnt);
        goto lend;
      }
      if (!readcnt)
        break;

      errno = 0;
      writecnt = fwrite (buf, 1, readcnt, f);
      if (writecnt != readcnt)
      {
        printf ("can't write file \'%s\', error: %d\n"
                "tried to write: %u bytes, wrote: %u bytes!\n",
                pFileName, errno,
                (uint)readcnt, (uint)writecnt);
        goto lend;
      }
    }
    break;

  case OP_COPY_TO_IMAGE:
    f = fopen (pFileName, "rb");

    if (f == NULL)
    {
      printf ("can't open file \'%s\'!\n", pFileName);
      goto lend;
    }

    FAT_fopen (&stdio_FATFS, &IOres, &file, pImageFileName, "wb");
    if (IOres != ior_OK)
    {
      printf ("can't create file \'%s\' in \'%s\', error: %u!\n",
              pImageFileName, pImageName, (uint)IOres);
      goto lend;
    }

    IsFileInImageOpen = 1;

    for(;;)
    {
      char buf[4096];
      size_t readcnt, writecnt;

      errno = 0;
      readcnt = fread (buf, 1, sizeof(buf), f);
      if (errno)
      {
        printf ("can't read file \'%s\', error: %d\n"
                "tried to read: %u bytes, read: %u bytes!\n",
                pFileName, errno,
                (uint)sizeof(buf), (uint)readcnt);
        goto lend;
      }
      if (!readcnt)
        break;

      writecnt = FAT_fwrite (&stdio_FATFS, &IOres, buf, 1, readcnt, &file);
      if ((writecnt != readcnt) || (IOres != ior_OK))
      {
        printf ("can't write file \'%s\' in \'%s\', error: %u\n"
                "tried to write: %u bytes, wrote: %u bytes!\n",
                pImageFileName, pImageName, (uint)IOres,
                (uint)readcnt, (uint)writecnt);
        goto lend;
      }
    }
    break;

  case OP_MAKE_DIR_TO_IMAGE:
    FAT_mkdir (&stdio_FATFS, &IOres, pImageFileName, mt_S_IWUSR);
    if (IOres != ior_OK)
    {
      printf ("can't create directory \'%s\' in \'%s\', error: %u!\n",
              pImageFileName, pImageName, (uint)IOres);
      goto lend;
    }
    break;
  }

  res = 0;

lend:

  if (IsFileInImageOpen)
  {
    FAT_fclose (&stdio_FATFS, &IOres, &file);
    if (IOres != ior_OK)
    {
      printf ("can't close file \'%s\' in \'%s\', error:%u!\n",
              pImageFileName, pImageName, (uint)IOres);
      res = 1;
    }
  }

  if (f != NULL)
  {
    if (fclose (f))
    {
      printf ("failed to close file \'%s\'\n", pFileName);
      res = 1;
    }
  }

  if (pImageName != NULL)
    free (pImageName);

  if (!stdio_DoneFATFS())
  {
    if (!res)
    {
      printf("Failed to unmount image file \'%s\'!\n", pImageName);
      res = 1;
    }
  }

  return res;
}
