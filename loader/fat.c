/*
 * Copyright (C) 2008 MingJian Hong
 * All rights reserved.
 *
 * Authors: Yun Long<>
 *          MingJian Hong<hmj@cqu.edu.cn>
 *
 */
 
#include "utils.h"
#include "loader.h"

struct BOOTSECT {
    uint8_t    BS_jmpBoot[3];               /* jump inst E9xxxx or EBxx90 */
    char       BS_OEMName[8];               /* OEM name and version */
    char       BS_BPB[25];                  /* BIOS parameter block */
    char       BS_EXT[54];                  /* Bootsector Extension */
    char       BS_BootCode[420];            /* pad so structure is 512b */
    uint8_t    BS_BootSectSig0;             /* boot sector signature byte 0x55 */
  	uint8_t    BS_BootSectSig1;             /* boot sector signature byte 0xAA */
#define BOOTSIG0        0x55
#define BOOTSIG1        0xaa
}__attribute__ ((packed));

#define BS_BPB_OFFSET		__offsetof(struct BOOTSECT, BS_BPB)
struct BPB {
  	uint16_t   BPB_BytsPerSec; 			/* bytes per sector */
    uint8_t    BPB_SecPerClus; 			/* sectors per cluster */
    uint16_t   BPB_RsvdSecCnt;  			/* number of reserved sectors */
    uint8_t    BPB_NumFATs;       			/* number of FATs */
    uint16_t   BPB_RootEntCnt; 			/* number of root directory entries */
    uint16_t   BPB_TotSec16;     			/* total number of sectors */
    uint8_t    BPB_Media;       			/* media descriptor */
    uint16_t   BPB_FATSz16;     			/* number of sectors per FAT */
    uint16_t   BPB_SecPerTrk; 				/* sectors per track */
    uint16_t   BPB_NumHeads;      			/* number of heads */
    uint32_t   BPB_HiddSec;	  			/* # of hidden sectors */
    uint32_t   BPB_TotSec32;	 			/* # of sectors if bpbSectors == 0 */
}__attribute__ ((packed));

#define BS_EXT_OFFSET		__offsetof(struct BOOTSECT, BS_EXT)
struct BS_EXT {
    uint8_t    BS_DrvNum;             		/* drive number (0x80) */
    uint8_t    BS_Reserved1;               /* reserved */
    uint8_t    BS_BootSig;			        /* ext. boot signature (0x29) */
#define BOOTSIGEXT       0x29
    uint32_t   BS_VolID;	                /* volume ID number */
    char       BS_VolLab[11];		        /* volume label */
    char       BS_FilsysType[8];           /* fs type (FAT12 or FAT16) */
}__attribute__ ((packed));

struct BPB_FAT32 {
	uint32_t	BPB_FATSz32;				/* FAT32文件系统的FAT表占用的扇区数 */
	uint16_t	BPB_ExtFlags;				/* FAT32文件系统的FAT激活标记 */ 
	uint16_t	BPB_FSVer;					/* FAT32文件系统的版本号 */
	uint32_t	BPB_RootClus;				/* FAT32文件系统根目录的起始簇号，通常为2 */
	uint16_t	BPB_FSInfo;					/* FSInfo结构体的在Reserved Area的扇区号，通常为1 */
	uint16_t	BPB_BkBootSec;			/* Boot Record在Reserved Area的扇区号，通常为6 */
	char      BPB_Reserved[12];
	
	struct BS_EXT BS_EXT;
}__attribute__ ((packed));

struct DIRENTRY
{
    uint8_t        DIR_Name[11];     		/* filename, blank filled */
 #define SLOT_EMPTY      0x00           /* slot has never been used */
 #define SLOT_E5         0x05           /* the real value is 0xe5 */
 #define SLOT_DELETED    0xe5           /* file in this slot deleted */
 #define SLOT_IS_DIR     0x2e           /* this is directory */

    uint8_t        DIR_Attr;   	        /* file attributes */
 #define ATTR_READ_ONLY  0x01           /* file is readonly */
 #define ATTR_HIDDEN     0x02           /* file is hidden */
 #define ATTR_SYSTEM     0x04           /* file is a system file */
 #define ATTR_VOLUME_ID  0x08           /* entry is a volume label */
 #define ATTR_DIRECTORY  0x10           /* entry is a directory name */
 #define ATTR_ARCHIVE    0x20           /* file is new or modified */
 #define ATTR_LONG_FILENAME \
                         (ATTR_READ_ONLY|ATTR_HIDDEN|ATTR_SYSTEM|ATTR_VOLUME_ID)       /* this is a long filename entry */

