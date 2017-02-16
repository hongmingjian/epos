#ifdef USE_FLOPPY

#include "kernel.h"

/* Defines for accessing the upper and lower byte of an integer. */
#define LO_BYTE(x)           ((x) & 0xFF)
#define HI_BYTE(x)          (((x) & 0xFF00) >> 8)

/* Quick-access registers and ports for each DMA channel. */
static unsigned char MaskReg[8]   = { 0x0A, 0x0A, 0x0A, 0x0A, 0xD4, 0xD4, 0xD4, 0xD4 };
static unsigned char ModeReg[8]   = { 0x0B, 0x0B, 0x0B, 0x0B, 0xD6, 0xD6, 0xD6, 0xD6 };
static unsigned char ClearReg[8]  = { 0x0C, 0x0C, 0x0C, 0x0C, 0xD8, 0xD8, 0xD8, 0xD8 };

static unsigned char PagePort[8]  = { 0x87, 0x83, 0x81, 0x82, 0x8F, 0x8B, 0x89, 0x8A };
static unsigned char AddrPort[8]  = { 0x00, 0x02, 0x04, 0x06, 0xC0, 0xC4, 0xC8, 0xCC };
static unsigned char CountPort[8] = { 0x01, 0x03, 0x05, 0x07, 0xC2, 0xC6, 0xCA, 0xCE };

static void _dma_xfer(unsigned char DMA_channel, unsigned char page, unsigned int offset, unsigned int length, unsigned char mode)
{
    /* Set up the DMA channel so we can use it.  This tells the DMA */
    /* that we're going to be using this channel.  (It's masked) */
    outportb(MaskReg[DMA_channel], 0x04 | DMA_channel);

    /* Clear any data transfers that are currently executing. */
    //outportb(ClearReg[DMA_channel], 0x00);
    outportb (0xd8,0xff);	//reset master flip-flop

    /* Send the specified mode to the DMA. */
    outportb(ModeReg[DMA_channel], mode);

    /* Send the offset address.  The first byte is the low base offset, the */
    /* second byte is the high offset. */
    outportb(AddrPort[DMA_channel], LO_BYTE(offset));
    outportb(AddrPort[DMA_channel], HI_BYTE(offset));

    outportb (0xd8,0xff);	//reset master flip-flop

    /* Send the physical page that the data lies on. */
    outportb(PagePort[DMA_channel], page);

    /* Send the length of the data.  Again, low byte first. */
    outportb(CountPort[DMA_channel], LO_BYTE(length));
    outportb(CountPort[DMA_channel], HI_BYTE(length));

    /* Ok, we're done.  Enable the DMA channel (clear the mask). */
    outportb(MaskReg[DMA_channel], DMA_channel);
}

static void dma_xfer(uint8_t channel, uint32_t address, size_t length, int read)
{
	unsigned char page, mode;
	unsigned int offset;
	
	if(read)
		mode = 0x54 + channel;
	else
		mode = 0x58 + channel;
		
	page = address >> 16;
	offset = address & 0xFFFF;
	length--;
	
	_dma_xfer(channel, page, offset, length, mode);	
}	
	
#define FDC_DOR  (0x3f2)   /* Digital Output Register */
#define FDC_MSR  (0x3f4)   /* Main Status Register (input) */
#define FDC_DRS  (0x3f4)   /* Data Rate Select Register (output) */
#define FDC_DATA (0x3f5)   /* Data Register */
#define FDC_DIR  (0x3f7)   /* Digital Input Register (input) */
#define FDC_CCR  (0x3f7)   /* Configuration Control Register (output) */

enum {
	FDC_DOR_DRIVE0			=	0,	//00000000	= here for completeness sake
	FDC_DOR_DRIVE1			=	1,	//00000001
	FDC_DOR_DRIVE2			=	2,	//00000010
	FDC_DOR_DRIVE3			=	3,	//00000011
	FDC_DOR_RESET			=	4,	//00000100
	FDC_DOR_DMA		    	=	8,	//00001000
	FDC_DOR_DRIVE0_MOTOR		=	16,	//00010000
	FDC_DOR_DRIVE1_MOTOR		=	32,	//00100000
	FDC_DOR_DRIVE2_MOTOR		=	64,	//01000000
	FDC_DOR_DRIVE3_MOTOR		=	128	//10000000
};

enum {
	FDC_MSR_DRIVE1_POS_MODE	=	1,	//00000001
	FDC_MSR_DRIVE2_POS_MODE	=	2,	//00000010
	FDC_MSR_DRIVE3_POS_MODE	=	4,	//00000100
	FDC_MSR_DRIVE4_POS_MODE	=	8,	//00001000
	FDC_MSR_BUSY			=	16,	//00010000
	FDC_MSR_DMA			    =	32,	//00100000
	FDC_MSR_DATAIO			=	64, //01000000
	FDC_MSR_DATAREG		    =	128	//10000000
};

