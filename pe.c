/**
 *
 * Copyright (C) 2013 Hong MingJian
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are freely
 * permitted provided that the above copyright notice and this
 * paragraph and the following disclaimer are duplicated in all
 * such forms.
 *
 * This software is provided "AS IS" and without any express or
 * implied warranties, including, without limitation, the implied
 * warranties of merchantability and fitness for a particular
 * purpose.
 *
 */
#include "kernel.h"
#include "utils.h"
#include "pe.h"

uint32_t load_pe(char *file)
{
  int fd;

  fd=fat_fopen(file, O_RDONLY);
  if(fd >= 0) {
    int i, read;
    char buffer[512];
    PIMAGE_DOS_HEADER pidh;
    PIMAGE_NT_HEADERS pinh;
    PIMAGE_SECTION_HEADER pish;

//    printk("%s: size=%d bytes\n\r", file, fat_fgetsize(fd));

    read = fat_fread(fd, (void *)buffer, sizeof(buffer));
//    printk("%s: %d bytes expected, %d bytes read\n\r", file, sizeof(buffer), read);

    pidh = (PIMAGE_DOS_HEADER)buffer;
    pinh = (PIMAGE_NT_HEADERS)((BYTE*)pidh + pidh->e_lfanew);
    pish = (PIMAGE_SECTION_HEADER)(pinh + 1);

    if((pidh->e_magic   != IMAGE_DOS_SIGNATURE) || 
       (pinh->Signature != IMAGE_NT_SIGNATURE)) {
      printk("%s: invalid .EXE file\n\r", file);
      return 0;
    }

    for(i = 0; i < pinh->FileHeader.NumberOfSections; i++, pish++) {

      if(pish->Name[0] == '/')
        continue;

      if(pish->PointerToRawData) {
        fat_fseek(fd, pish->PointerToRawData, SEEK_SET);
        read = fat_fread(fd, (void *)(pinh->OptionalHeader.ImageBase+pish->VirtualAddress), pish->SizeOfRawData);
//        pish->Name[IMAGE_SIZEOF_SHORT_NAME-1] = 0;
//        printk("%s%s: %d bytes expected, %d bytes read\n\r", file, pish->Name, pish->SizeOfRawData, read);
      }

      if(pish->Misc.VirtualSize > pish->SizeOfRawData)
        memset((void *)(pinh->OptionalHeader.ImageBase+pish->SizeOfRawData), 0, pish->Misc.VirtualSize-pish->SizeOfRawData);
    }

    fat_fclose(fd);

    return pinh->OptionalHeader.ImageBase+pinh->OptionalHeader.AddressOfEntryPoint;
  }

  printk("%s: failed to open\n\r", file);

  return 0;
}
