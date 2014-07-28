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
CROSS=i386-mingw32-
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

all: $(PROG).out

CFLAGS=	-fno-builtin -ffreestanding -fno-stack-check \
	-fleading-underscore \
	-mno-stack-arg-probe -O #-fno-stack-protector #-Wall
LDFLAGS=-nostdlib -nostartfiles -Wl,-Map,$(PROG).map 

OBJS=	crt0.o syscall.o utils.o vsprintf.o rand.o main.o tlsf.o vm86.o video.o

$(PROG).out: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

../hd.img: a.out
ifeq ($(OS),Windows_NT)
	../../bin/imgcpy.exe $^ $@=C:\$^
else
ifeq ($(shell uname -s),Linux)
	sudo mount -o loop,offset=32256 -t vfat $@ /mnt
	sudo cp $^ /mnt
	sudo umount /mnt
endif
ifeq ($(shell uname -s),Darwin)
	hdiutil attach -imagekey diskimage-class=CRawDiskImage $@
	cp $^ /Volumes/EPOSDISK
	hdiutil detach /Volumes/EPOSDISK
endif
endif

.PHONY: run
run: ../hd.img
	$(MAKE) -C .. $@

.PHONY: debug
debug: ../hd.img
	$(MAKE) -C .. $@ 

.PHONY: qemu
qemu: ../hd.img
	$(MAKE) -C .. $@

.PHONY: bochs
bochs: ../hd.img
	$(MAKE) -C .. $@ 

vsprintf.o: ../vsprintf.c
	$(CC) -c $(CFLAGS) $^

utils.o: ../utils.c
	$(CC) -c $(CFLAGS) $^

tlsf.o: ../tlsf/tlsf.c
	$(CC) -c $(CFLAGS) $^

.PHONY: clean
clean:
	-$(RM)  *.o *.out *.bin *.*~ #$(PROG).map
