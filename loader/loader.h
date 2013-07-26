#ifndef _LOADER_H_
#define	_LOADER_H_

typedef unsigned char   uint8_t;
typedef          char    int8_t;
typedef unsigned short  uint16_t;
typedef          short   int16_t;
typedef unsigned int    uint32_t;
typedef          int     int32_t;
typedef unsigned int    size_t;
typedef          int    ssize_t;

#ifndef __offsetof
#define __offsetof(type, field) ((size_t)(&((type *)0)->field))
#endif

#ifndef NULL
#define	NULL		((void *) 0)
#endif

#define	sti()		__asm__ __volatile__ ("sti")
#define	cli()		__asm__ __volatile__ ("cli")

#define		IRQ_TIMER	      0
#define		IRQ_KEYBOARD    1
#define		IRQ_FLOPPY      6
#define   NR_IRQ          16
extern void (*intr_vector[NR_IRQ])(int);

/* ------------------------------------------------------------ */
/* Support for functions implemented in crt0.S                  */
/* ------------------------------------------------------------ */
void		 outportb(uint16_t, uint8_t) ;
uint8_t	 inportb(uint16_t) ;
void		 outportw(uint16_t, uint16_t) ;
uint16_t inportw(uint16_t) ;
void		 exit(int) ;

/* ------------------------------------------------------------ */
/* Support for functions implemented in putchar.c               */
/* ------------------------------------------------------------ */
int putchar(int);

/* ------------------------------------------------------------ */
/* Support for functions implemented in delay.c                 */
/* ------------------------------------------------------------ */
unsigned long init_delay(void);
void delay(unsigned long milliseconds);

/* ------------------------------------------------------------ */
/* Support for functions implemented in dma.c                   */
/* ------------------------------------------------------------ */
void dma_xfer(uint8_t channel, uint32_t address, size_t length, int read);

/* ------------------------------------------------------------ */
/* Support for functions implemented in floppy.c                */
/* ------------------------------------------------------------ */
void init_floppy();
uint8_t *floppy_read_sector (uint32_t lba);

/* ------------------------------------------------------------ */
/* Support for functions implemented in fat.c                   */
/* ------------------------------------------------------------ */
void init_fat();
int  fat_fopen(const char *pathname, int flag);
#define O_RDONLY		1
#define O_WRONLY		2
#define O_RDWR			(O_RDONLY | O_WRONLY)

int  fat_fclose(int fd);
int  fat_fgetsize(int fd);

#define EOF (-1)
int  fat_fread(int fd, void *buf, size_t count);

/* ------------------------------------------------------------ */
/* Support for functions implemented in loader.c                */
/* ------------------------------------------------------------ */
void enable_irq(uint32_t irq);
void disable_irq(uint32_t irq);

#endif