    uint8_t        DIR_NTRes;    		    /* NT VFAT lower case flags */
 #define LCASE_BASE      0x08           /* filename base in lower case */
 #define LCASE_EXT       0x10           /* filename extension in lower case */

    uint8_t        DIR_CrtTimeTenth;   	/* hundredth of seconds in CTime */
    uint16_t        DIR_CrtTime;     		/* create time */
    uint16_t        DIR_CrtDate;     		/* create date */
    uint16_t        DIR_LstAccDate;    	/* access date */
    uint16_t        DIR_FstClusHI;    	/* high bytes of cluster number */
    uint16_t        DIR_WrtTime;     		/* last update time */
    uint16_t        DIR_WrtDate;     		/* last update date */
    uint16_t        DIR_FstClusLO; 		  /* starting cluster of file */
    uint32_t       DIR_FileSize;     		/* size of file in bytes */
}__attribute__ ((packed));

#define FAT12_EOC       0x00000ff8UL
#define FAT16_EOC       0x0000fff8UL
#define FAT32_EOC		    0x0ffffff8UL

#define BYTES_PER_SECTOR		512

struct FAT_STRUCT {
	uint8_t	FATType;
#define FAT_TYPE_FAT12		12
#define FAT_TYPE_FAT16		16
#define FAT_TYPE_FAT32		32

	uint32_t	FirstDataSect;				/* 数据区第一个逻辑扇区 */
	uint32_t	FirstRootDirSect;			/* 根目录第一个逻辑扇区 */
	size_t	RootDirSects;				/* 根目录占用的扇区数 */
	uint32_t	RootDirClus;				/* 根目录的起始簇号 */
	size_t	SectsPerClus;				/* 每个簇的扇区数 */
	size_t	BytsPerSect;				/* 每个扇区的字节数 */
	size_t	RsvdSectCnt;				/* 预留的逻辑扇区数 */
};

struct FILE {
	int   valid; 						/* 1 valid, 0 free. */
	
	int   FileFlag;						/* 文件标记，标记只读，只写或者可读写 */
	uint32_t DirSect;						/* 文件父目录的扇区号 */
	int   DirIndex;						/* 文件在父目录中的偏移量 */
	uint32_t FirstSect;					/* 文件的第一个扇区 */
	uint32_t CurrentSect;					/* 文件的当前扇区 */
	uint32_t SectOffset;					/* 文件在当前扇区的偏移量 */
	uint32_t Offset;						/* 文件的总偏移量 */
	
	struct DIRENTRY Dir;				/* 文件的文件项 */
};

static struct FAT_STRUCT g_fat_struct;

#define FAT_MAX_FILDES		16
static struct FILE g_handles[FAT_MAX_FILDES];

#define FirstSectOfClus(N) ((((N) - 2) * g_fat_struct.SectsPerClus) + g_fat_struct.FirstDataSect)
#define SectNum2ClusNum(N) (((N) - g_fat_struct.FirstDataSect) / g_fat_struct.SectsPerClus + 2)

static int do_read_sector(size_t lba, char *buf)
{
  uint8_t *p = floppy_read_sector (lba);
  memcpy(buf, p, BYTES_PER_SECTOR);
  return 0;
}

/*
================================================================================
  内部静态函数，将文件名格式化为[8.3]格式文件名
参  数：pathname		文件路径名
返回值：NULL			无效的文件路径名
		char*			[8.3]格式文件名
================================================================================
*/
static
char* get_format83(const char *pathname)
{
	static char format_name[360];
	char pchar, *pname;
	int dir_len_count = 0, i;
	
	if(pathname == NULL || strlen(pathname) > 80 || *pathname != '\\') {
		return NULL;
	}
		
	if(strlen(pathname) > 1 && pathname[strlen(pathname) - 1] == '\\') {
		return NULL;
	}
		
	memset(format_name, 0x0, 360);
	pname = format_name;
	
	while(1) {
		switch(pchar = toupper(*pathname++)) {
			case 0x00:
				for(i = dir_len_count; i < 11; i++)
					pname[i] = 0x20;
				return format_name;
			case '.':
				if(dir_len_count > 8) {
					return NULL;
				}
				for(i = dir_len_count; i < 8; i++)
					pname[i] = 0x20;
				dir_len_count = 8;
				break;
			case '\\':
				for(i = dir_len_count; i < 11; i++)
					pname[i] = 0x20;
				pname[11] = '\\';
				pname += 12;				
				dir_len_count = 0;
				break;
			case 0x22: case 0x2a: case 0x2b: case 0x2f:
			case 0x3a: case 0x3b: case 0x3c: case 0x3d:
			case 0x3e: case 0x3f: case 0x5b: case 0x5d:
			case 0x7c:
				return NULL;
				
			default:
				if(pchar <= 0x20 && dir_len_count == 0) {
					return NULL;
				}
				
				pname[dir_len_count++] = pchar;
				
				if(dir_len_count > 11) {
					return NULL;
				}
		}
	}
}

