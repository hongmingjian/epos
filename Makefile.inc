CROSS=

ifeq ($(shell uname -s),Darwin)
CROSS=x86_64-elf-
endif

SHELL=		/bin/bash
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
GDB=		$(CROSS)gdb
OBJDUMP= $(CROSS)objdump