enum {
	FDC_CMD_SPECIFY		    =	3,      //00000011
	FDC_CMD_WRITE           =	5,      //00000101
	FDC_CMD_READ            =	6,      //00000110
	FDC_CMD_CALIBRATE	    =	7,      //00000111
	FDC_CMD_CHECK_INT	    =	8,      //00001000
	FDC_CMD_SEEK		    =	0xf,    //00001111
	FDC_CMD_EXT_SKIP	    =	0x20,	//00100000
	FDC_CMD_EXT_DENSITY	    =	0x40,	//01000000
	FDC_CMD_EXT_MULTITRACK	=	0x80	//10000000
};

enum {
	FDC_SECTOR_DTL_128	=	0,
	FDC_SECTOR_DTL_256	=	1,
	FDC_SECTOR_DTL_512	=	2,
	FDC_SECTOR_DTL_1024	=	4
};

enum {
	FLOPPY_GAP3_LENGTH_STD = 42,
	FLOPPY_GAP3_LENGTH_5_14= 32,
	FLOPPY_GAP3_LENGTH_3_5= 27
};

#define FLOPPY_BYTES_PER_SECTOR 512
#define FLOPPY_SECTORS_PER_TRACK 18
#define FLOPPY_HEADS 2
#define FLOPPY_TRACKS 80

static uint8_t *g_buf_dma=(uint8_t *)0x1000;

static volatile uint8_t _FloppyIRQ = 0;
static void fdc_wait_irq()
{
    while(_FloppyIRQ==0)
        ;

    _FloppyIRQ = 0;
}

static void fdc_sendbyte(uint8_t byte)
{
   volatile int msr;
   int tmo;
   
   for (tmo = 0;tmo < 128;tmo++) {
      msr = inportb(FDC_MSR);
      if ((msr & 0xc0) == 0x80) {
        outportb(FDC_DATA,byte);
        return;
      }
      inportb(0x80);   /* delay */
   }
}

static uint8_t fdc_getbyte()
{
   volatile int msr;
   int tmo;
   
   for (tmo = 0;tmo < 128;tmo++) {
      msr = inportb(FDC_MSR);
      if ((msr & 0xd0) == 0xd0) {
        return inportb(FDC_DATA);
      }
      inportb(0x80);   /* delay */
   }

   return -1;   /* read timeout */
}

static void fdc_check_int (uint8_t *st0, uint8_t *cyl)
{
  fdc_sendbyte (FDC_CMD_CHECK_INT);
  *st0 = fdc_getbyte ();
  *cyl = fdc_getbyte ();
//printk("st0=0x%02x\r\n", *st0);
//printk("cyl=0x%02x\r\n", *cyl);
}

static void fdc_motoron()
{
  outportb(FDC_DOR,FDC_DOR_RESET|FDC_DOR_DMA|FDC_DOR_DRIVE0_MOTOR); 
}

static void fdc_motoroff()
{
  outportb(FDC_DOR,FDC_DOR_RESET|FDC_DOR_DMA); 
}

static int calibrate()
{
  int i; 
	uint8_t st0, cyl;
 
	fdc_motoron();
 
	for (i = 0; i < 10; i++) {
		fdc_sendbyte ( FDC_CMD_CALIBRATE );
		fdc_sendbyte ( 0 );
		fdc_wait_irq ();
		fdc_check_int (&st0, &cyl);
 
		if (0 == cyl) {
			fdc_motoroff();
			return 0;
		}
	}
 
	fdc_motoroff();
	return -1;
}

static void reset(void)
{
    uint8_t st0, cyl;

    /* stop the motor and disable IRQ/DMA */
    outportb(FDC_DOR,0);

    /* re-enable interrupts */
    outportb(FDC_DOR,FDC_DOR_RESET|FDC_DOR_DMA); 

    fdc_wait_irq();
    fdc_check_int(&st0, &cyl);
   
    /* program data rate (500K/s) */
    outportb(FDC_DRS,0);

    /* specify drive timings (got these off the BIOS) */
    fdc_sendbyte(FDC_CMD_SPECIFY);
    fdc_sendbyte(0xdf);  /* SRT = 3ms, HUT = 240ms */
    fdc_sendbyte(0x02);  /* HLT = 16ms, ND = 0 */
   
    calibrate();
}

