include ../Makefile.inc

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
OBJS+=	../lib/div.o ../lib/string.o \
		../lib/snprintf.o ../lib/tlsf/tlsf.o

$(PROG).bin: $(OBJS)
	$(CC) $(LDFLAGS) -o $(PROG).out $^ $(LIBS)
	$(OBJCOPY) -S -O binary $(PROG).out $@

.PHONY: clean
clean:
	-$(RM) $(OBJS) $(PROG).bin $(PROG).out $(PROG).map *.*~
