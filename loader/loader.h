#ifndef _LOADER_H_
#define	_LOADER_H_

#include "utils.h"

#define	sti()		__asm__ __volatile__ ("sti")
#define	cli()		__asm__ __volatile__ ("cli")

#define		IRQ_TIMER	0
#define		IRQ_KEYBOARD    1
#define		IRQ_FLOPPY  6
extern void (*intr_vector[])(int);

/* ------------------------------------------------------------ */
/* Support for functions implemented in crt0.S                  */
/* ------------------------------------------------------------ */
void		 outportb(uint16_t, uint8_t) ;
uint8_t	 inportb(uint16_t) ;
void		 outportw(uint16_t, uint16_t) ;
uint16_t inportw(uint16_t) ;
void		 exit(int) ;

/* ------------------------------------------------------------ */
/* Support for functions implemented in console.c               */
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
uint8_t* floppy_read_sector (uint32_t lba);

/* ------------------------------------------------------------ */
/* Support for functions implemented in fat.c                   */
/* ------------------------------------------------------------ */
void init_fat();
int  fat_open (const char *pathname, int flag);
#define O_RDONLY		1
#define O_WRONLY		2
#define O_RDWR			(O_RDONLY | O_WRONLY)

int  fat_close(int fd);
int  fat_getlen(int fd);
int  fat_read(int fd, void *buf, size_t count);

#endif
