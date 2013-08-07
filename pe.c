#include "kernel.h"
#include "utils.h"
#include "pe.h"

uint32_t load_pe(char *file)
{
  int fd;

  fd=fat_fopen(file, O_RDONLY);
  if(fd >= 0) {
    int i, read;
    char *buffer;
    PIMAGE_DOS_HEADER pidh;
    PIMAGE_NT_HEADERS pinh;
    PIMAGE_SECTION_HEADER pish;

//    printk("%s: size=%d bytes\n\r", file, fat_fgetsize(fd));

    buffer = kmalloc(PAGE_SIZE);
    read = fat_fread(fd, (void *)buffer, PAGE_SIZE);
//    printk("%s: %d bytes expected, %d bytes read\n\r", file, PAGE_SIZE, read);

    pidh = (PIMAGE_DOS_HEADER)buffer;
    pinh = (PIMAGE_NT_HEADERS)((BYTE*)pidh + pidh->e_lfanew);
    pish = (PIMAGE_SECTION_HEADER)(pinh + 1);

    if((pidh->e_magic   != IMAGE_DOS_SIGNATURE) || 
       (pinh->Signature != IMAGE_NT_SIGNATURE)) {
      kfree(buffer);
      return 0;
    }

    for(i = 0; i < pinh->FileHeader.NumberOfSections; i++, pish++) {
      DWORD vs;
      uint32_t flags;

      if(pish->Name[0] == '/')
        continue;

      vs = 0;
      while(vs < pish->Misc.VirtualSize) {
        save_flags_cli(flags);
        do_page_fault(pinh->OptionalHeader.ImageBase+pish->VirtualAddress+vs, PTE_U);
        restore_flags(flags);
        vs += PAGE_SIZE;
      }

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
    kfree(buffer);

    return pinh->OptionalHeader.ImageBase+pinh->OptionalHeader.AddressOfEntryPoint;
  }

  printk("%s: failed to open\n\r", file);

  return 0;
}
