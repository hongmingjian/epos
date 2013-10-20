#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include "testcase.h"
#include "image.h"
#include "FAT.h"

uint TestDir1(void)
{
  tIORes res;
  uint result = 1;
  tFATFSStatus FSStatus1, FSStatus2;

  FAT_statvfs (&stdio_FATFS, &res, "\\", &FSStatus1);
  CHK(res == ior_OK);

  FAT_rmdir (&stdio_FATFS, &res, "level1");
  CHK(res == ior_NOT_FOUND);

  FAT_rmdir (&stdio_FATFS, &res, "level1");
  CHK(res == ior_NOT_FOUND);

  FAT_mkdir (&stdio_FATFS, &res, "level1", mt_S_IWUSR);
  CHK(res == ior_OK);

  FAT_mkdir (&stdio_FATFS, &res, "level1", mt_S_IWUSR);
  CHK(res == ior_DIR_FOUND);

  FAT_rmdir (&stdio_FATFS, &res, "level1");
  CHK(res == ior_OK);

  FAT_rmdir (&stdio_FATFS, &res, "level1");
  CHK(res == ior_NOT_FOUND);

  FAT_mkdir (&stdio_FATFS, &res, "level1", mt_S_IWUSR);
  CHK(res == ior_OK);

  FAT_rmdir (&stdio_FATFS, &res, "level1");
  CHK(res == ior_OK);

  FAT_statvfs (&stdio_FATFS, &res, "\\", &FSStatus2);
  CHK(res == ior_OK);
  CHK(FSStatus2.TotalFreeClusterCount == FSStatus1.TotalFreeClusterCount);

  result = 0;

lend:
  return result;
}

uint TestDir2(void)
{
  tIORes res;
  uint result = 1;
  tFATFSStatus FSStatus1, FSStatus2;

  FAT_statvfs (&stdio_FATFS, &res, "\\", &FSStatus1);
  CHK(res == ior_OK);

// bad chars: \"*+,./:;<=>?[\]|

  FAT_mkdir (&stdio_FATFS, &res, NULL, mt_S_IWUSR);
  CHK(res == ior_INVALID_ARGUMENT);

  FAT_mkdir (&stdio_FATFS, &res, "", mt_S_IWUSR);
  CHK(res == ior_BAD_NAME);

  FAT_mkdir (&stdio_FATFS, &res, "\xE5", mt_S_IWUSR);
  CHK(res == ior_BAD_NAME);

  FAT_mkdir (&stdio_FATFS, &res, "a\x05", mt_S_IWUSR);
  CHK(res == ior_BAD_NAME);

  FAT_mkdir (&stdio_FATFS, &res, "\\", mt_S_IWUSR);
  CHK(res == ior_DIR_FOUND);

  FAT_mkdir (&stdio_FATFS, &res, ".", mt_S_IWUSR);
  CHK(res == ior_DIR_FOUND);

  FAT_mkdir (&stdio_FATFS, &res, "..", mt_S_IWUSR);
  CHK(res == ior_BAD_NAME);

  FAT_mkdir (&stdio_FATFS, &res, "\"", mt_S_IWUSR);
  CHK(res == ior_BAD_NAME);

  FAT_mkdir (&stdio_FATFS, &res, "*", mt_S_IWUSR);
  CHK(res == ior_BAD_NAME);

  FAT_mkdir (&stdio_FATFS, &res, "+", mt_S_IWUSR);
  CHK(res == ior_BAD_NAME);

  FAT_mkdir (&stdio_FATFS, &res, ",", mt_S_IWUSR);
  CHK(res == ior_BAD_NAME);

  FAT_mkdir (&stdio_FATFS, &res, "/", mt_S_IWUSR);
  CHK(res == ior_BAD_NAME);

  FAT_mkdir (&stdio_FATFS, &res, ":", mt_S_IWUSR);
  CHK(res == ior_BAD_NAME);

  FAT_mkdir (&stdio_FATFS, &res, ";", mt_S_IWUSR);
  CHK(res == ior_BAD_NAME);

  FAT_mkdir (&stdio_FATFS, &res, "<", mt_S_IWUSR);
  CHK(res == ior_BAD_NAME);

  FAT_mkdir (&stdio_FATFS, &res, "=", mt_S_IWUSR);
  CHK(res == ior_BAD_NAME);

  FAT_mkdir (&stdio_FATFS, &res, ">", mt_S_IWUSR);
  CHK(res == ior_BAD_NAME);

  FAT_mkdir (&stdio_FATFS, &res, "?", mt_S_IWUSR);
  CHK(res == ior_BAD_NAME);

  FAT_mkdir (&stdio_FATFS, &res, "[", mt_S_IWUSR);
  CHK(res == ior_BAD_NAME);

  FAT_mkdir (&stdio_FATFS, &res, "]", mt_S_IWUSR);
  CHK(res == ior_BAD_NAME);

  FAT_mkdir (&stdio_FATFS, &res, "|", mt_S_IWUSR);
  CHK(res == ior_BAD_NAME);

  FAT_mkdir (&stdio_FATFS, &res, " ", mt_S_IWUSR);
  CHK(res == ior_BAD_NAME);

  FAT_mkdir (&stdio_FATFS, &res, "blah .txt", mt_S_IWUSR);
  CHK(res == ior_BAD_NAME);

  FAT_mkdir (&stdio_FATFS, &res, "blah. c", mt_S_IWUSR);
  CHK(res == ior_BAD_NAME);

  FAT_mkdir (&stdio_FATFS, &res, "c . c", mt_S_IWUSR);
  CHK(res == ior_BAD_NAME);

  FAT_mkdir (&stdio_FATFS, &res, "a b", mt_S_IWUSR);
  CHK(res == ior_BAD_NAME);

  FAT_mkdir (&stdio_FATFS, &res, "c.d e", mt_S_IWUSR);
  CHK(res == ior_BAD_NAME);

  FAT_mkdir (&stdio_FATFS, &res, ".txt", mt_S_IWUSR);
  CHK(res == ior_BAD_NAME);

  FAT_mkdir (&stdio_FATFS, &res, ".text", mt_S_IWUSR);
  CHK(res == ior_BAD_NAME);

  FAT_mkdir (&stdio_FATFS, &res, "text.text", mt_S_IWUSR);
  CHK(res == ior_BAD_NAME);

  FAT_mkdir (&stdio_FATFS, &res, "a..", mt_S_IWUSR);
  CHK(res == ior_BAD_NAME);

  FAT_mkdir (&stdio_FATFS, &res, "123456789", mt_S_IWUSR);
  CHK(res == ior_BAD_NAME);

  FAT_mkdir (&stdio_FATFS, &res, "1234567890AB", mt_S_IWUSR);
  CHK(res == ior_BAD_NAME);

  FAT_mkdir (&stdio_FATFS, &res, "1234567890ABC", mt_S_IWUSR);
  CHK(res == ior_BAD_NAME);

  FAT_mkdir (&stdio_FATFS, &res, "1234567890ABCD", mt_S_IWUSR);
  CHK(res == ior_BAD_NAME);

  FAT_statvfs (&stdio_FATFS, &res, "\\", &FSStatus2);
  CHK(res == ior_OK);
  CHK(FSStatus2.TotalFreeClusterCount == FSStatus1.TotalFreeClusterCount);

  result = 0;

lend:
  return result;
}

uint TestDir3(void)
{
  tIORes res;
  uint result = 1;
  tFATFSStatus FSStatus1, FSStatus2;

  FAT_statvfs (&stdio_FATFS, &res, "\\", &FSStatus1);
  CHK(res == ior_OK);

// bad chars: \"*+,./:;<=>?[\]|

  FAT_rmdir (&stdio_FATFS, &res, NULL);
  CHK(res == ior_INVALID_ARGUMENT);

  FAT_rmdir (&stdio_FATFS, &res, "");
  CHK(res == ior_BAD_NAME);

  FAT_rmdir (&stdio_FATFS, &res, "\\");
  CHK(res == ior_PROHIBITED);

  FAT_rmdir (&stdio_FATFS, &res, ".");
  CHK(res == ior_PROHIBITED);

  FAT_rmdir (&stdio_FATFS, &res, "..");
  CHK(res == ior_NOT_FOUND);

  FAT_rmdir (&stdio_FATFS, &res, " ");
  CHK(res == ior_NOT_FOUND);

  FAT_statvfs (&stdio_FATFS, &res, "\\", &FSStatus2);
  CHK(res == ior_OK);
  CHK(FSStatus2.TotalFreeClusterCount == FSStatus1.TotalFreeClusterCount);

  result = 0;

lend:
  return result;
}