static
uint32_t get_next_clus(uint32_t Cluster)
{
	size_t RsvdSecCnt, BytesPerSector;
  uint8_t FATType;
  char tmpbuf[BYTES_PER_SECTOR + BYTES_PER_SECTOR];
  size_t FATOffset, ThisFATSecNum, ThisFATEntOffset;

	RsvdSecCnt     = g_fat_struct.RsvdSectCnt;
	BytesPerSector = g_fat_struct.BytsPerSect;
	FATType        = g_fat_struct.FATType;

  if(FATType == FAT_TYPE_FAT12)
    FATOffset = Cluster + (Cluster >> 1);
  else if (FATType == FAT_TYPE_FAT16)
    FATOffset = Cluster << 1;
  else/* if (FATType == FAT_TYPE_FAT32)*/
    FATOffset = Cluster << 2;

  ThisFATSecNum = RsvdSecCnt + (FATOffset / BytesPerSector);
  ThisFATEntOffset = FATOffset % BytesPerSector;

  do_read_sector(ThisFATSecNum, tmpbuf);

  if(FATType == FAT_TYPE_FAT12) {
    uint32_t N = Cluster;
    if(ThisFATEntOffset == (BytesPerSector - 1)) {
            /* This cluster access spans a sector boundary in the FAT      */
            /* There are a number of strategies to handling this. The      */
            /* easiest is to always load FAT sectors into memory           */
            /* in pairs if the volume is FAT12 (if you want to load        */
            /* FAT sector N, you also load FAT sector N+1 immediately      */
            /* following it in memory unless sector N is the last FAT      */
            /* sector). It is assumed that this is the strategy used here  */
            /* which makes this if test for a sector boundary span         */
            /* unnecessary.                                                */
      do_read_sector(ThisFATSecNum+1, tmpbuf+BytesPerSector);
    }

    Cluster = *((uint16_t *) &tmpbuf[ThisFATEntOffset]);
    if(N & 0x0001)
      Cluster = Cluster >> 4;	/* Cluster number is ODD */
    else
      Cluster = Cluster & 0x0FFF;	/* Cluster number is EVEN */
  } else if(FATType == FAT_TYPE_FAT16)
    Cluster = *((uint16_t *) &tmpbuf[ThisFATEntOffset]);
  else/* if (FATType == FAT_TYPE_FAT32)*/
    Cluster = (*((uint32_t *) &tmpbuf[ThisFATEntOffset])) & 0x0FFFFFFF;
        
  return Cluster;
}

/*
================================================================================
功  能：在指定的扇区内寻找与filename名称相同的项目
参  数：lba			    起始扇区号
		    filename		要查找的文件名
		    fp			    输出参数，用以返回文件的描述信息
返回值：文件的第一个扇区号
				-1 没有找到指定的文件
================================================================================
*/
static
uint32_t sector_search(uint32_t lba, const char filename[11], struct FILE *fp)
{
	int i, dir_count;
	char buf[BYTES_PER_SECTOR];
	struct DIRENTRY* dir;
	uint32_t start_cluster;
	
	do_read_sector(lba, buf);
	
	dir_count = (g_fat_struct.BytsPerSect) / sizeof(struct DIRENTRY);
	for(i = 0, dir = (struct DIRENTRY*)buf; i < dir_count; i++, dir++) {
		if ((strncmp((dir->DIR_Name), filename, 11) == 0) /*&& (!(dir->DIR_Attr & ATTR_VOLUME_ID))*/) {
			if (fp != NULL) {
				fp->DirSect = lba;
				fp->DirIndex = i;
				memcpy(&(fp->Dir), (void*)dir, sizeof(struct DIRENTRY));
			}
			
			if(g_fat_struct.FATType != FAT_TYPE_FAT32)
				start_cluster = dir->DIR_FstClusLO;
			else
				start_cluster = (dir->DIR_FstClusHI << 16) + dir->DIR_FstClusLO;

			return FirstSectOfClus(start_cluster);
		}
	}
	
	return -1;
}

