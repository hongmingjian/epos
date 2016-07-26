include ../Makefile.inc

PROG=		kernel

.PHONY: all
all: make.dep $(PROG).img

CPPFLAGS=-DVERBOSE=1 -nostdinc -I../include
ASFLAGS=-Wall -O2 -D__ASSEMBLY__
CFLAGS=	-Wall -O2  \
	-fomit-frame-pointer \
	-ffreestanding -fno-stack-check -fleading-underscore
LDFLAGS+=-Tldscript -nostdlib -nostartfiles -nodefaultlibs \
		-Wl,-Map,$(PROG).map

COBJS=timer.o machdep.o task.o \
	startup.o kmalloc.o page.o frame.o bitmap.o \
	printk.o 
COBJS+= ../lib/string.o ../lib/memcpy.o \
		../lib/memset.o  ../lib/tlsf/tlsf.o ../lib/snprintf.o ../lib/softfloat.o
OBJS=entry.o $(COBJS)

../lib/snprintf.o:   .FORCE
.PHONY: .FORCE
.FORCE:

.DELETE_ON_ERROR: make.dep
make.dep: $(COBJS:.o=.c)
	$(CC) $(CPPFLAGS) -M $^ >make.dep
	
$(PROG).img: $(PROG).elf
	$(OBJCOPY) $^ -O binary $@

$(PROG).elf: ldscript $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $@

ldscript: ldscript.in
	$(CC) -E -P -x c-header -o $@ $^
	
.PHONY: clean
clean:
	-$(RM) ldscript $(OBJS) $(PROG).img $(PROG).elf $(PROG).map make.dep *.*~

-include make.dep