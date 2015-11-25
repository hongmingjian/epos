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

PROG=	a

.PHONY: all
all: $(PROG).out

ASFLAGS=-nostdinc -I../include
CFLAGS=	 -nostdinc -I../include -fno-builtin \
	-ffreestanding -fno-stack-check \
	-fleading-underscore -mno-ms-bitfields \
	-mno-stack-arg-probe -O #-fno-stack-protector #-Wall
LDFLAGS=-nostdlib -nostartfiles -Wl,-Map,$(PROG).map

OBJS=	crt0.o syscall-wrapper.o vm86call.o \
		math.o graphics.o main.o
OBJS+=	../lib/stdlib.o ../lib/div.o \
		../lib/string.o ../lib/snprintf.o \
		../lib/tlsf/tlsf.o

$(PROG).out: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

.PHONY: clean
clean:
	-$(RM) $(OBJS) $(PROG).bin $(PROG).out $(PROG).map *.*~
