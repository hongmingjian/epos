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

PROG=	a

all: $(PROG).out

CFLAGS=	-fno-builtin -ffreestanding -fno-stack-check \
		-fleading-underscore \
		-fno-stack-protector -mno-stack-arg-probe -O #-Wall
LDFLAGS=	-nostdlib -nostartfiles -Wl,-Map,$(PROG).map

OBJS=		crt0.o syscall.o utils.o vsprintf.o rand.o main.o tlsf.o

$(PROG).out: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

../hd.img: a.out
	../../bin/imgcpy.exe $^ $@=C:\$^
#	sudo mount -o loop,offset=32256 -t vfat $@ /mnt
#	cp $^ /mnt
#	sudo umount /mnt

.PHONY: run
run: ../hd.img
	$(MAKE) -C .. $@

.PHONY: debug
debug: ../hd.img
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
