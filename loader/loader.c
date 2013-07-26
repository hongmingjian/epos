#include "loader.h"
#include "utils.h"

void enable_irq(uint32_t irq)
{
  if(irq < 8)
    outportb(0x21, inportb(0x21) & (~(1<<irq)));
  else if(irq < NR_IRQ) {
    irq -= 8;
    outportb(0xa1, inportb(0xa1) & (~(1<<irq)));
  }
}

void disable_irq(uint32_t irq)
{
  if(irq < 8)
    outportb(0x21, inportb(0x21) | (1<<irq));
  else if(irq < NR_IRQ) {
    irq -= 8;
    outportb(0xa1, inportb(0xa1) | (1<<irq));
  }
}

int
abort(int fn, int code)
{
  printk("abort %d with code %d\n\r", fn, code);

  return 0;
}

/*****************************************************************************/
/*http://sources.redhat.com/ml/crossgcc/2000-q1/msg00227.html*/
void xmain(void)
{
  int fd;
  char *filename="\\eposkrnl.bin";

  printk("Calibrating delay loop: %d\n\r", init_delay());

  init_floppy();
  init_fat();

  fd=fat_fopen(filename, O_RDONLY);
  if(fd >= 0) {
    int read, size;
    size = fat_fgetsize(fd);
    printk("%s: size=%d bytes\n\r", filename, size);    
    read = fat_fread(fd, (void *)0x100000, size);
    printk("%s: %d bytes read\n\r", filename, read);
    fat_fclose(fd);

    if(read == size) 
      ((void (*)(void))(0x100000))();
    else
      printk("Failed reading %s, %d bytes expected, got %d bytes\n\r", filename, size, read);
  } else
    printk("Failed openning %s!\n\r", filename);

  printk("System died\n\r");

  while(1)
    ;
}

/*
 * End of file
 */