uint TestDir4(void)
{
  tIORes res;
  uint result = 1;
  tFATFSStatus FSStatus1, FSStatus2;

  FAT_statvfs (&stdio_FATFS, &res, "\\", &FSStatus1);
  CHK(res == ior_OK);

  FAT_mkdir (&stdio_FATFS, &res, "level1", mt_S_IWUSR);
  CHK(res == ior_OK);

  FAT_rmdir (&stdio_FATFS, &res, "level1\\level2");
  CHK(res == ior_NOT_FOUND);

  FAT_rmdir (&stdio_FATFS, &res, "level1\\level2");
  CHK(res == ior_NOT_FOUND);

  FAT_mkdir (&stdio_FATFS, &res, "level1\\level2", mt_S_IWUSR);
  CHK(res == ior_OK);

  FAT_mkdir (&stdio_FATFS, &res, "level1\\level2", mt_S_IWUSR);
  CHK(res == ior_DIR_FOUND);

  FAT_rmdir (&stdio_FATFS, &res, "level1\\level2");
  CHK(res == ior_OK);

  FAT_rmdir (&stdio_FATFS, &res, "level1\\level2");
  CHK(res == ior_NOT_FOUND);

  FAT_mkdir (&stdio_FATFS, &res, "level1\\level2", mt_S_IWUSR);
  CHK(res == ior_OK);

  FAT_rmdir (&stdio_FATFS, &res, "level1");
  CHK(res == ior_PROHIBITED);

  FAT_rmdir (&stdio_FATFS, &res, "level1\\level2");
  CHK(res == ior_OK);

  FAT_rmdir (&stdio_FATFS, &res, "level1");
  CHK(res == ior_OK);

  FAT_statvfs (&stdio_FATFS, &res, "\\", &FSStatus2);
  CHK(res == ior_OK);
  CHK(FSStatus2.TotalFreeClusterCount == FSStatus1.TotalFreeClusterCount);

  result = 0;

lend:
  return result;
}

uint TestDir5(void)
{
  tIORes res;
  uint result = 1;
  tFATFSStatus FSStatus1, FSStatus2;

  FAT_statvfs (&stdio_FATFS, &res, "\\", &FSStatus1);
  CHK(res == ior_OK);

  FAT_mkdir (&stdio_FATFS, &res, "level1", mt_S_IWUSR);
  CHK(res == ior_OK);

  FAT_mkdir (&stdio_FATFS, &res, "level1\\level2", mt_S_IWUSR);
  CHK(res == ior_OK);

  FAT_rmdir (&stdio_FATFS, &res, "level1\\level2\\level3");
  CHK(res == ior_NOT_FOUND);

  FAT_rmdir (&stdio_FATFS, &res, "level1\\level2\\level3");
  CHK(res == ior_NOT_FOUND);

  FAT_mkdir (&stdio_FATFS, &res, "level1\\level2\\level3", mt_S_IWUSR);
  CHK(res == ior_OK);

  FAT_mkdir (&stdio_FATFS, &res, "level1\\level2\\level3", mt_S_IWUSR);
  CHK(res == ior_DIR_FOUND);

  FAT_rmdir (&stdio_FATFS, &res, "level1\\level2\\level3");
  CHK(res == ior_OK);

  FAT_rmdir (&stdio_FATFS, &res, "level1\\level2\\level3");
  CHK(res == ior_NOT_FOUND);

  FAT_mkdir (&stdio_FATFS, &res, "level1\\level2\\level3", mt_S_IWUSR);
  CHK(res == ior_OK);

  FAT_rmdir (&stdio_FATFS, &res, "level1\\level2");
  CHK(res == ior_PROHIBITED);

  FAT_rmdir (&stdio_FATFS, &res, "level1");
  CHK(res == ior_PROHIBITED);

  FAT_rmdir (&stdio_FATFS, &res, "level1\\level2\\level3");
  CHK(res == ior_OK);

  FAT_rmdir (&stdio_FATFS, &res, "level1\\level2");
  CHK(res == ior_OK);

  FAT_rmdir (&stdio_FATFS, &res, "level1");
  CHK(res == ior_OK);

  FAT_statvfs (&stdio_FATFS, &res, "\\", &FSStatus2);
  CHK(res == ior_OK);
  CHK(FSStatus2.TotalFreeClusterCount == FSStatus1.TotalFreeClusterCount);

  result = 0;

lend:
  return result;
}

uint TestDir6(void)
{
  tIORes res;
  uint result = 1;
  tFATFSStatus FSStatus1, FSStatus2;
  char name[128];

  FAT_statvfs (&stdio_FATFS, &res, "\\", &FSStatus1);
  CHK(res == ior_OK);

  FAT_chdir (&stdio_FATFS, &res, "a:", 0);
  CHK(res == ior_OK);

  FAT_getcwd (&stdio_FATFS, &res, name, sizeof(name));
  CHK(res == ior_OK);
  res = (strcmp (name, "A:\\") == 0) ? ior_OK : ior_BAD_NAME;
  CHK(res == ior_OK);

// bad chars: \"*+,./:;<=>?[\]|

  FAT_mkdir (&stdio_FATFS, &res, "\x05\xE5", mt_S_IWUSR);
  CHK(res == ior_OK);

  FAT_rmdir (&stdio_FATFS, &res, "\x05\xE5");
  CHK(res == ior_OK);

  FAT_mkdir (&stdio_FATFS, &res, "a.b", mt_S_IWUSR);
  CHK(res == ior_OK);

  FAT_rmdir (&stdio_FATFS, &res, "a.b");
  CHK(res == ior_OK);

  FAT_mkdir (&stdio_FATFS, &res, "level1\\", mt_S_IWUSR);
  CHK(res == ior_OK);

  FAT_chdir (&stdio_FATFS, &res, "level", 0);
  CHK(res == ior_NOT_FOUND);

  FAT_chdir (&stdio_FATFS, &res, "level11\\", 0);
  CHK(res == ior_NOT_FOUND); // ior_BAD_NAME

  FAT_chdir (&stdio_FATFS, &res, "level1\\", 0);
  CHK(res == ior_OK);

  FAT_chdir (&stdio_FATFS, &res, "a:", 1);
  CHK(res == ior_OK);

  FAT_rmdir (&stdio_FATFS, &res, "\\level1");
  CHK(res == ior_PROHIBITED);

  FAT_rmdir (&stdio_FATFS, &res, "..\\level1");
  CHK(res == ior_PROHIBITED);

  FAT_rmdir (&stdio_FATFS, &res, ".");
  CHK(res == ior_PROHIBITED);

  FAT_rmdir (&stdio_FATFS, &res, "..");
  CHK(res == ior_PROHIBITED);

  FAT_getcwd (&stdio_FATFS, &res, name, sizeof(name));
  CHK(res == ior_OK);
  res = (strcmp (name, "A:\\LEVEL1") == 0) ? ior_OK : ior_BAD_NAME;
  CHK(res == ior_OK);

  FAT_chdir (&stdio_FATFS, &res, "..", 0);
  CHK(res == ior_OK);

  FAT_rmdir (&stdio_FATFS, &res, "level1");
  CHK(res == ior_OK);

  FAT_chdir (&stdio_FATFS, &res, "z:", 0);
  CHK(res == ior_BAD_NAME);

  FAT_chdir (&stdio_FATFS, &res, "[:", 0);
  CHK(res == ior_BAD_NAME);

  FAT_chdir (&stdio_FATFS, &res, "a:", 1);
  CHK(res == ior_OK);

  FAT_mkdir (&stdio_FATFS, &res, "level1.dir", mt_S_IWUSR);
  CHK(res == ior_OK);

  FAT_chdir (&stdio_FATFS, &res, "level1.dir", 0);
  CHK(res == ior_OK);

  CHK(FAT_getcwd (&stdio_FATFS, &res, name, sizeof(name)) == name);
  CHK(res == ior_OK);
  res = (strcmp (name, "A:\\LEVEL1.DIR") == 0) ? ior_OK : ior_BAD_NAME;
  CHK(res == ior_OK);

  CHK(FAT_getcwd (&stdio_FATFS, &res, name, strlen("A:\\LEVEL1.DIR")) == NULL);
  CHK(res == ior_ARGUMENT_OUT_OF_RANGE);

  CHK(FAT_getcwd (&stdio_FATFS, &res, name, strlen("A:\\LEVEL1.DIR")+1) == name);
  CHK(res == ior_OK);
  res = (strcmp (name, "A:\\LEVEL1.DIR") == 0) ? ior_OK : ior_BAD_NAME;
  CHK(res == ior_OK);

  FAT_chdir (&stdio_FATFS, &res, "..", 0);
  CHK(res == ior_OK);

  FAT_rmdir (&stdio_FATFS, &res, "level1.dir");
  CHK(res == ior_OK);

  FAT_statvfs (&stdio_FATFS, &res, "\\", &FSStatus2);
  CHK(res == ior_OK);
  CHK(FSStatus2.TotalFreeClusterCount == FSStatus1.TotalFreeClusterCount);

  result = 0;

lend:
  return result;
}

