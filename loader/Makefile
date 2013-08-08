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

PROG=		eposldr

all: $(PROG).bin

CFLAGS=		-fno-builtin -ffreestanding #-Wall
LDFLAGS=	-Tldscript -nostdlib -nostartfiles -Wl,-Map,$(PROG).map

OBJS=		crt0.o loader.o putchar.o vsprintf.o utils.o delay.o floppy.o dma.o fat.o
LIBS=

$(PROG).bin: $(OBJS)
	$(CC) $(LDFLAGS) -o $(PROG).out $(OBJS) $(LIBS)
	$(OBJCOPY) -S -O binary $(PROG).out $@
	../../bin/winimage /h ../image.flp  /i $@

run:
	$(MAKE) -C .. run

debug:
	$(MAKE) -C .. debug 

clean:
	-$(RM)  *.o *.out *.bin *.*~ $(PROG).exe $(PROG).map
