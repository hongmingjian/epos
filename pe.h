#ifndef _PE_H
#define _PE_H

typedef unsigned long  DWORD;
typedef unsigned long  ULONG;
typedef unsigned short USHORT;
typedef          short WORD;
typedef unsigned char  UCHAR;
typedef unsigned char  BYTE;

#define IMAGE_DOS_SIGNATURE 0x5a4d

typedef struct _IMAGE_DOS_HEADER {  // DOS .EXE header
    USHORT e_magic;		// must contain "MZ"
    USHORT e_cblp;		// number of bytes on the last page of the file
    USHORT e_cp;		// number of pages in file
    USHORT e_crlc;		// relocations
    USHORT e_cparhdr;		// size of the header in paragraphs
    USHORT e_minalloc;	// minimum and maximum paragraphs to allocate
    USHORT e_maxalloc;
    USHORT e_ss;		// initial SS:SP to set by Loader
    USHORT e_sp;
    USHORT e_csum;		// checksum
    USHORT e_ip;		// initial CS:IP
    USHORT e_cs;
    USHORT e_lfarlc;		// address of relocation table
    USHORT e_ovno;		// overlay number
    USHORT e_res[4];		// resevered
    USHORT e_oemid;		// OEM id
    USHORT e_oeminfo;		// OEM info
    USHORT e_res2[10];	// reserved
    ULONG  e_lfanew;	// address of new EXE header
} IMAGE_DOS_HEADER, *PIMAGE_DOS_HEADER;

typedef struct _IMAGE_FILE_HEADER {
    USHORT  Machine;
    USHORT  NumberOfSections;			// Number of sections in section table
    ULONG   TimeDateStamp;			// Date and time of program link
    ULONG   PointerToSymbolTable;		// RVA of symbol table
    ULONG   NumberOfSymbols;			// Number of symbols in table
    USHORT  SizeOfOptionalHeader;		// Size of IMAGE_OPTIONAL_HEADER in bytes
    USHORT  Characteristics;
} IMAGE_FILE_HEADER, *PIMAGE_FILE_HEADER;

typedef struct _IMAGE_DATA_DIRECTORY {
  DWORD VirtualAddress;		// RVA of table
  DWORD Size;			// size of table
} IMAGE_DATA_DIRECTORY, *PIMAGE_DATA_DIRECTORY;

#define IMAGE_NUMBEROF_DIRECTORY_ENTRIES    16

typedef struct _IMAGE_OPTIONAL_HEADER {
    USHORT  Magic;				// not-so-magical number
    UCHAR   MajorLinkerVersion;			// linker version
    UCHAR   MinorLinkerVersion;
    ULONG   SizeOfCode;				// size of .text in bytes
    ULONG   SizeOfInitializedData;		// size of .bss (and others) in bytes
    ULONG   SizeOfUninitializedData;		// size of .data,.sdata etc in bytes
    ULONG   AddressOfEntryPoint;		// RVA of entry point
    ULONG   BaseOfCode;				// base of .text
    ULONG   BaseOfData;				// base of .data
    ULONG   ImageBase;				// image base VA
    ULONG   SectionAlignment;			// file section alignment
    ULONG   FileAlignment;			// file alignment
    USHORT  MajorOperatingSystemVersion;	// Windows specific. OS version required to run image
    USHORT  MinorOperatingSystemVersion;
    USHORT  MajorImageVersion;			// version of program
    USHORT  MinorImageVersion;
    USHORT  MajorSubsystemVersion;		// Windows specific. Version of SubSystem
    USHORT  MinorSubsystemVersion;
    ULONG   Reserved1;
    ULONG   SizeOfImage;			// size of image in bytes
    ULONG   SizeOfHeaders;			// size of headers (and stub program) in bytes
    ULONG   CheckSum;				// checksum
    USHORT  Subsystem;				// Windows specific. subsystem type
    USHORT  DllCharacteristics;			// DLL properties
    ULONG   SizeOfStackReserve;			// size of stack, in bytes
    ULONG   SizeOfStackCommit;			// size of stack to commit
    ULONG   SizeOfHeapReserve;			// size of heap, in bytes
    ULONG   SizeOfHeapCommit;			// size of heap to commit
    ULONG   LoaderFlags;			// no longer used
    ULONG   NumberOfRvaAndSizes;		// number of DataDirectory entries
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
  DWORD Characteristics;
} IMAGE_SECTION_HEADER, *PIMAGE_SECTION_HEADER;

#endif