uint TestDir7(void)
{
  tIORes res;
  uint result = 1;
  uint i;
  char name[13];
  tFATFSStatus FSStatus1, FSStatus2;

  FAT_statvfs (&stdio_FATFS, &res, "\\", &FSStatus1);
  CHK(res == ior_OK);

  FAT_mkdir (&stdio_FATFS, &res, "level1", mt_S_IWUSR);
  CHK(res == ior_OK);

  FAT_chdir (&stdio_FATFS, &res, "level1", 0);
  CHK(res == ior_OK);

  for (i = 0; i < 257; i++)
  {
    sprintf (name, "%u", i);
    FAT_mkdir (&stdio_FATFS, &res, name, mt_S_IWUSR);
    CHK(res == ior_OK);
  }

  for (i = 0; i < 257; i++)
  {
    sprintf (name, "%u", i);
    FAT_rmdir (&stdio_FATFS, &res, name);
    CHK(res == ior_OK);
  }

  FAT_chdir (&stdio_FATFS, &res, "..", 0);
  CHK(res == ior_OK);

  FAT_rmdir (&stdio_FATFS, &res, "level1");
  CHK(res == ior_OK);

  FAT_statvfs (&stdio_FATFS, &res, "\\", &FSStatus2);
  CHK(res == ior_OK);
  CHK(FSStatus2.TotalFreeClusterCount == FSStatus1.TotalFreeClusterCount);

  result = 0;

lend:
  return result;
}

uint TestDir8(void)
{
  tIORes res;
  uint result = 1;
  uint i, cnt, j;
  char name[13];
  tReadDirState ReadDirState;
  tFATDirectoryEntry DirEntry;
  tFATFSStatus FSStatus1, FSStatus2;

  FAT_statvfs (&stdio_FATFS, &res, "\\", &FSStatus1);
  CHK(res == ior_OK);

  FAT_opendir (&stdio_FATFS, &res, "blah", &ReadDirState);
  CHK(res == ior_NOT_FOUND);

  FAT_opendir (&stdio_FATFS, &res, ".", &ReadDirState);
  CHK(res == ior_OK);

  FAT_readdir (&stdio_FATFS, &res, &ReadDirState, &DirEntry);
  CHK(res == ior_NO_MORE_FILES);

  FAT_closedir (&stdio_FATFS, &res, &ReadDirState);
  CHK(res == ior_OK);

  for (cnt = i = 0; i < UINT_MAX; i++)
  {
    sprintf (name, "%u", i);
    FAT_mkdir (&stdio_FATFS, &res, name, mt_S_IWUSR);
    CHK((res == ior_OK) || (res == ior_NO_SPACE));
    if (res == ior_OK)
      cnt++;
    else
      break;
  }

  FAT_opendir (&stdio_FATFS, &res, ".", &ReadDirState);
  CHK(res == ior_OK);

  for (j = 0; j < 2; j++)
  {
    for (i = 0; i < cnt; i++)
    {
      char s[13];
      sprintf (name, "%u", i);
      FAT_readdir (&stdio_FATFS, &res, &ReadDirState, &DirEntry);
      CHK(res == ior_OK);
      CHK(DirEntryNameToASCIIZ (&DirEntry, s) == s);
      CHK(!strcmp (name, s));
    }

    if (!j)
    {
      FAT_rewinddir (&stdio_FATFS, &res, &ReadDirState);
      CHK(res == ior_OK);
    }
  }

  FAT_readdir (&stdio_FATFS, &res, &ReadDirState, &DirEntry);
  CHK(res == ior_NO_MORE_FILES);

  FAT_closedir (&stdio_FATFS, &res, &ReadDirState);
  CHK(res == ior_OK);

  for (i = 0; i < cnt; i++)
  {
    sprintf (name, "%u", i);
    FAT_rmdir (&stdio_FATFS, &res, name);
    CHK(res == ior_OK);
  }

  FAT_opendir (&stdio_FATFS, &res, ".", &ReadDirState);
  CHK(res == ior_OK);

  FAT_readdir (&stdio_FATFS, &res, &ReadDirState, &DirEntry);
  CHK(res == ior_NO_MORE_FILES);

  FAT_closedir (&stdio_FATFS, &res, &ReadDirState);
  CHK(res == ior_OK);

  CHK(cnt == 224);

  // repeat
  for (i = 0; i < cnt; i++)
  {
    sprintf (name, "%u", i);
    FAT_mkdir (&stdio_FATFS, &res, name, mt_S_IWUSR);
    CHK((res == ior_OK) || (res == ior_NO_SPACE));
    if (res != ior_OK)
    {
      CHK(i == cnt - 1);
      break;
    }
  }

  for (i = 0; i < cnt; i++)
  {
    sprintf (name, "%u", i);
    FAT_rmdir (&stdio_FATFS, &res, name);
    CHK(res == ior_OK);
  }

  FAT_statvfs (&stdio_FATFS, &res, "\\", &FSStatus2);
  CHK(res == ior_OK);
  CHK(FSStatus2.TotalFreeClusterCount == FSStatus1.TotalFreeClusterCount);

  result = 0;

lend:
  return result;
}

uint TestDir9(void)
{
  tIORes res;
  uint result = 1;
  tFATFSStatus FSStatus1, FSStatus2;

  FAT_statvfs (&stdio_FATFS, &res, "\\", &FSStatus1);
  CHK(res == ior_OK);

  FAT_mkdir (&stdio_FATFS, &res, "level1", mt_S_IWUSR);
  CHK(res == ior_OK);

  FAT_mkdir (&stdio_FATFS, &res, "level1\\level2", mt_S_IWUSR);
  CHK(res == ior_OK);

  FAT_mkdir (&stdio_FATFS, &res, "level1\\level2\\level3", mt_S_IWUSR);
  CHK(res == ior_OK);

  FAT_chdir (&stdio_FATFS, &res, "level1\\level2\\level3", 0);
  CHK(res == ior_OK);

  FAT_chdir (&stdio_FATFS, &res, "..", 0);
  CHK(res == ior_OK);

  FAT_chdir (&stdio_FATFS, &res, "..", 0);
  CHK(res == ior_OK);

  FAT_chdir (&stdio_FATFS, &res, "..", 0);
  CHK(res == ior_OK);

  FAT_chdir (&stdio_FATFS, &res, "level1\\level2\\level3", 0);
  CHK(res == ior_OK);

  FAT_chdir (&stdio_FATFS, &res, "..\\..", 0);
  CHK(res == ior_OK);

  FAT_chdir (&stdio_FATFS, &res, "..", 0);
  CHK(res == ior_OK);

  FAT_chdir (&stdio_FATFS, &res, "level1\\level2\\level3", 0);
  CHK(res == ior_OK);

  FAT_chdir (&stdio_FATFS, &res, "..", 0);
  CHK(res == ior_OK);

  FAT_chdir (&stdio_FATFS, &res, "..\\..", 0);
  CHK(res == ior_OK);

  FAT_rmdir (&stdio_FATFS, &res, "level1\\level2\\level3");
  CHK(res == ior_OK);

  FAT_rmdir (&stdio_FATFS, &res, "level1\\level2");
  CHK(res == ior_OK);

  FAT_rmdir (&stdio_FATFS, &res, "level1");
  CHK(res == ior_OK);

  FAT_statvfs (&stdio_FATFS, &res, "\\", &FSStatus2);
  CHK(res == ior_OK);
  CHK(FSStatus2.TotalFreeClusterCount == FSStatus1.TotalFreeClusterCount);

  result = 0;

lend:
  return result;
}

