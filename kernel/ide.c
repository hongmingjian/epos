#include <inttypes.h>
#include "kernel.h"

#define ATA_SR_BSY     0x80
#define ATA_SR_DRDY    0x40
#define ATA_SR_DF      0x20
#define ATA_SR_DSC     0x10
#define ATA_SR_DRQ     0x08
#define ATA_SR_CORR    0x04
#define ATA_SR_IDX     0x02
#define ATA_SR_ERR     0x01

#define ATA_ER_BBK      0x80
#define ATA_ER_UNC      0x40
#define ATA_ER_MC       0x20
#define ATA_ER_IDNF     0x10
#define ATA_ER_MCR      0x08
#define ATA_ER_ABRT     0x04
#define ATA_ER_TK0NF    0x02
#define ATA_ER_AMNF     0x01

#define ATA_CMD_READ_PIO          0x20
#define ATA_CMD_READ_PIO_EXT      0x24
#define ATA_CMD_READ_DMA          0xC8
#define ATA_CMD_READ_DMA_EXT      0x25
#define ATA_CMD_WRITE_PIO         0x30
#define ATA_CMD_WRITE_PIO_EXT     0x34
#define ATA_CMD_WRITE_DMA         0xCA
#define ATA_CMD_WRITE_DMA_EXT     0x35
#define ATA_CMD_CACHE_FLUSH       0xE7
#define ATA_CMD_CACHE_FLUSH_EXT   0xEA
#define ATA_CMD_PACKET            0xA0
#define ATA_CMD_IDENTIFY_PACKET   0xA1
#define ATA_CMD_IDENTIFY          0xEC

#define ATAPI_CMD_READ       0xA8
#define ATAPI_CMD_EJECT      0x1B

#define ATA_IDENT_DEVICETYPE   0
#define ATA_IDENT_CYLINDERS    2
#define ATA_IDENT_HEADS        6
#define ATA_IDENT_SECTORS      12
#define ATA_IDENT_SERIAL       20
#define ATA_IDENT_MODEL        54
#define ATA_IDENT_CAPABILITIES 98
#define ATA_IDENT_FIELDVALID   106
#define ATA_IDENT_MAX_LBA      120
#define ATA_IDENT_COMMANDSETS  164
#define ATA_IDENT_MAX_LBA_EXT  200

#define IDE_ATA        0x00
#define IDE_ATAPI      0x01

#define ATA_MASTER     0x00
#define ATA_SLAVE      0x01

#define ATA_REG_DATA       0x00
#define ATA_REG_ERROR      0x01
#define ATA_REG_FEATURES   0x01
#define ATA_REG_SECCOUNT0  0x02
#define ATA_REG_LBA0       0x03
#define ATA_REG_LBA1       0x04
#define ATA_REG_LBA2       0x05
#define ATA_REG_HDDEVSEL   0x06
#define ATA_REG_COMMAND    0x07
#define ATA_REG_STATUS     0x07
#define ATA_REG_SECCOUNT1  0x08
#define ATA_REG_LBA3       0x09
#define ATA_REG_LBA4       0x0A
#define ATA_REG_LBA5       0x0B
#define ATA_REG_CONTROL    0x0C
#define ATA_REG_ALTSTATUS  0x0C
#define ATA_REG_DEVADDRESS 0x0D


typedef struct {
	uint8_t  reserved;
	uint8_t  channel;
	uint8_t  drive;
	uint16_t type;
	uint16_t signature;
	uint16_t capabilities;
	uint32_t command_sets;
	uint32_t size;
	uint8_t  model[41];
} ide_device_t;

typedef struct {
	uint16_t flags;
	uint16_t unused1[9];
	char     serial[20];
	uint16_t unused2[3];
	char     firmware[8];
	char     model[40];
	uint16_t sectors_per_int;
	uint16_t unused3;
	uint16_t capabilities[2];
	uint16_t unused4[2];
	uint16_t valid_ext_data;
	uint16_t unused5[5];
	uint16_t size_of_rw_mult;
	uint32_t sectors_28;
	uint16_t unused6[38];
	uint64_t sectors_48;
	uint16_t unused7[152];
} __attribute__((packed)) ata_identify_t;


static void repinsw(unsigned short port, unsigned char * data, unsigned long size)
{
	__asm__ __volatile__ ("rep insw" : "+D" (data), "+c" (size) : "d" (port) : "memory");
}

