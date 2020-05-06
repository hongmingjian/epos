include ../Makefile.inc

PROG=		kernel

.PHONY: all
all: release

RPI_QEMU=0

CPPFLAGS=-nostdinc -Iinclude -I../include -DRPI_QEMU=$(RPI_QEMU)
ASFLAGS=-Wall -march=armv6zk -D__ASSEMBLY__
CFLAGS= -Wall -pipe \
	-ffreestanding -fleading-underscore
LDFLAGS+=-Tkernel.ld -nostdlib -Wl,-Map,$(PROG).map

COBJS=timer.o machdep.o task.o \
	startup.o kmalloc.o page.o frame.o bitmap.o \
	printk.o sdcard.o dosfs.o elf.o devfs.o file.o \
	virtdev.o mailbox.o sem.o
COBJS+= ../lib/string.o ../lib/memcpy.o \
		../lib/memset.o  ../lib/tlsf/tlsf.o ../lib/snprintf.o ../lib/softfloat.o
OBJS=entry.o $(COBJS)

.PHONY: .FORCE
.FORCE:

../lib/tlsf/tlsf.o: .FORCE
../lib/memcpy.o: .FORCE
../lib/memset.o: .FORCE
../lib/snprintf.o: .FORCE
../lib/softfloat.o: .FORCE
../lib/string.o: .FORCE

.DELETE_ON_ERROR: make.dep
make.dep: $(COBJS:.o=.c)
	$(CC) $(CPPFLAGS) -M $^ >make.dep

$(PROG).img: $(PROG).elf
	$(OBJCOPY) $^ -O binary $@

$(PROG).elf: $(PROG).ld $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $@

$(PROG).ld: $(PROG).ld.in
	$(CC) $(CPPFLAGS) -E -P -x c-header -o $@ $^

$(PROG).img.uue: $(PROG).img
	uuencode $^ $^ >$@

.PHONY: release
release: CFLAGS+=-O2 -ggdb
release: make.dep $(PROG).img.uue

.PHONY: debug
debug: CFLAGS+=-O0 -ggdb -ffunction-sections
debug: make.dep $(PROG).img.uue

.PHONY: qemu
qemu: RPI_QEMU=1
qemu: release

.PHONY: qemudbg
qemudbg: RPI_QEMU=1
qemudbg: debug

.PHONY: clean
clean:
	-$(RM) $(OBJS) $(PROG).{ld,img,elf,map,img.uue} make.dep *.*~

-include make.dep