static
uint32_t fat_search(uint32_t lba, const char filename[11], struct FILE *fp)
{
	int i;
	uint32_t first_sector;
	
	if((g_fat_struct.FATType != FAT_TYPE_FAT32) && (lba == g_fat_struct.FirstRootDirSect)) {
		for(i = 0; i < (g_fat_struct.RootDirSects); i++) {
			first_sector = sector_search(lba++, filename, fp);
			if (first_sector != -1)
				return first_sector;
		}
	} else {
		uint32_t clus_num = SectNum2ClusNum(lba);
		
		while (1) {	
			for (i = 0; i < g_fat_struct.SectsPerClus; i++) {
				first_sector = sector_search(lba++, filename, fp);
				if(first_sector != -1)
					return first_sector;
			}
			
			clus_num = get_next_clus(clus_num);
			if ( ((g_fat_struct.FATType == FAT_TYPE_FAT12) && (clus_num >= FAT12_EOC)) ||
				 ((g_fat_struct.FATType == FAT_TYPE_FAT16) && (clus_num >= FAT16_EOC)) ||
				 ((g_fat_struct.FATType == FAT_TYPE_FAT32) && (clus_num >= FAT32_EOC)) )
				break;
			else
				lba = FirstSectOfClus(clus_num);
		}
	}
	
	return -1;
}

static
uint32_t fat_locate(const char *pathname_f83, struct FILE *fp)
{
	int i;
	uint32_t sector = g_fat_struct.FirstRootDirSect;
	
	pathname_f83 += 12;
	
	for(i = 0; ; i++) {
		if (*pathname_f83 == 0x0 || *pathname_f83 == 0x20) {
#if 0  /*what does this mean?*/
			if(i == 0) {	
				if(fp != NULL) {
					if(g_fat_struct.FATType != FAT_TYPE_FAT32) {
						fp->Dir.DIR_FstClusHI = 0;
						fp->Dir.DIR_FstClusLO = 2;					
					} else {
						fp->Dir.DIR_FstClusHI = (g_fat_struct.RootDirClus >> 16) & 0xffff;
						fp->Dir.DIR_FstClusLO = (g_fat_struct.RootDirClus) & 0xffff;
					}
					fp->FirstSect = FirstSectOfClus(fp->Dir.DIR_FstClusLO);
				}
			}
			
#endif
			return sector;
		}
			
		sector = fat_search(sector, pathname_f83, fp);
		if (sector == -1)
			return -1;
			
		pathname_f83 += 12;
	}
}

void init_fat()
{
	uint32_t FATSz;
	uint32_t TotSec, CountofClusters;
	struct BPB *pbpb;
	char buf[BYTES_PER_SECTOR];
	
	do_read_sector(0, buf);
	
	pbpb = (struct BPB*)&buf[BS_BPB_OFFSET];
	g_fat_struct.SectsPerClus = pbpb->BPB_SecPerClus;
	g_fat_struct.BytsPerSect  = pbpb->BPB_BytsPerSec;
	g_fat_struct.RsvdSectCnt  = pbpb->BPB_RsvdSecCnt;
	
	if(pbpb->BPB_FATSz16 != 0)
		FATSz = pbpb->BPB_FATSz16;
	else {
		struct BPB_FAT32 *bpb32 = (struct BPB_FAT32*)&buf[BS_EXT_OFFSET];
		FATSz = bpb32->BPB_FATSz32;
	}
	
	g_fat_struct.FirstRootDirSect = pbpb->BPB_RsvdSecCnt + pbpb->BPB_NumFATs * FATSz;
	g_fat_struct.RootDirSects     = ((pbpb->BPB_RootEntCnt * sizeof(struct DIRENTRY)) + (pbpb->BPB_BytsPerSec - 1)) / pbpb->BPB_BytsPerSec;
	g_fat_struct.FirstDataSect    = g_fat_struct.FirstRootDirSect + g_fat_struct.RootDirSects;

  if(pbpb->BPB_TotSec16 != 0)
    TotSec = pbpb->BPB_TotSec16;
  else
    TotSec = pbpb->BPB_TotSec32;
    	
  CountofClusters = (TotSec - g_fat_struct.FirstDataSect) / g_fat_struct.SectsPerClus;
	if(CountofClusters < 4085) {			/* FAT12 */
		g_fat_struct.FATType = FAT_TYPE_FAT12;	
	} else if(CountofClusters < 65525) {	/* FAT16 */
		g_fat_struct.FATType = FAT_TYPE_FAT16;
	} else {								/* FAT32 */
		struct BPB_FAT32 *bpb32 = (struct BPB_FAT32*)&buf[BS_EXT_OFFSET];

		g_fat_struct.FATType = FAT_TYPE_FAT32;		
		g_fat_struct.RootDirClus = bpb32->BPB_RootClus;
		g_fat_struct.FirstRootDirSect = FirstSectOfClus(g_fat_struct.RootDirClus);
	}
}

