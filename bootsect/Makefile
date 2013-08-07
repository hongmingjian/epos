
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


PROG=		bootsect

CFLAGS=		-O
LDFLAGS=	-Ttext 0 -nostdlib

OBJS=		$(PROG).o
LIBS=		

all: $(PROG).bin

$(PROG).bin: $(OBJS)
	$(CC) $(LDFLAGS) -o $(PROG).out $^ $(LIBS)
	$(OBJCOPY) -S -O binary $(PROG).out $(PROG)
	dd if=$(PROG) of=$@ bs=512 count=1

clean:
	${RM} *.o $(PROG).out $(PROG) $(PROG).bin *.*~