uint TestDir10(void)
{
  tIORes res;
  uint result = 1;
  uint i, cnt;
  char name[12+1+13];
  tReadDirState ReadDirState;
  tFATDirectoryEntry DirEntry;
  tFATFSStatus FSStatus1, FSStatus2;
  tFATFile file;
  uint32 FreeSpace1, FreeSpace2;
  int subtest;

  // tbd: allocate almost all free space to big file(s), leave 1 free cluster
  // create directory ending up with disk full, create empty files in it
  // until it needs to grow and fails
  
  FAT_statvfs (&stdio_FATFS, &res, "\\", &FSStatus1);
  CHK(res == ior_OK);

  FAT_mkdir (&stdio_FATFS, &res, "level1", mt_S_IWUSR);
  CHK(res == ior_OK);

  for (subtest = 0; subtest < 2; subtest++)
  {
    FAT_fopen (&stdio_FATFS, &res, &file, "huge0000", "wb");
    CHK(res == ior_OK);

    FAT_statvfs (&stdio_FATFS, &res, "\\", &FSStatus2);
    CHK(res == ior_OK);

    FreeSpace1 = FSStatus2.TotalFreeClusterCount * FSStatus2.ClusterSize;
    CHK(FreeSpace1 / FSStatus2.TotalFreeClusterCount ==
        FSStatus2.ClusterSize);
    CHK(FreeSpace1 >= 2 * FSStatus2.ClusterSize);

    if (!subtest)
    {
      FreeSpace2 = 0;
    }
    else
    {
      FreeSpace1 -= FSStatus2.ClusterSize;
      FreeSpace2 = FSStatus2.ClusterSize;
    }

    FAT_fseek (&stdio_FATFS, &res, &file, FreeSpace1);
    CHK(res == ior_OK);

    FAT_fclose (&stdio_FATFS, &res, &file);
    CHK(res == ior_OK);

    FAT_fopen (&stdio_FATFS, &res, &file, "huge0001", "wb");
    CHK(res == ior_OK);

    FAT_fseek (&stdio_FATFS, &res, &file, FreeSpace2);
    CHK(res == ior_OK);

    FAT_fclose (&stdio_FATFS, &res, &file);
    CHK(res == ior_OK);

    if (subtest)
    {
      int idx;
      for (idx = 0; idx < 2; idx++)
      {
        FAT_unlink (&stdio_FATFS, &res, "huge0000");
        CHK(res == ior_OK);

        FAT_fopen (&stdio_FATFS, &res, &file, "huge0000", "wb");
        CHK(res == ior_OK);

        FAT_fseek (&stdio_FATFS, &res, &file, FreeSpace1);
        CHK(res == ior_OK);

        FAT_fclose (&stdio_FATFS, &res, &file);
        CHK(res == ior_OK);
      }
    }

    for (cnt = i = 0; i < UINT_MAX; i++)
    {
      strcpy (name, "level1\\");
      sprintf (name + strlen(name), "%u", i);
      FAT_fopen (&stdio_FATFS, &res, &file, name, "wb");
      CHK((res == ior_OK) || (res == ior_NO_SPACE));
      if (res == ior_OK)
      {
        FAT_fclose (&stdio_FATFS, &res, &file);
        CHK(res == ior_OK);
        cnt++;
      }
      else
        break;
    }

    CHK(cnt == 16 - 2);

    FAT_opendir (&stdio_FATFS, &res, "level1", &ReadDirState);
    CHK(res == ior_OK);

    for (i = 0; i < cnt; i++)
    {
      char s[13];
      sprintf (name, "%u", i);
lSkipDotEntries:
      FAT_readdir (&stdio_FATFS, &res, &ReadDirState, &DirEntry);
      CHK(res == ior_OK);
      CHK(DirEntryNameToASCIIZ (&DirEntry, s) == s);
      if (s[0] ==  '.') goto lSkipDotEntries;
      CHK(!strcmp (name, s));
    }

    FAT_readdir (&stdio_FATFS, &res, &ReadDirState, &DirEntry);
    CHK(res == ior_NO_MORE_FILES);

    FAT_closedir (&stdio_FATFS, &res, &ReadDirState);
    CHK(res == ior_OK);

    for (i = 0; i < cnt; i++)
    {
      strcpy (name, "level1\\");
      sprintf (name + strlen(name), "%u", i);
      FAT_unlink (&stdio_FATFS, &res, name);
      CHK(res == ior_OK);
    }

    FAT_unlink (&stdio_FATFS, &res, "huge0001");
    CHK(res == ior_OK);

    FAT_unlink (&stdio_FATFS, &res, "huge0000");
    CHK(res == ior_OK);
  }

  FAT_rmdir (&stdio_FATFS, &res, "level1");
  CHK(res == ior_OK);

  FAT_statvfs (&stdio_FATFS, &res, "\\", &FSStatus2);
  CHK(res == ior_OK);
  CHK(FSStatus2.TotalFreeClusterCount == FSStatus1.TotalFreeClusterCount);

  result = 0;

lend:
  return result;
}

uint TestDir11(void)
{
  tIORes res;
  uint result = 1;
  tFATFSStatus FSStatus1, FSStatus2;
  tFATFileStatus FStatus;

  FAT_statvfs (&stdio_FATFS, &res, "\\", &FSStatus1);
  CHK(res == ior_OK);

  FAT_stat (&stdio_FATFS, &res,
            "\\", &FStatus);
  CHK(res == ior_OK);
  CHK(FStatus.Attribute == (dea_DIRECTORY | dea_READ_ONLY));
  CHK(FStatus.FileSize == 0);
  CHK(FStatus.Year   == 1980);
  CHK(FStatus.Month  == 1);
  CHK(FStatus.Day    == 1);
  CHK(FStatus.Hour   == 0);
  CHK(FStatus.Minute == 0);
  CHK(FStatus.Second == 0);

  FAT_access (&stdio_FATFS, &res,
              "\\", am_F_OK);
  CHK(res == ior_OK);

  FAT_access (&stdio_FATFS, &res,
              "\\", am_R_OK | am_X_OK);
  CHK(res == ior_OK);

  FAT_access (&stdio_FATFS, &res,
              "\\", am_W_OK);
  CHK(res == ior_PROHIBITED);

  FAT_utime (&stdio_FATFS, &res,
             "\\",
             2107, 12, 31,
             23, 59, 58);
  CHK(res == ior_PROHIBITED);

  FAT_chmod (&stdio_FATFS, &res,
             "\\", mt_S_IRWXU);
  CHK(res == ior_PROHIBITED);

  FAT_utime (&stdio_FATFS, &res,
             "level1",
             2107, 12, 31,
             23, 59, 58);
  CHK(res == ior_NOT_FOUND);

  FAT_chmod (&stdio_FATFS, &res,
             "level1", mt_S_IRWXU);
  CHK(res == ior_NOT_FOUND);

  FAT_access (&stdio_FATFS, &res,
              "level1", am_R_OK);
  CHK(res == ior_NOT_FOUND);

  FAT_stat (&stdio_FATFS, &res,
            "level1", &FStatus);
  CHK(res == ior_NOT_FOUND);

  FAT_mkdir (&stdio_FATFS, &res, "level1", mt_S_IWUSR);
  CHK(res == ior_OK);

  FAT_access (&stdio_FATFS, &res,
              "level1", am_R_OK);
  CHK(res == ior_OK);

  FAT_access (&stdio_FATFS, &res,
              "level1", am_W_OK);
  CHK(res == ior_OK);

  FAT_access (&stdio_FATFS, &res,
              "level1", am_X_OK);
  CHK(res == ior_OK);

  FAT_access (&stdio_FATFS, &res,
              "level1", am_F_OK);
  CHK(res == ior_OK);

  FAT_access (&stdio_FATFS, &res,
              "level1", am_R_OK | am_X_OK);
  CHK(res == ior_OK);

  FAT_access (&stdio_FATFS, &res,
              "level1", am_R_OK | am_W_OK);
  CHK(res == ior_OK);

  FAT_access (&stdio_FATFS, &res,
              "level1", am_X_OK | am_W_OK);
  CHK(res == ior_OK);

  FAT_access (&stdio_FATFS, &res,
              "level1", am_R_OK | am_W_OK | am_X_OK);
  CHK(res == ior_OK);

  FAT_stat (&stdio_FATFS, &res,
            "level1", &FStatus);
  CHK(res == ior_OK);
  CHK(FStatus.Attribute == dea_DIRECTORY);
  CHK(FStatus.FileSize == 0);

  FAT_chmod (&stdio_FATFS, &res,
             "level1", mt_S_IRUSR | mt_S_IXUSR);
  CHK(res == ior_OK);

  FAT_stat (&stdio_FATFS, &res,
            "level1", &FStatus);
  CHK(res == ior_OK);
  CHK(FStatus.Attribute == (dea_DIRECTORY | dea_READ_ONLY));
  CHK(FStatus.FileSize == 0);

  FAT_rmdir (&stdio_FATFS, &res, "level1");
  CHK(res == ior_PROHIBITED);

  FAT_access (&stdio_FATFS, &res,
              "level1", am_R_OK);
  CHK(res == ior_OK);

  FAT_access (&stdio_FATFS, &res,
              "level1", am_W_OK);
  CHK(res == ior_PROHIBITED);

  FAT_access (&stdio_FATFS, &res,
              "level1", am_X_OK);
  CHK(res == ior_OK);

  FAT_access (&stdio_FATFS, &res,
              "level1", am_F_OK);
  CHK(res == ior_OK);

  FAT_access (&stdio_FATFS, &res,
              "level1", am_R_OK | am_X_OK);
  CHK(res == ior_OK);

  FAT_access (&stdio_FATFS, &res,
              "level1", am_R_OK | am_W_OK);
  CHK(res == ior_PROHIBITED);

  FAT_access (&stdio_FATFS, &res,
              "level1", am_X_OK | am_W_OK);
  CHK(res == ior_PROHIBITED);

  FAT_access (&stdio_FATFS, &res,
              "level1", am_R_OK | am_W_OK | am_X_OK);
  CHK(res == ior_PROHIBITED);

  FAT_utime (&stdio_FATFS, &res,
             "level1",
             2107, 12, 31,
             23, 59, 58);
  CHK(res == ior_PROHIBITED);

  FAT_chmod (&stdio_FATFS, &res,
             "level1", mt_S_IRWXU);
  CHK(res == ior_OK);

  FAT_utime (&stdio_FATFS, &res,
             "level1",
             2107, 12, 31,
             23, 59, 58);
  CHK(res == ior_OK);

  FAT_stat (&stdio_FATFS, &res,
            "level1", &FStatus);
  CHK(res == ior_OK);
  CHK(FStatus.Attribute == dea_DIRECTORY);
  CHK(FStatus.FileSize == 0);
  CHK(FStatus.Year   == 2107);
  CHK(FStatus.Month  == 12);
  CHK(FStatus.Day    == 31);
  CHK(FStatus.Hour   == 23);
  CHK(FStatus.Minute == 59);
  CHK(FStatus.Second == 58);

  FAT_utime (&stdio_FATFS, &res,
             "level1",
             1980, 1, 1,
             0, 0, 0);
  CHK(res == ior_OK);

  FAT_stat (&stdio_FATFS, &res,
            "level1", &FStatus);
  CHK(res == ior_OK);
  CHK(FStatus.Attribute == dea_DIRECTORY);
  CHK(FStatus.FileSize == 0);
  CHK(FStatus.Year   == 1980);
  CHK(FStatus.Month  == 1);
  CHK(FStatus.Day    == 1);
  CHK(FStatus.Hour   == 0);
  CHK(FStatus.Minute == 0);
  CHK(FStatus.Second == 0);

  FAT_rmdir (&stdio_FATFS, &res, "level1");
  CHK(res == ior_OK);

  FAT_mkdir (&stdio_FATFS, &res, "level1", 0);
  CHK(res == ior_OK);

  FAT_rmdir (&stdio_FATFS, &res, "level1");
  CHK(res == ior_PROHIBITED);

  FAT_chmod (&stdio_FATFS, &res,
             "level1", mt_S_IRWXU);
  CHK(res == ior_OK);

  FAT_rmdir (&stdio_FATFS, &res, "level1");
  CHK(res == ior_OK);

  FAT_statvfs (&stdio_FATFS, &res, "\\", &FSStatus2);
  CHK(res == ior_OK);
  CHK(FSStatus2.TotalFreeClusterCount == FSStatus1.TotalFreeClusterCount);

  result = 0;

lend:
  return result;
}

