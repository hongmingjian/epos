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

PROG=	imgcpy

all: $(PROG).exe

CFLAGS=		-O

OBJS=		Cache.o FAT.o image.o imgcpy.o

$(PROG).exe: $(OBJS)
	$(CC) -o $@ $(OBJS)

clean:
	-$(RM)  *.o $(PROG).exe

