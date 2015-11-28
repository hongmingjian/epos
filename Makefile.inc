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
