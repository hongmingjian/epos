ifeq ($(OS),Windows_NT)
CROSS=
else
ifeq ($(shell uname -s),Linux)
# For Ubuntu
#CROSS=i586-mingw32msvc-

# For ArchLinux
#CROSS=i486-mingw32-
endif
ifeq ($(shell uname -s),Darwin)
CROSS=i586-mingw32-
endif
endif

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

.PHONY: all
all: $(PROG).bin

ASFLAGS=-nostdinc -I../include
CFLAGS=	-DUSE_FLOPPY=0 -DVERBOSE=0 -nostdinc -I../include \
	-O -fomit-frame-pointer -fno-builtin \
	-ffreestanding -mno-stack-arg-probe \
	-mno-ms-bitfields -fleading-underscore \
	-fno-stack-check #-fno-stack-protector #-Wall

LDFLAGS=-Tldscript -nostdlib -nostartfiles -Wl,-Map,$(PROG).map

OBJS=	entry.o ide.o floppy.o e1000.o pci.o vm86.o \
	kbd.o timer.o machdep.o task.o mktime.o sem.o \
	page.o startup.o frame.o kmalloc.o dosfs.o pe.o \
	printk.o bitmap.o
OBJS+=	../lib/stdlib.o ../lib/div.o \
		../lib/string.o ../lib/snprintf.o \
		../lib/tlsf/tlsf.o

$(PROG).bin: $(OBJS)
	$(CC) $(LDFLAGS) -o $(PROG).out $^ $(LIBS)
	$(OBJCOPY) -S -O binary $(PROG).out $@

.PHONY: clean
clean:
	-$(RM) $(OBJS) $(PROG).bin $(PROG).out $(PROG).map *.*~
