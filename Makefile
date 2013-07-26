CROSS=

CC=		$(CROSS)gcc
AS=		$(CROSS)as
LD=		$(CROSS)ld
OBJCOPY=	$(CROSS)objcopy
AR=		$(CROSS)ar
STRIP=		$(CROSS)strip
SIZE=		$(CROSS)size
STRINGS=	$(CROSS)strings
READELF=	$(CROSS)readelf
RANLIB=		$(CROSS)ranlib
NM=		$(CROSS)nm

PROG=		eposkrnl

all: $(PROG).bin

CFLAGS=		-fno-builtin -ffreestanding -O #-Wall
LDFLAGS=	-Tldscript -nostdlib -nostartfiles -Wl,-Map,$(PROG).map

OBJS=		entry.o machdep.o printk.o vsprintf.o utils.o task.o timer.o sem.o callout.o kmalloc.o startup.o tlsf/tlsf.o

$(PROG).bin: $(OBJS)
	$(CC) $(LDFLAGS) -o $(PROG).out $(OBJS) $(LIBS)
	$(OBJCOPY) -S -O binary $(PROG).out $@
	../bin/winimage.exe /h image.flp  /i $@

debug:
	-../Bochs-2.6.2/bochsdbg.exe -q -f bochsrc.txt

run:
	-../Bochs-2.6.2/bochs.exe -q -f bochsrc.txt

clean:
	-$(RM)  *.o *.out *.bin *.*~ $(PROG).exe $(PROG).map
