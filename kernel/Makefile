include ../Makefile.inc

PROG=		kernel

.PHONY: all
all: release

CPPFLAGS=-DVERBOSE=0 -nostdinc -I../include
ASFLAGS=-Wall -D__ASSEMBLY__
CFLAGS=	-Wall -pipe \
	-ffreestanding -fleading-underscore
LDFLAGS+=-Tkernel.ld -nostdlib \
		-Wl,-Map,$(PROG).map

COBJS=timer.o machdep.o task.o \
	startup.o kmalloc.o page.o frame.o bitmap.o \
	printk.o sdcard.o dosfs.o elf.o
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

$(PROG).elf: kernel.ld $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $@

kernel.ld: kernel.ld.in
	$(CC) $(CPPFLAGS) -E -P -x c-header -o $@ $^

.PHONY: release
release: CFLAGS+=-O2 -ggdb
release: make.dep $(PROG).img

.PHONY: debug
debug: CFLAGS+=-O0 -ggdb
debug: make.dep $(PROG).img

.PHONY: qemu
qemu: CPPFLAGS+=-DRPI_QEMU
qemu: release

.PHONY: qemudbg
qemudbg: CPPFLAGS+=-DRPI_QEMU
qemudbg: debug

.PHONY: clean
clean:
	-$(RM) kernel.ld $(OBJS) $(PROG).img $(PROG).elf $(PROG).map make.dep *.*~

-include make.dep
