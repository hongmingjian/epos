include ../Makefile.inc

PROG=		eposkrnl

.PHONY: all
all: make.dep run

CPPFLAGS=-DVERBOSE=0 -nostdinc -I../include
ASFLAGS=-m32 -Wall -D__ASSEMBLY__
CFLAGS=	-m32 -Wall \
	-fomit-frame-pointer \
	-ffreestanding -fno-stack-check -fleading-underscore \
	-mno-ms-bitfields -mno-stack-arg-probe
LDFLAGS=-m32 -Tkernel.ld -nostdlib -nostartfiles -nodefaultlibs \
		-Wl,-Map,$(PROG).map

COBJS=	ide.o floppy.o e1000.o pci.o vm86.o \
	kbd.o timer.o machdep.o task.o mktime.o sem.o \
	page.o startup.o frame.o kmalloc.o dosfs.o pe.o \
	elf.o printk.o bitmap.o
COBJS+=	../lib/softfloat.o ../lib/string.o ../lib/memcpy.o \
		../lib/memset.o ../lib/snprintf.o ../lib/tlsf/tlsf.o

OBJS=	entry.o $(COBJS)

make.dep: *.c *.h
	$(CC) $(CPPFLAGS) -M $(COBJS:.o=.c) >make.dep

.DELETE_ON_ERROR: make.dep

$(PROG).bin: kernel.ld $(OBJS)
	$(CC) $(LDFLAGS) -o $(PROG).out $(OBJS) $(LIBS)
	$(OBJCOPY) -S -O binary $(PROG).out $@

kernel.ld: kernel.ld.in
	$(CC) -E -P -x c-header -o $@ $^

.PHONY: debug
debug: CFLAGS+=-O0 -ggdb
debug: $(PROG).bin

.PHONY: run
run: CFLAGS+=-O2 -ggdb
run: $(PROG).bin

.PHONY: clean
clean:
	-$(RM) kernel.ld $(OBJS) $(PROG).bin $(PROG).out $(PROG).map make.dep *.*~

-include make.dep
