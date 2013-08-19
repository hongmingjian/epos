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

CFLAGS=		-O -fomit-frame-pointer -fno-builtin \
					-ffreestanding -mno-stack-arg-probe \
					-mno-ms-bitfields \
					-fno-stack-check -fno-stack-protector #-Wall

LDFLAGS=	-Tldscript -nostdlib -nostartfiles -Wl,-Map,$(PROG).map

OBJS=		entry.o machdep.o printk.o vsprintf.o \
				utils.o task.o timer.o sem.o callout.o \
				kmalloc.o startup.o floppy.o fat.o pe.o \
				tlsf/tlsf.o

$(PROG).bin: $(OBJS)
	$(CC) $(LDFLAGS) -o $(PROG).out $(OBJS) $(LIBS)
	$(OBJCOPY) -S -O binary $(PROG).out $@
	../bin/fat_imgen.exe -m -g -i $@ -f floppy.img

debug:
	-../Bochs-2.6.2/bochsdbg.exe -q -f bochsrc.txt

run:
	-../Qemu-1.5.1/qemu-system-i386w.exe -L ../Qemu-1.5.1/Bios -m 32 -boot a -fda floppy.img
#	-../Bochs-2.6.2/bochs.exe -q -f bochsrc.txt

clean:
	-$(RM)  *.o tlsf/*.o *.bin *.*~ $(PROG).out $(PROG).map