static void repoutsw(unsigned short port, unsigned char * data, unsigned long size)
{
	__asm__ __volatile__ ("rep outsw" : "+S" (data), "+c" (size) : "d" (port));
}

static void delay400ns(uint16_t bus)
{
	inportb(bus + ATA_REG_ALTSTATUS);
	inportb(bus + ATA_REG_ALTSTATUS);
	inportb(bus + ATA_REG_ALTSTATUS);
	inportb(bus + ATA_REG_ALTSTATUS);
}

static int ata_wait(uint16_t bus, int advanced)
{
	uint8_t status = 0;

	delay400ns(bus);

	while ((status = inportb(bus + ATA_REG_STATUS)) & ATA_SR_BSY);

	if (advanced) {
		status = inportb(bus + ATA_REG_STATUS);
		if (status   & ATA_SR_ERR)  return 1;
		if (status   & ATA_SR_DF)   return 1;
		if (!(status & ATA_SR_DRQ)) return 1;
	}

	return 0;
}

static void ata_wait_ready(uint16_t bus)
{
	while (inportb(bus + ATA_REG_STATUS) & ATA_SR_BSY);
}

static void ata_select(uint16_t bus)
{
	outportb(bus + ATA_REG_HDDEVSEL, 0xA0);
}

void ide_init(uint16_t bus)
{
	int i;
  uint16_t *buf;
	uint8_t *ptr;

	outportb(bus + 1, 1);
	outportb(bus + 0x306, 0);

	ata_select(bus);
	delay400ns(bus);

	outportb(bus + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
	delay400ns(bus);

	inportb(bus + ATA_REG_COMMAND);

	ata_wait_ready(bus);

	ata_identify_t device;
	buf = (uint16_t *)&device;

	for (i = 0; i < 256; ++i)
		buf[i] = inportw(bus);

	ptr = (uint8_t *)&device.model;
	for (i = 0; i < 39; i+=2) {
		uint8_t tmp = ptr[i+1];
		ptr[i+1] = ptr[i];
		ptr[i] = tmp;
	}

	outportb(bus + ATA_REG_CONTROL, 0x02);
}

void ide_read_sector(uint16_t bus, uint8_t slave, uint32_t lba, uint8_t *buf)
{
	outportb(bus + ATA_REG_CONTROL, 0x02);

	ata_wait_ready(bus);

	outportb(bus + ATA_REG_HDDEVSEL,  0xe0 | slave << 4 |
								 (lba & 0x0f000000) >> 24);
	outportb(bus + ATA_REG_FEATURES, 0x00);
	outportb(bus + ATA_REG_SECCOUNT0, 1);
	outportb(bus + ATA_REG_LBA0, (lba & 0x000000ff) >>  0);
	outportb(bus + ATA_REG_LBA1, (lba & 0x0000ff00) >>  8);
	outportb(bus + ATA_REG_LBA2, (lba & 0x00ff0000) >> 16);
	outportb(bus + ATA_REG_COMMAND, ATA_CMD_READ_PIO);

	if (ata_wait(bus, 1)) {
		printk("Error ATA reading bus=0x%03x, %s, lba=%d", bus, slave?"slave":"master", lba);
	}

	repinsw(bus,buf,256);
	ata_wait(bus, 0);
}

void ide_write_sector(uint16_t bus, uint8_t slave, uint32_t lba, uint8_t *buf)
{
	outportb(bus + ATA_REG_CONTROL, 0x02);

	ata_wait_ready(bus);

	outportb(bus + ATA_REG_HDDEVSEL,  0xe0 | slave << 4 |
								 (lba & 0x0f000000) >> 24);
	ata_wait(bus, 0);
	outportb(bus + ATA_REG_FEATURES, 0x00);
	outportb(bus + ATA_REG_SECCOUNT0, 0x01);
	outportb(bus + ATA_REG_LBA0, (lba & 0x000000ff) >>  0);
	outportb(bus + ATA_REG_LBA1, (lba & 0x0000ff00) >>  8);
	outportb(bus + ATA_REG_LBA2, (lba & 0x00ff0000) >> 16);
	outportb(bus + ATA_REG_COMMAND, ATA_CMD_WRITE_PIO);
	ata_wait(bus, 0);

	repoutsw(bus,buf,256);
	outportb(bus + ATA_REG_COMMAND, ATA_CMD_CACHE_FLUSH);
	ata_wait(bus, 0);
}