uint TestDir12(void)
{
  tIORes res;
  uint result = 1;
  tFATFSStatus FSStatus1, FSStatus2;
  uint disk, DiskCount;

  FAT_statvfs (&stdio_FATFS, &res, "\\", &FSStatus1);
  CHK(res == ior_OK);

  FAT_dos_getdrive (&stdio_FATFS, &res, &disk);
  CHK(res == ior_OK);
  CHK(disk == 1);

  FAT_dos_setdrive (&stdio_FATFS, &res, disk, &DiskCount);
  CHK(res == ior_OK);
//  CHK(DiskCount == 2);

  FAT_dos_setdrive (&stdio_FATFS, &res, 0, &DiskCount);
  CHK(res == ior_ARGUMENT_OUT_OF_RANGE);

  FAT_dos_setdrive (&stdio_FATFS, &res, 'Z'-'A'+1, &DiskCount);
  CHK(res == ior_ARGUMENT_OUT_OF_RANGE);

  FAT_dos_setdrive (&stdio_FATFS, &res, 'Z'-'A'+2, &DiskCount);
  CHK(res == ior_ARGUMENT_OUT_OF_RANGE);

  FAT_dos_getdrive (&stdio_FATFS, &res, &disk);
  CHK(res == ior_OK);
  CHK(disk == 1);

  FAT_statvfs (&stdio_FATFS, &res, "\\", &FSStatus2);
  CHK(res == ior_OK);
  CHK(FSStatus2.TotalFreeClusterCount == FSStatus1.TotalFreeClusterCount);

  for (disk = 'A'; disk <= 'Z'; disk++)
  {
    char path[]="A:\\";
    path[0] = disk;
    FAT_statvfs (&stdio_FATFS, &res, path, &FSStatus2);
    CHK((res == ior_OK) || (res == ior_BAD_NAME));
    if (res == ior_OK)
    {
      printf ("Disk \'%c\':\n"
              "  ClusterSize: %lu\n"
              "  TotalClusterCount: %lu\n"
              "  TotalFreeClusterCount: %lu\n\n",
              disk,
              (ulong)FSStatus2.ClusterSize,
              (ulong)FSStatus2.TotalClusterCount,
              (ulong)FSStatus2.TotalFreeClusterCount);
    }
  }

  result = 0;

lend:
  return result;
}

uint TestDir13(void)
{
  tIORes res;
  uint result = 1;
//  tFATFSStatus FSStatus1;

  FAT_mkdir (&stdio_FATFS, &res, "d:\\level1", mt_S_IWUSR);
  CHK(res == ior_OK);

  result = 0;

lend:
  return result;
}

uint TestDir14(void)
{
  tIORes res;
  uint result = 1;
  char name[13];
  tReadDirState ReadDirState;
  tFATDirectoryEntry DirEntry;
//  tFATFSStatus FSStatus2;

  FAT_opendir (&stdio_FATFS, &res, "d:\\", &ReadDirState);
  CHK(res == ior_OK);

  FAT_readdir (&stdio_FATFS, &res, &ReadDirState, &DirEntry);
  CHK(res == ior_OK);

  CHK(DirEntryNameToASCIIZ (&DirEntry, name) == name);
  CHK(!strcmp(name, "LEVEL1"));

  FAT_readdir (&stdio_FATFS, &res, &ReadDirState, &DirEntry);
  CHK(res == ior_NO_MORE_FILES);

  FAT_closedir (&stdio_FATFS, &res, &ReadDirState);
  CHK(res == ior_OK);

  FAT_rmdir (&stdio_FATFS, &res, "d:\\level1");
  CHK(res == ior_OK);

  result = 0;

lend:
  return result;
}

uint TestDir15(void)
{
  tIORes res;
  uint result = 1;
  tReadDirState ReadDirState;
  tFATDirectoryEntry DirEntry;
//  tFATFSStatus FSStatus2;

  FAT_opendir (&stdio_FATFS, &res, "d:\\", &ReadDirState);
  CHK(res == ior_OK);

  FAT_readdir (&stdio_FATFS, &res, &ReadDirState, &DirEntry);
  CHK(res == ior_NO_MORE_FILES);

  FAT_closedir (&stdio_FATFS, &res, &ReadDirState);
  CHK(res == ior_OK);

  result = 0;

lend:
  return result;
}

uint TestFile1(void)
{
  tIORes res;
  uint result = 1;
  tFATFile file;
  tFATFSStatus FSStatus1, FSStatus2;

  FAT_statvfs (&stdio_FATFS, &res, "\\", &FSStatus1);
  CHK(res == ior_OK);

  FAT_fopen (&stdio_FATFS, &res, &file, ".", "rb");
  CHK(res == ior_DIR_FOUND);

  FAT_unlink (&stdio_FATFS, &res, ".");
  CHK(res == ior_DIR_FOUND);

  FAT_unlink (&stdio_FATFS, &res, "file1");
  CHK(res == ior_NOT_FOUND);

  FAT_fopen (&stdio_FATFS, &res, &file, "file1", "rb");
  CHK(res == ior_NOT_FOUND);

  FAT_fopen (&stdio_FATFS, &res, &file, "level1\\file1", "wb");
  CHK(res == ior_BAD_NAME);

  FAT_fopen (&stdio_FATFS, &res, &file, "\xE5", "wb");
  CHK(res == ior_BAD_NAME);

  FAT_fopen (&stdio_FATFS, &res, &file, "z:\\file1", "rb");
  CHK(res == ior_BAD_NAME);

  FAT_fopen (&stdio_FATFS, &res, &file, NULL, "rb");
  CHK(res == ior_INVALID_ARGUMENT);

  FAT_fopen (&stdio_FATFS, &res, &file, "file1", NULL);
  CHK(res == ior_INVALID_ARGUMENT);

  FAT_fopen (&stdio_FATFS, &res, &file, "file1", "blah");
  CHK(res == ior_INVALID_ARGUMENT);

  FAT_fopen (&stdio_FATFS, &res, &file, "file1", "wb");
  CHK(res == ior_OK);

  FAT_fclose (&stdio_FATFS, &res, &file);
  CHK(res == ior_OK);

  FAT_fopen (&stdio_FATFS, &res, &file, "file1", "rb");
  CHK(res == ior_OK);

  FAT_fclose (&stdio_FATFS, &res, &file);
  CHK(res == ior_OK);

  FAT_fopen (&stdio_FATFS, &res, &file, "file1", "wb");
  CHK(res == ior_OK);

  FAT_fclose (&stdio_FATFS, &res, &file);
  CHK(res == ior_OK);

  FAT_fopen (&stdio_FATFS, &res, &file, "file1\\", "rb");
  CHK(res == ior_BAD_NAME);

  FAT_unlink (&stdio_FATFS, &res, "file1");
  CHK(res == ior_OK);

  FAT_unlink (&stdio_FATFS, &res, "file1");
  CHK(res == ior_NOT_FOUND);

  FAT_fopen (&stdio_FATFS, &res, &file, "file1", "rb");
  CHK(res == ior_NOT_FOUND);

  FAT_statvfs (&stdio_FATFS, &res, "\\", &FSStatus2);
  CHK(res == ior_OK);
  CHK(FSStatus2.TotalFreeClusterCount == FSStatus1.TotalFreeClusterCount);

  result = 0;

lend:
  return result;
}