int fat_fopen(const char *pathname, int flag)
{
	char *format83_pathname;
	int i;
	uint32_t first_sector;
	struct FILE *fp = NULL;
	
	for(i = 0; i < FAT_MAX_FILDES; i++) {
		if(!g_handles[i].valid) {
			fp = &g_handles[i];
			break;
		}
	}	
	if(fp == NULL)
		return -1;
	
	format83_pathname = get_format83(pathname);
	if(format83_pathname == NULL)
		return -2;
	
	first_sector = fat_locate(format83_pathname, fp);
	if(first_sector == -1)
		return -3;
	
	fp->valid = 1;
	fp->FirstSect = fp->CurrentSect = first_sector;
	fp->SectOffset = fp->Offset = 0;
	fp->FileFlag = flag;
	
	return i;
}

int fat_fgetsize(int fd)
{
	if(fd < 0 || fd >= FAT_MAX_FILDES)
		return -1;
		
	return g_handles[fd].Dir.DIR_FileSize;
}

int fat_fclose(int fd)
{
	if(fd < 0 || fd >= FAT_MAX_FILDES)
		return -1;

	if(g_handles[fd].valid == 0)
    return -1;

	g_handles[fd].valid = 0;
	return 0;
}

int fat_fread(int fd, void *buf, size_t count)
{
	int read_bytes = 0;
	int i;
	int read_bytes_per_sector;
	uint32_t cluster;
	struct FILE *fp;
	char tmpbuf[BYTES_PER_SECTOR];
	
	if(fd < 0 || fd >= FAT_MAX_FILDES)
		return -2;
	if(buf == NULL)
		return -3;
  if(count == 0)
    return 0;
		
	fp = &g_handles[fd];
	cluster = SectNum2ClusNum(fp->CurrentSect);
	
	if(!(fp->FileFlag & O_RDONLY) && !(fp->FileFlag & O_RDWR))
		return -4;
	
	if(fp->Offset >= fp->Dir.DIR_FileSize) {
		return EOF;
	}

	if(count > (fp->Dir.DIR_FileSize - fp->Offset))
		count = fp->Dir.DIR_FileSize - fp->Offset;
	
	while(1) {
		for(i = 0; i < g_fat_struct.SectsPerClus; i++) {
			do_read_sector(fp->CurrentSect, tmpbuf);
			
			read_bytes_per_sector = (g_fat_struct.BytsPerSect - fp->SectOffset) > (count - read_bytes)?
				(count - read_bytes): (g_fat_struct.BytsPerSect - fp->SectOffset);
			memcpy(&(((char*)buf)[read_bytes]), &tmpbuf[fp->SectOffset], read_bytes_per_sector);
			read_bytes += read_bytes_per_sector;
			
			if(read_bytes >= count) {
				fp->SectOffset += read_bytes_per_sector;
				fp->Offset += read_bytes;
				return read_bytes;
			}
			
			fp->CurrentSect++;
			fp->SectOffset = 0;
		}
		
		cluster = get_next_clus(cluster);
		if( ((g_fat_struct.FATType == FAT_TYPE_FAT12) && (cluster >= FAT12_EOC)) ||
			((g_fat_struct.FATType == FAT_TYPE_FAT16) && (cluster >= FAT16_EOC)) ||
			((g_fat_struct.FATType == FAT_TYPE_FAT32) && (cluster >= FAT32_EOC)) )
			break;
		
		fp->CurrentSect = FirstSectOfClus(cluster);
		fp->SectOffset = 0;
	}
	
	fp->Offset += read_bytes;	
	return read_bytes;	
}

