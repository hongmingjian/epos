/**
 * vim: filetype=c:fenc=utf-8:ts=4:et:sw=4:sts=4
 *
 * Copyright (C) 2013 Hong MingJian<hongmingjian@gmail.com>
 * All rights reserved.
 *
 * This file is part of the EPOS.
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
#if defined(__WIN32__) || defined(__CYGWIN__)
#include <stddef.h>
#include <string.h>
#include "kernel.h"
#include "dosfs.h"

typedef unsigned long  DWORD;
typedef unsigned long  ULONG;
typedef unsigned short USHORT;
typedef          short WORD;
typedef unsigned char  UCHAR;
typedef unsigned char  BYTE;

/**
 * These definitions come from WinNT.h of Microsoft
 */
#define IMAGE_DOS_SIGNATURE 0x5a4d

typedef struct _IMAGE_DOS_HEADER {  // DOS .EXE header
    USHORT e_magic;  // must contain "MZ"
    USHORT e_cblp;  // number of bytes on the last page of the file
    USHORT e_cp;  // number of pages in file
    USHORT e_crlc;  // relocations
    USHORT e_cparhdr;  // size of the header in paragraphs
    USHORT e_minalloc; // minimum and maximum paragraphs to allocate
    USHORT e_maxalloc;
    USHORT e_ss;  // initial SS:SP to set by Loader
    USHORT e_sp;
    USHORT e_csum;  // checksum
    USHORT e_ip;  // initial CS:IP
    USHORT e_cs;
    USHORT e_lfarlc;  // address of relocation table
    USHORT e_ovno;  // overlay number
    USHORT e_res[4];  // resevered
    USHORT e_oemid;  // OEM id
    USHORT e_oeminfo;  // OEM info
    USHORT e_res2[10]; // reserved
    ULONG  e_lfanew; // address of new EXE header
} IMAGE_DOS_HEADER, *PIMAGE_DOS_HEADER;

typedef struct _IMAGE_FILE_HEADER {
    USHORT  Machine;
    USHORT  NumberOfSections;   // Number of sections in section table
    ULONG   TimeDateStamp;   // Date and time of program link
    ULONG   PointerToSymbolTable;  // RVA of symbol table
    ULONG   NumberOfSymbols;   // Number of symbols in table
    USHORT  SizeOfOptionalHeader;  // Size of IMAGE_OPTIONAL_HEADER in bytes
    USHORT  Characteristics;
} IMAGE_FILE_HEADER, *PIMAGE_FILE_HEADER;

typedef struct _IMAGE_DATA_DIRECTORY {
    DWORD VirtualAddress;  // RVA of table
    DWORD Size;   // size of table
} IMAGE_DATA_DIRECTORY, *PIMAGE_DATA_DIRECTORY;

#define IMAGE_NUMBEROF_DIRECTORY_ENTRIES    16