uint TestFile2(void)
{
  tIORes res;
  uint result = 1;
  tFATFile file;
  tFATFSStatus FSStatus1, FSStatus2;
  char c;
  size_t s;
  uint disk;

  for (disk = 'a'; disk <= 'z'; disk++)
  {
    char DiskPath[]="a:\\";
    char FilePath[128];

    DiskPath[0] = disk;

    FAT_statvfs (&stdio_FATFS, &res, DiskPath, &FSStatus1);
    if (((res == ior_ERR_IO) && (disk < 'c')) || (res == ior_BAD_NAME))
      continue;
    CHK(res == ior_OK);

    strcpy (FilePath, "a:file1");
    FilePath[0] = disk;
    FAT_fopen (&stdio_FATFS, &res, &file, FilePath, "wb");
    CHK(res == ior_OK);

    s = FAT_fwrite (&stdio_FATFS, &res, "A", 1, 1, &file);
    CHK((res == ior_OK) && (s == 1));

    FAT_rewind (&stdio_FATFS, &res, &file);
    CHK(res == ior_OK);

    c = 'z';
    s = FAT_fread (&stdio_FATFS, &res, &c, 1, 1, &file);
    CHK((res == ior_OK) && (s == 1) && (c == 'A'));

    FAT_fclose (&stdio_FATFS, &res, &file);
    CHK(res == ior_OK);

    strcpy (FilePath, "a:\\.\\file1");
    FilePath[0] = disk;
    FAT_fopen (&stdio_FATFS, &res, &file, FilePath, "rb");
    CHK(res == ior_OK);

    c = 'z';
    s = FAT_fread (&stdio_FATFS, &res, &c, 1, 1, &file);
    CHK((res == ior_OK) && (s == 1) && (c == 'A'));

    FAT_fclose (&stdio_FATFS, &res, &file);
    CHK(res == ior_OK);

    strcpy (FilePath, "a:file1");
    FilePath[0] = disk;
    FAT_fopen (&stdio_FATFS, &res, &file, FilePath, "wb");
    CHK(res == ior_OK);

    c = 'z';
    s = FAT_fread (&stdio_FATFS, &res, &c, 1, 1, &file);
    CHK((res == ior_OK) && (s == 0) && (c == 'z'));

    FAT_fclose (&stdio_FATFS, &res, &file);
    CHK(res == ior_OK);

    strcpy (FilePath, "a:file1");
    FilePath[0] = disk;
    FAT_unlink (&stdio_FATFS, &res, FilePath);
    CHK(res == ior_OK);

    FAT_statvfs (&stdio_FATFS, &res, DiskPath, &FSStatus2);
    CHK(res == ior_OK);
    CHK(FSStatus2.TotalFreeClusterCount == FSStatus1.TotalFreeClusterCount);
  }

  result = 0;

lend:
  return result;
}

int CreateTestFile (tIORes* pRes, tFATFile* pFile, const char* name,
                    unsigned long size, int close)
{
  unsigned long pos;
  size_t s;
  uchar c;

  FAT_fopen (&stdio_FATFS, pRes, pFile, name, "wb");

  if (*pRes != ior_OK)
    return 0;

  for (pos = 0; pos < size; pos++)
  {
    c = (uchar)(pos % 251);
    s = FAT_fwrite (&stdio_FATFS, pRes, &c, 1, 1, pFile);

    if (*pRes != ior_OK)
      return 0;
    if (s != 1)
    {
      *pRes = ior_PROHIBITED; // $$$
      return 0;
    }
  }

  if (close)
  {
    FAT_fclose (&stdio_FATFS, pRes, pFile);
    if (*pRes != ior_OK)
      return 0;
  }
  else
  {
    FAT_rewind (&stdio_FATFS, pRes, pFile);
    if (*pRes != ior_OK)
      return 0;
  }

  return 1;
}

#define MAX_CLUSTER_SIZE        32768

uint TestFile3(void)
{
  tIORes res;
  uint result = 1;
  tFATFile file;
  tFATFSStatus FSStatus1, FSStatus2;
  uchar buf[4UL*MAX_CLUSTER_SIZE+1];
  uint32 aFileSize[]=
  {0, 1,                                                  // ~= 0
   FAT_SECTOR_SIZE-1, FAT_SECTOR_SIZE, FAT_SECTOR_SIZE+1, // ~= sector
   -1, 0, 1,                                              // ~= cluster
   -1, 0, 1,                                              // ~= 2 clusters
   -1, 0, 1};                                             // ~= 4 clusters
  int FileSizeCount = 5;
  int FileSizeIndex = 0;
  int CloseIndex;
  int ReadSizeIndex = 0;

  FAT_statvfs (&stdio_FATFS, &res, "\\", &FSStatus1);
  CHK(res == ior_OK);

  if (FSStatus1.ClusterSize != FAT_SECTOR_SIZE)
  {
    aFileSize[FileSizeCount++] = FSStatus1.ClusterSize - 1;
    aFileSize[FileSizeCount++] = FSStatus1.ClusterSize;
    aFileSize[FileSizeCount++] = FSStatus1.ClusterSize + 1;
  }
  aFileSize[FileSizeCount++] = 2 * FSStatus1.ClusterSize - 1;
  aFileSize[FileSizeCount++] = 2 * FSStatus1.ClusterSize;
  aFileSize[FileSizeCount++] = 2 * FSStatus1.ClusterSize + 1;

  aFileSize[FileSizeCount++] = 4 * FSStatus1.ClusterSize - 1;
  aFileSize[FileSizeCount++] = 4 * FSStatus1.ClusterSize;
  aFileSize[FileSizeCount++] = 4 * FSStatus1.ClusterSize + 1;

  for (CloseIndex = 0; CloseIndex <= 1; CloseIndex++)
  {
    for (FileSizeIndex = 0; FileSizeIndex < FileSizeCount; FileSizeIndex++)
    {
      for (ReadSizeIndex = 0; ReadSizeIndex < FileSizeCount; ReadSizeIndex++)
      {
        uint32 size, pos=0, ReadPos;
        size_t ReadSize, s;
        int IsEof;

        size = (size_t)aFileSize[FileSizeIndex];
        ReadSize = aFileSize[ReadSizeIndex];

        CreateTestFile (&res, &file, "file1",
                        size, CloseIndex);
        CHK(res == ior_OK);

        if (!CloseIndex)
        {
          FAT_rewind (&stdio_FATFS, &res, &file);
          CHK(res == ior_OK);
        }
        else
        {
          FAT_fopen (&stdio_FATFS, &res, &file, "file1", "rb");
          CHK(res == ior_OK);
        }

        FAT_fsize (&stdio_FATFS, &res, &file, &ReadPos);
        CHK(res == ior_OK);
        CHK(size == ReadPos);

        FAT_feof (&stdio_FATFS, &res, &file, &IsEof);
        CHK(res == ior_OK);
        CHK(size ? !IsEof : IsEof);

        FAT_ftell (&stdio_FATFS, &res, &file, &ReadPos);
        CHK(res == ior_OK);
        CHK(pos == ReadPos);

        while (pos < size)
        {
          uint32 i;

          s = FAT_fread (&stdio_FATFS, &res, buf, 1, ReadSize, &file);
          CHK(res == ior_OK);
          CHK(s == MIN(size - pos, ReadSize));

          if (!ReadSize)
            break;

          for (i = 0; i < s; i++)
          {
            CHK(buf[i] == (pos + i) % 251);
          }

          pos += s;
          FAT_ftell  (&stdio_FATFS, &res, &file, &ReadPos);
          CHK(res == ior_OK);
          CHK(pos == ReadPos);
        }

        if (ReadSize)
        {
          s = FAT_fread (&stdio_FATFS, &res, buf, 1, ReadSize, &file);
          CHK(res == ior_OK);
          CHK(s == 0);

          CHK(pos == size);

          FAT_feof (&stdio_FATFS, &res, &file, &IsEof);
          CHK(res == ior_OK);
          CHK(IsEof);
        }

        FAT_fclose (&stdio_FATFS, &res, &file);
        CHK(res == ior_OK);

        FAT_unlink (&stdio_FATFS, &res, "file1");
        CHK(res == ior_OK);
      }
    }
  }

  FAT_statvfs (&stdio_FATFS, &res, "\\", &FSStatus2);
  CHK(res == ior_OK);
  CHK(FSStatus2.TotalFreeClusterCount == FSStatus1.TotalFreeClusterCount);

  result = 0;

lend:
  return result;
}

