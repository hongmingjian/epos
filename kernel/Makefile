include ../Makefile.inc

PROG=		eposkrnl

.PHONY: all
all: run

ASFLAGS=-nostdinc -I../include
CFLAGS=	$(ASFLAGS) -m32 -DUSE_FLOPPY=0 -DVERBOSE=0 \
	-fomit-frame-pointer \
	-fno-builtin -ffreestanding -fno-stack-check -fleading-underscore \
	-mno-ms-bitfields -mno-stack-arg-probe
LDFLAGS=-m32 -Tldscript -nostdlib -nostartfiles -nodefaultlibs \
		-Wl,-Map,$(PROG).map

OBJS=	entry.o ide.o floppy.o e1000.o pci.o vm86.o \
	kbd.o timer.o machdep.o task.o mktime.o sem.o \
	page.o startup.o frame.o kmalloc.o dosfs.o pe.o \
	elf.o printk.o bitmap.o
OBJS+=	../lib/div.o ../lib/string.o ../lib/memcpy.o \
		../lib/memset.o ../lib/snprintf.o ../lib/tlsf/tlsf.o

$(PROG).bin: ldscript $(OBJS)
	$(CC) $(LDFLAGS) -o $(PROG).out $(OBJS) $(LIBS)
	$(OBJCOPY) -S -O binary $(PROG).out $@

ldscript: ldscript.in
	$(CC) -E -P -x c-header -o $@ $^

.PHONY: debug
debug: CFLAGS+=-O0 -ggdb
debug: $(PROG).bin

.PHONY: run
run: CFLAGS+=-O2 -ggdb
run: $(PROG).bin

.PHONY: clean
clean:
	-$(RM) ldscript $(OBJS) $(PROG).bin $(PROG).out $(PROG).map *.*~