static void fdc_read_sector_imp (uint8_t head, uint8_t track, uint8_t sector)
{
  uint32_t flags;
  int i;
	uint8_t st0, cyl;

  save_flags_cli(flags);

  dma_xfer(2, (uint32_t)g_buf_dma, FLOPPY_BYTES_PER_SECTOR, 1); 
	fdc_sendbyte (
    FDC_CMD_READ | FDC_CMD_EXT_MULTITRACK |
    FDC_CMD_EXT_SKIP | FDC_CMD_EXT_DENSITY);
	fdc_sendbyte ( head << 2 | 0 );
	fdc_sendbyte ( track);
	fdc_sendbyte ( head);
	fdc_sendbyte ( sector);
	fdc_sendbyte ( FDC_SECTOR_DTL_512 );
	fdc_sendbyte ( FLOPPY_SECTORS_PER_TRACK );
	fdc_sendbyte ( FLOPPY_GAP3_LENGTH_3_5 );
	fdc_sendbyte ( 0xff );

  restore_flags(flags);
 
	fdc_wait_irq ();
 
	for (i=0; i<7; i++) {
    fdc_getbyte();
//		printk("[%d]=0x%02x\r\n", i, fdc_getbyte ());
  }
 
	fdc_check_int (&st0,&cyl);
}

static void fdc_write_sector_imp (uint8_t head, uint8_t track, uint8_t sector)
{
  uint32_t flags;
  int i;
	uint8_t st0, cyl;

  save_flags_cli(flags);

  dma_xfer(2, (uint32_t)g_buf_dma, FLOPPY_BYTES_PER_SECTOR, 0); 
	fdc_sendbyte (
    FDC_CMD_WRITE | FDC_CMD_EXT_MULTITRACK |
    FDC_CMD_EXT_DENSITY);
	fdc_sendbyte ( head << 2 | 0 );
	fdc_sendbyte ( track);
	fdc_sendbyte ( head);
	fdc_sendbyte ( sector);
	fdc_sendbyte ( FDC_SECTOR_DTL_512 );
	fdc_sendbyte ( FLOPPY_SECTORS_PER_TRACK );
	fdc_sendbyte ( FLOPPY_GAP3_LENGTH_3_5 );
	fdc_sendbyte ( 0xff );

  restore_flags(flags);
 
	fdc_wait_irq ();
 
	for (i=0; i<7; i++) {
    fdc_getbyte();
//    printk("[%d]=0x%02x\r\n", i, fdc_getbyte ());
  }
 
	fdc_check_int (&st0,&cyl);
}


static int fdc_seek ( uint8_t cyl )
{
  int i;
	uint8_t st0, cyl0;

	for (i = 0; i < 10; i++ ) {
		fdc_sendbyte (FDC_CMD_SEEK);
		fdc_sendbyte ((0) << 2 | 0);
		fdc_sendbyte (cyl);
 
		fdc_wait_irq ();
		fdc_check_int (&st0,&cyl0);
 
		if ( cyl0 == cyl) {
			return 0;
    }
	}
 
	return -1;
}

static void floppy_lba2chs(size_t lba, uint8_t *head, uint8_t *track, uint8_t *sector)
{
   *head = (lba % (FLOPPY_SECTORS_PER_TRACK * FLOPPY_HEADS)) / (FLOPPY_SECTORS_PER_TRACK);
   *track = lba / (FLOPPY_SECTORS_PER_TRACK * FLOPPY_HEADS);
   *sector = lba % FLOPPY_SECTORS_PER_TRACK + 1;
}

static void isr_floppy(uint32_t irq, struct context *ctx)
{
    _FloppyIRQ = 1;
}

void init_floppy()
{
  g_intr_vector[IRQ_FDC] = isr_floppy;
  enable_irq(IRQ_FDC);

  *vtopte((uint32_t)g_buf_dma)=((uint32_t)g_buf_dma)|L2E_V|L2E_W;
  invlpg((uint32_t)g_buf_dma);

  reset();
}

uint8_t *floppy_read_sector (size_t lba)
{
	uint8_t head, track, sector;

  if(lba >= FLOPPY_SECTORS_PER_TRACK*FLOPPY_HEADS*FLOPPY_TRACKS)
    return 0;

	floppy_lba2chs (lba, &head, &track, &sector);

//  printk("head=%d, track=%d, sector=%d\r\n", head, track, sector);
 
	fdc_motoron();

	if (fdc_seek (track) < 0) {
    printk("failed to seek\r\n");
		return 0;
  }
 
//  printk("R%d ", lba);
	fdc_read_sector_imp (head, track, sector);

	fdc_motoroff ();
 
	return g_buf_dma;
}

int floppy_write_sector (size_t lba, uint8_t *buffer)
{
	uint8_t head, track, sector;

  if(lba >= FLOPPY_SECTORS_PER_TRACK*FLOPPY_HEADS*FLOPPY_TRACKS)
    return -1;

	floppy_lba2chs (lba, &head, &track, &sector);

//  printk("head=%d, track=%d, sector=%d\r\n", head, track, sector);
 
	fdc_motoron();

	if (fdc_seek (track) < 0) {
    printk("failed to seek\r\n");
		return -2;
  }
 
//  printk("W%d ", lba);
  memcpy(g_buf_dma, buffer, FLOPPY_BYTES_PER_SECTOR); 
	fdc_write_sector_imp (head, track, sector);

	fdc_motoroff ();

  return 0;
}

#endif /*USE_FLOPPY*/