uint TestFile4(void)
{
  tIORes res;
  uint result = 1;
  tFATFile file;
  tFATFSStatus FSStatus1, FSStatus2;
  uint32 aFileSize[]=
  {0, 1,                                                  // ~= 0
   FAT_SECTOR_SIZE-1, FAT_SECTOR_SIZE, FAT_SECTOR_SIZE+1, // ~= sector
   -1, 0, 1,                                              // ~= cluster
   -1, 0, 1,                                              // ~= 2 clusters
   -1, 0, 1};                                             // ~= 4 clusters
  int FileSizeCount = 5;
  int FileSizeIndex = 0;
  int CloseIndex;
  int ReadSizeIndex = 0;

  FAT_statvfs (&stdio_FATFS, &res, "\\", &FSStatus1);
  CHK(res == ior_OK);

  if (FSStatus1.ClusterSize != FAT_SECTOR_SIZE)
  {
    aFileSize[FileSizeCount++] = FSStatus1.ClusterSize - 1;
    aFileSize[FileSizeCount++] = FSStatus1.ClusterSize;
    aFileSize[FileSizeCount++] = FSStatus1.ClusterSize + 1;
  }
  aFileSize[FileSizeCount++] = 2 * FSStatus1.ClusterSize - 1;
  aFileSize[FileSizeCount++] = 2 * FSStatus1.ClusterSize;
  aFileSize[FileSizeCount++] = 2 * FSStatus1.ClusterSize + 1;

  aFileSize[FileSizeCount++] = 4 * FSStatus1.ClusterSize - 1;
  aFileSize[FileSizeCount++] = 4 * FSStatus1.ClusterSize;
  aFileSize[FileSizeCount++] = 4 * FSStatus1.ClusterSize + 1;

  for (CloseIndex = 0; CloseIndex <= 1; CloseIndex++)
  {
    for (FileSizeIndex = 0; FileSizeIndex < FileSizeCount; FileSizeIndex++)
    {
      for (ReadSizeIndex = 0; ReadSizeIndex < FileSizeCount; ReadSizeIndex++)
      {
        uint32 size, pos=0, ReadPos;
        size_t ReadSize;

        size = (size_t)aFileSize[FileSizeIndex];
        ReadSize = aFileSize[ReadSizeIndex];

        CreateTestFile (&res, &file, "file1",
                        size, CloseIndex);
        CHK(res == ior_OK);

        if (!CloseIndex)
        {
          FAT_rewind (&stdio_FATFS, &res, &file);
          CHK(res == ior_OK);
        }
        else
        {
          FAT_fopen (&stdio_FATFS, &res, &file, "file1", "rb");
          CHK(res == ior_OK);
        }

        FAT_ftell (&stdio_FATFS, &res, &file, &ReadPos);
        CHK(res == ior_OK);
        CHK(pos == ReadPos);

        while (pos < size)
        {
          FAT_fseek (&stdio_FATFS, &res, &file, pos);
          CHK(res == ior_OK);

          FAT_ftell (&stdio_FATFS, &res, &file, &ReadPos);
          CHK(res == ior_OK);
          CHK(pos == ReadPos);

          pos += MIN(size - pos, ReadSize);

          if (!ReadSize)
            break;
        }

        FAT_fseek (&stdio_FATFS, &res, &file, 0);
        CHK(res == ior_OK);

        FAT_ftell (&stdio_FATFS, &res, &file, &ReadPos);
        CHK(res == ior_OK);
        CHK(ReadPos == 0);

        FAT_fseek (&stdio_FATFS, &res, &file, size);
        CHK(res == ior_OK);

        FAT_ftell (&stdio_FATFS, &res, &file, &ReadPos);
        CHK(res == ior_OK);
        CHK(ReadPos == size);

        if (CloseIndex)
        {
          FAT_fseek (&stdio_FATFS, &res, &file, size + 1);
          CHK(res == ior_ARGUMENT_OUT_OF_RANGE);

          FAT_ftell (&stdio_FATFS, &res, &file, &ReadPos);
          CHK(res == ior_OK);
          CHK(ReadPos == size);
        }

        FAT_fsize (&stdio_FATFS, &res, &file, &ReadPos);
        CHK(res == ior_OK);
        CHK(size == ReadPos);

        FAT_fclose (&stdio_FATFS, &res, &file);
        CHK(res == ior_OK);

        FAT_unlink (&stdio_FATFS, &res, "file1");
        CHK(res == ior_OK);
      }
    }
  }

  FAT_statvfs (&stdio_FATFS, &res, "\\", &FSStatus2);
  CHK(res == ior_OK);
  CHK(FSStatus2.TotalFreeClusterCount == FSStatus1.TotalFreeClusterCount);

  result = 0;

lend:
  return result;
}

uint TestFile5(void)
{
  tIORes res;
  uint result = 1;
  tFATFile file;
  tFATFSStatus FSStatus1, FSStatus2;
  tFATFileStatus FStatus;
  size_t s;

  FAT_statvfs (&stdio_FATFS, &res, "\\", &FSStatus1);
  CHK(res == ior_OK);

  FAT_utime (&stdio_FATFS, &res,
             "file1",
             2107, 12, 31,
             23, 59, 58);
  CHK(res == ior_NOT_FOUND);

  FAT_chmod (&stdio_FATFS, &res,
             "file1", mt_S_IRWXU);
  CHK(res == ior_NOT_FOUND);

  FAT_access (&stdio_FATFS, &res,
              "file1", am_R_OK);
  CHK(res == ior_NOT_FOUND);

  FAT_stat (&stdio_FATFS, &res,
            "file1", &FStatus);
  CHK(res == ior_NOT_FOUND);

  FAT_fopen (&stdio_FATFS, &res, &file, "file1", "wb");
  CHK(res == ior_OK);

  s = FAT_fwrite (&stdio_FATFS, &res, "Blah", 1, strlen("Blah"), &file);
  CHK((res == ior_OK) && (s == strlen("Blah")));

  FAT_fclose (&stdio_FATFS, &res, &file);
  CHK(res == ior_OK);

  FAT_access (&stdio_FATFS, &res,
              "file1", am_R_OK);
  CHK(res == ior_OK);

  FAT_access (&stdio_FATFS, &res,
              "file1", am_W_OK);
  CHK(res == ior_OK);

  FAT_access (&stdio_FATFS, &res,
              "file1", am_X_OK);
  CHK(res == ior_OK);

  FAT_access (&stdio_FATFS, &res,
              "file1", am_F_OK);
  CHK(res == ior_OK);

  FAT_access (&stdio_FATFS, &res,
              "file1", am_R_OK | am_X_OK);
  CHK(res == ior_OK);

  FAT_access (&stdio_FATFS, &res,
              "file1", am_R_OK | am_W_OK);
  CHK(res == ior_OK);

  FAT_access (&stdio_FATFS, &res,
              "file1", am_X_OK | am_W_OK);
  CHK(res == ior_OK);

  FAT_access (&stdio_FATFS, &res,
              "file1", am_R_OK | am_W_OK | am_X_OK);
  CHK(res == ior_OK);

  FAT_stat (&stdio_FATFS, &res,
            "file1", &FStatus);
  CHK(res == ior_OK);
  CHK(FStatus.Attribute == dea_ARCHIVE);
  CHK(FStatus.FileSize == strlen("Blah"));

  FAT_chmod (&stdio_FATFS, &res,
             "file1", mt_S_IRUSR | mt_S_IXUSR);
  CHK(res == ior_OK);

  FAT_stat (&stdio_FATFS, &res,
            "file1", &FStatus);
  CHK(res == ior_OK);
  CHK(FStatus.Attribute == (dea_ARCHIVE | dea_READ_ONLY));
  CHK(FStatus.FileSize == strlen("Blah"));

  FAT_fopen (&stdio_FATFS, &res, &file, "file1", "rb");
  CHK(res == ior_OK);

  s = FAT_fwrite (&stdio_FATFS, &res, "Blah", 1, strlen("Blah"), &file);
  CHK(res == ior_PROHIBITED);

  FAT_fclose (&stdio_FATFS, &res, &file);
  CHK(res == ior_OK);

  FAT_fopen (&stdio_FATFS, &res, &file, "file1", "wb");
  CHK(res == ior_PROHIBITED);

  FAT_fopen (&stdio_FATFS, &res, &file, "file1", "rb+");
  CHK(res == ior_PROHIBITED);

  FAT_unlink (&stdio_FATFS, &res, "file1");
  CHK(res == ior_PROHIBITED);

  FAT_access (&stdio_FATFS, &res,
              "file1", am_R_OK);
  CHK(res == ior_OK);

  FAT_access (&stdio_FATFS, &res,
              "file1", am_W_OK);
  CHK(res == ior_PROHIBITED);

  FAT_access (&stdio_FATFS, &res,
              "file1", am_X_OK);
  CHK(res == ior_OK);

  FAT_access (&stdio_FATFS, &res,
              "file1", am_F_OK);
  CHK(res == ior_OK);

  FAT_access (&stdio_FATFS, &res,
              "file1", am_R_OK | am_X_OK);
  CHK(res == ior_OK);

  FAT_access (&stdio_FATFS, &res,
              "file1", am_R_OK | am_W_OK);
  CHK(res == ior_PROHIBITED);

  FAT_access (&stdio_FATFS, &res,
              "file1", am_X_OK | am_W_OK);
  CHK(res == ior_PROHIBITED);

  FAT_access (&stdio_FATFS, &res,
              "file1", am_R_OK | am_W_OK | am_X_OK);
  CHK(res == ior_PROHIBITED);

  FAT_utime (&stdio_FATFS, &res,
             "file1",
             2107, 12, 31,
             23, 59, 58);
  CHK(res == ior_PROHIBITED);

  FAT_chmod (&stdio_FATFS, &res,
             "file1", mt_S_IRWXU);
  CHK(res == ior_OK);

  FAT_utime (&stdio_FATFS, &res,
             "file1",
             2107, 12, 31,
             23, 59, 58);
  CHK(res == ior_OK);

  FAT_stat (&stdio_FATFS, &res,
            "file1", &FStatus);
  CHK(res == ior_OK);
  CHK(FStatus.Attribute == dea_ARCHIVE);
  CHK(FStatus.FileSize == strlen("Blah"));
  CHK(FStatus.Year   == 2107);
  CHK(FStatus.Month  == 12);
  CHK(FStatus.Day    == 31);
  CHK(FStatus.Hour   == 23);
  CHK(FStatus.Minute == 59);
  CHK(FStatus.Second == 58);

  FAT_utime (&stdio_FATFS, &res,
             "file1",
             1980, 1, 1,
             0, 0, 0);
  CHK(res == ior_OK);

  FAT_stat (&stdio_FATFS, &res,
            "file1", &FStatus);
  CHK(res == ior_OK);
  CHK(FStatus.Attribute == dea_ARCHIVE);
  CHK(FStatus.FileSize == strlen("Blah"));
  CHK(FStatus.Year   == 1980);
  CHK(FStatus.Month  == 1);
  CHK(FStatus.Day    == 1);
  CHK(FStatus.Hour   == 0);
  CHK(FStatus.Minute == 0);
  CHK(FStatus.Second == 0);

  FAT_unlink (&stdio_FATFS, &res, "file1");
  CHK(res == ior_OK);

  FAT_statvfs (&stdio_FATFS, &res, "\\", &FSStatus2);
  CHK(res == ior_OK);
  CHK(FSStatus2.TotalFreeClusterCount == FSStatus1.TotalFreeClusterCount);

  result = 0;

lend:
  return result;
}