typedef struct _IMAGE_OPTIONAL_HEADER {
    USHORT  Magic;    // not-so-magical number
    UCHAR   MajorLinkerVersion;   // linker version
    UCHAR   MinorLinkerVersion;
    ULONG   SizeOfCode;    // size of .text in bytes
    ULONG   SizeOfInitializedData;  // size of .bss (and others) in bytes
    ULONG   SizeOfUninitializedData;  // size of .data,.sdata etc in bytes
    ULONG   AddressOfEntryPoint;  // RVA of entry point
    ULONG   BaseOfCode;    // base of .text
    ULONG   BaseOfData;    // base of .data
    ULONG   ImageBase;    // image base VA
    ULONG   SectionAlignment;   // file section alignment
    ULONG   FileAlignment;   // file alignment
    USHORT  MajorOperatingSystemVersion; // OS version required to run image
    USHORT  MinorOperatingSystemVersion;
    USHORT  MajorImageVersion;   // version of program
    USHORT  MinorImageVersion;
    USHORT  MajorSubsystemVersion;  // Windows specific. Version of SubSystem
    USHORT  MinorSubsystemVersion;
    ULONG   Reserved1;
    ULONG   SizeOfImage;   // size of image in bytes
    ULONG   SizeOfHeaders;   // size of headers (and stub program) in bytes
    ULONG   CheckSum;    // checksum
    USHORT  Subsystem;    // Windows specific. subsystem type
    USHORT  DllCharacteristics;   // DLL properties
    ULONG   SizeOfStackReserve;   // size of stack, in bytes
    ULONG   SizeOfStackCommit;   // size of stack to commit
    ULONG   SizeOfHeapReserve;   // size of heap, in bytes
    ULONG   SizeOfHeapCommit;   // size of heap to commit
    ULONG   LoaderFlags;   // no longer used
    ULONG   NumberOfRvaAndSizes;  // number of DataDirectory entries
    IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER, *PIMAGE_OPTIONAL_HEADER;

#define IMAGE_NT_SIGNATURE 0x00004550

typedef struct _IMAGE_NT_HEADERS {
    DWORD                 Signature;
    IMAGE_FILE_HEADER     FileHeader;
    IMAGE_OPTIONAL_HEADER OptionalHeader;
} IMAGE_NT_HEADERS, *PIMAGE_NT_HEADERS;

#define  IMAGE_SIZEOF_SHORT_NAME 8

typedef struct _IMAGE_SECTION_HEADER {
    BYTE  Name[IMAGE_SIZEOF_SHORT_NAME];
    union {
        DWORD PhysicalAddress;
        DWORD VirtualSize;
    } Misc;
    DWORD VirtualAddress;
    DWORD SizeOfRawData;
    DWORD PointerToRawData;
    DWORD PointerToRelocations;
    DWORD PointerToLinenumbers;
    WORD  NumberOfRelocations;
    WORD  NumberOfLinenumbers;

    /*
     * IMAGE_SCN_MEM_EXECUTE This section is executable.
     * IMAGE_SCN_MEM_READ    This section is readable.
     * IMAGE_SCN_MEM_WRITE   The section is writeable.
     *                       If this flag isn't set in an EXE's section,
     *                       the loader should mark the memory mapped pages
     *                       as read-only or execute-only.
     */
    DWORD Characteristics;
#define IMAGE_SCN_MEM_EXECUTE 0x20000000
#define IMAGE_SCN_MEM_READ    0x40000000
#define IMAGE_SCN_MEM_WRITE   0x80000000
} IMAGE_SECTION_HEADER, *PIMAGE_SECTION_HEADER;

uint32_t load_aout(VOLINFO *pvi, char *filename)
{
    unsigned char scratch[SECTOR_SIZE];
    FILEINFO fi;
    uint32_t res;

    res = DFS_OpenFile(pvi, filename, DFS_READ, scratch, &fi);
    if(DFS_OK != res) {
        printk("task #%d: failed to open %s(%d)\r\n",
                sys_task_getid(), filename, res);
        return 0;
    }

    {
        unsigned int i, read, e_lfanew;
        unsigned char *buffer = NULL;
        int bufsize;
        PIMAGE_DOS_HEADER pidh;
        PIMAGE_NT_HEADERS pinh;
        PIMAGE_SECTION_HEADER pish;

        bufsize = sizeof(IMAGE_DOS_HEADER);
        buffer = krealloc(buffer, bufsize);
        DFS_ReadFile(&fi, scratch, buffer, &read, bufsize);
        pidh = (PIMAGE_DOS_HEADER)buffer;
        if((read != bufsize) ||
           (pidh->e_magic != IMAGE_DOS_SIGNATURE)) {
            printk("task #%d: invalid executable file %s\r\n",
                    sys_task_getid(), filename);
            kfree(buffer);
            return 0;
        }
        e_lfanew=pidh->e_lfanew;

        bufsize = sizeof(IMAGE_NT_HEADERS);
        buffer = krealloc(buffer, bufsize);
        DFS_Seek(&fi, e_lfanew, scratch);
        DFS_ReadFile(&fi, scratch, buffer, &read, bufsize);
        pinh = (PIMAGE_NT_HEADERS)buffer;
        if((read != bufsize) ||
           (pinh->Signature != IMAGE_NT_SIGNATURE)) {
            printk("task #%d: invalid executable file %s\r\n",
                    sys_task_getid(), filename);
            kfree(buffer);
            return 0;
        }

        bufsize = pinh->OptionalHeader.SizeOfHeaders - e_lfanew;
        buffer = krealloc(buffer, bufsize);
        DFS_Seek(&fi, e_lfanew, scratch);
        DFS_ReadFile(&fi, scratch, buffer, &read, bufsize);
        if(read != bufsize) {
            printk("task #%d: invalid executable file %s\r\n",
                    sys_task_getid(), filename);
            kfree(buffer);
            return 0;
        }

        uint32_t va, npages, prot;
        pinh = (PIMAGE_NT_HEADERS)buffer;
        pish = (PIMAGE_SECTION_HEADER)(pinh + 1);
        for(i = 0; i < pinh->FileHeader.NumberOfSections; i++, pish++) {

            if(pish->Name[0] == '/')
                continue;

            /*pish->Name[IMAGE_SIZEOF_SHORT_NAME-1] = 0;
            printk("task #%d: %s@0x%08x:%d -> 0x%08x:%d\r\n",
                   sys_task_getid(), pish->Name, pish->PointerToRawData,
                   pish->SizeOfRawData,
                   pinh->OptionalHeader.ImageBase+pish->VirtualAddress,
                   pish->Misc.VirtualSize);*/

            prot = VM_PROT_NONE;
            if(pish->Characteristics & IMAGE_SCN_MEM_EXECUTE)
                prot |= VM_PROT_EXEC;
            if(pish->Characteristics & IMAGE_SCN_MEM_READ)
                prot |= VM_PROT_READ;
            if(pish->Characteristics & IMAGE_SCN_MEM_WRITE)
                prot |= VM_PROT_WRITE;

            if(pish->PointerToRawData) {
                npages = PAGE_ROUNDUP(pish->SizeOfRawData)/PAGE_SIZE;
                va = page_alloc_in_addr(pinh->OptionalHeader.ImageBase+pish->VirtualAddress,
                                        npages, prot);
                if(va != pinh->OptionalHeader.ImageBase+pish->VirtualAddress) {
                    printk("task #%d: Address 0x%08x of %d pages has already been used!\r\n",
                           sys_task_getid(),
                           pinh->OptionalHeader.ImageBase+pish->VirtualAddress,
                           npages);
                    return 0;
                }

                DFS_Seek(&fi, pish->PointerToRawData, scratch);
                DFS_ReadFile(&fi, scratch,
                             (void *)(pinh->OptionalHeader.ImageBase+
                                      pish->VirtualAddress),
                             &read, pish->SizeOfRawData);
            } else {
                if(pish->Misc.VirtualSize) {
                    npages = PAGE_ROUNDUP(pish->Misc.VirtualSize)/PAGE_SIZE;
                    va = page_alloc_in_addr(pinh->OptionalHeader.ImageBase+pish->VirtualAddress,
                                            npages, prot);
                    if(va != pinh->OptionalHeader.ImageBase+pish->VirtualAddress) {
                        printk("task #%d: Address 0x%08x of %d pages has already been used!\r\n",
                               sys_task_getid(),
                               pinh->OptionalHeader.ImageBase+pish->VirtualAddress,
                               npages);
                        return 0;
                    }
                }
            }

            if(pish->Misc.VirtualSize > pish->SizeOfRawData) {
                memset((void *)(pinh->OptionalHeader.ImageBase+
                                pish->VirtualAddress+
                                pish->SizeOfRawData),
                       0,
                       pish->Misc.VirtualSize-pish->SizeOfRawData);
            }
        }

        kfree(buffer);

        return (pinh->OptionalHeader.ImageBase+
                pinh->OptionalHeader.AddressOfEntryPoint);
    }

    return 0;
}
#endif /*__WIN32__*/