tFATFile TestFile6File1, TestFile6File2;

tFATFile* TestFile6OpenFileSearch (tFATFS* pFATFS,
                                   tReadDirState* pReadDirState,
                                   tpOpenFilesMatchFxn pOpenFilesMatch)
{
  tFATFile** ppFile;
  tFATFile* apFiles[] =
  {
    &TestFile6File1,
    &TestFile6File2,
    NULL
  };

  for (ppFile = &apFiles[0]; *ppFile != NULL; ppFile++)
  {
    if ((*ppFile)->IsOpen)
    {
      if (pOpenFilesMatch (pFATFS, *ppFile, pReadDirState))
        return *ppFile;
    }
  }

  return NULL;
}

uint TestFile6(void)
{
  tIORes res;
  uint result = 1;
  tFATFSStatus FSStatus1, FSStatus2;
  tpOpenFileSearchFxn pOldOpenFileSearch;

  pOldOpenFileSearch = stdio_FATFS.pOpenFileSearch;
  stdio_FATFS.pOpenFileSearch = &TestFile6OpenFileSearch;

  memset (&TestFile6File1, 0, sizeof(TestFile6File1));
  memset (&TestFile6File2, 0, sizeof(TestFile6File2));

  FAT_statvfs (&stdio_FATFS, &res, "\\", &FSStatus1);
  CHK(res == ior_OK);

  FAT_fopen (&stdio_FATFS, &res, &TestFile6File1, "file1", "wb");
  CHK(res == ior_OK);

  FAT_fopen (&stdio_FATFS, &res, &TestFile6File2, "file1", "wb");
  CHK(res == ior_PROHIBITED);

  FAT_fopen (&stdio_FATFS, &res, &TestFile6File2, "file1", "rb");
  CHK(res == ior_PROHIBITED);

  FAT_utime (&stdio_FATFS, &res,
             "file1",
             1980, 1, 1,
             0, 0, 0);
  CHK(res == ior_PROHIBITED);

  FAT_chmod (&stdio_FATFS, &res,
             "file1", mt_S_IRWXU);
  CHK(res == ior_PROHIBITED);

  FAT_unlink (&stdio_FATFS, &res, "file1");
  CHK(res == ior_PROHIBITED);

  FAT_fclose (&stdio_FATFS, &res, &TestFile6File1);
  CHK(res == ior_OK);

  FAT_fopen (&stdio_FATFS, &res, &TestFile6File1, "file1", "rb");
  CHK(res == ior_OK);

  FAT_unlink (&stdio_FATFS, &res, "file1");
  CHK(res == ior_PROHIBITED);

  FAT_fopen (&stdio_FATFS, &res, &TestFile6File2, "file1", "wb");
  CHK(res == ior_PROHIBITED);

  FAT_utime (&stdio_FATFS, &res,
             "file1",
             1980, 1, 1,
             0, 0, 0);
  CHK(res == ior_PROHIBITED);

  FAT_chmod (&stdio_FATFS, &res,
             "file1", mt_S_IRWXU);
  CHK(res == ior_PROHIBITED);

  FAT_fopen (&stdio_FATFS, &res, &TestFile6File2, "file1", "rb");
  CHK(res == ior_OK);

  FAT_fclose (&stdio_FATFS, &res, &TestFile6File1);
  CHK(res == ior_OK);

  FAT_fclose (&stdio_FATFS, &res, &TestFile6File2);
  CHK(res == ior_OK);

  FAT_unlink (&stdio_FATFS, &res, "file1");
  CHK(res == ior_OK);

  FAT_statvfs (&stdio_FATFS, &res, "\\", &FSStatus2);
  CHK(res == ior_OK);
  CHK(FSStatus2.TotalFreeClusterCount == FSStatus1.TotalFreeClusterCount);

  result = 0;

lend:

  stdio_FATFS.pOpenFileSearch = pOldOpenFileSearch;

  return result;
}

uint TestDirFileMixup1(void)
{
  tIORes res;
  uint result = 1;
  tFATFile file;
  tFATFSStatus FSStatus1, FSStatus2;

  FAT_statvfs (&stdio_FATFS, &res, "\\", &FSStatus1);
  CHK(res == ior_OK);

  FAT_fopen (&stdio_FATFS, &res, &file, "file1", "wb");
  CHK(res == ior_OK);

  FAT_fclose (&stdio_FATFS, &res, &file);
  CHK(res == ior_OK);

  FAT_chdir (&stdio_FATFS, &res, "file1", 0);
  CHK(res == ior_FILE_FOUND);

  FAT_mkdir (&stdio_FATFS, &res, "file1", mt_S_IWUSR);
  CHK(res == ior_FILE_FOUND);

  FAT_rmdir (&stdio_FATFS, &res, "file1");
  CHK(res == ior_FILE_FOUND);

  FAT_unlink (&stdio_FATFS, &res, "file1");
  CHK(res == ior_OK);

  FAT_fopen (&stdio_FATFS, &res, &file, "file1", "rb");
  CHK(res == ior_NOT_FOUND);

  FAT_mkdir (&stdio_FATFS, &res, "level1", mt_S_IWUSR);
  CHK(res == ior_OK);

  FAT_fopen (&stdio_FATFS, &res, &file, "level1", "rb");
  CHK(res == ior_DIR_FOUND);

  FAT_unlink (&stdio_FATFS, &res, "level1");
  CHK(res == ior_DIR_FOUND);

  FAT_chdir (&stdio_FATFS, &res, "level1", 0);
  CHK(res == ior_OK);

  FAT_chdir (&stdio_FATFS, &res, "..", 0);
  CHK(res == ior_OK);

  FAT_rmdir (&stdio_FATFS, &res, "level1");
  CHK(res == ior_OK);

  FAT_chdir (&stdio_FATFS, &res, "..", 0);
  CHK(res == ior_NOT_FOUND);

  FAT_statvfs (&stdio_FATFS, &res, "\\", &FSStatus2);
  CHK(res == ior_OK);
  CHK(FSStatus2.TotalFreeClusterCount == FSStatus1.TotalFreeClusterCount);

  result = 0;

lend:
  return result;
}

tTestCase aTestCases[] =
{
  {101, "dir: rd\\1,\\1, md\\1,\\1, rd\\1,\\1, md\\1, rd\\1", TestDir1},
  {102, "dir: md\\badname", TestDir2},
  {103, "dir: rd\\badname", TestDir3},
  {104, "dir: md\\1, rd\\1\\2,\\1\\2, md\\1\\2,\\1\\2, rd\\1\\2,\\1\\2, md\\1\\2, rd\\1, rd\\1\\2, rd\\1", TestDir4},
  {105, "dir: md\\1, md\\1\\2, rd\\1\\2\\3,\\1\\2\\3, md\\1\\2\\3,\\1\\2\\3, rd\\1\\2\\3,\\1\\2\\3, md\\1\\2\\3, rd\\1\\2, rd\\1, rd\\1\\2\\3, rd\\1\\2, rd\\1", TestDir5},
  {106, "dir: md\\l1,cd\\l,\\l11,\\l1, rd\\l1,..\\l1,.,.. cd..,rd\\l1", TestDir6},
  {107, "dir: md\\l1,cd\\l, md\\0...256, rd\\0...256, cd..,rd\\l1", TestDir7},
  {108, "dir: md\\1-max, rd\\1-max", TestDir8},
  {109, "dir: md\\1\\2\\3, cd\\1\\2\\3, cd..\\..\\.., rd1\\2\\3", TestDir9},
  {110, "dir: md\\1,fopen1\\1,1\\2,...,readdir,unlink1\\1,1\\2,...,rd\\1", TestDir10},
  {111, "dir: stat, access, chmod, utime", TestDir11},
  {112, "dir: get/set disk", TestDir12},
  {113, "dir: md", TestDir13},
  {114, "dir: readdir,rd", TestDir14},
  {115, "dir: readdir", TestDir15},

  {201, "file: open\\badname", TestFile1},
  {202, "file: r/w 1 char", TestFile2},
  {203, "file: read various sizes forward", TestFile3},
  {204, "file: seek forward", TestFile4},
  {205, "file: stat, access, chmod, utime", TestFile5},
  {206, "file: open same file twice", TestFile6},

  {301, "file/dir: file operations on dirs & vice versa", TestDirFileMixup1},

  {0, NULL, NULL}
};
