include ../Makefile.inc

PROG=	a

.PHONY: all
all: run

CPPFLAGS=-nostdinc -I../include -Iinclude
ASFLAGS=-m32 -Wall -D__ASSEMBLY__
CFLAGS=	-m32 -Wall \
	-fno-builtin -ffreestanding -fno-stack-check -fleading-underscore \
	-mno-ms-bitfields -mno-stack-arg-probe
LDFLAGS=-m32 -nostdlib -nostartfiles -nodefaultlibs \
		-Wl,-Map,$(PROG).map

OBJS=	lib/crt0.o
OBJS+=	vm86call.o graphics.o main.o
OBJS+=	lib/sysconf.o lib/math.o lib/stdio.o lib/stdlib.o \
		lib/qsort.o lib/setjmp.o lib/syscall-wrapper.o
OBJS+=	../lib/div.o ../lib/string.o ../lib/memcpy.o \
		../lib/memset.o ../lib/snprintf.o
OBJS+=	lib/malloc.o ../lib/tlsf/tlsf.o

$(PROG).out: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

.PHONY: debug
debug: CFLAGS+=-O0 -ggdb
debug: $(PROG).out

.PHONY: run
run: CFLAGS+=-O2 -ggdb
run: $(PROG).out

.PHONY: clean
clean:
	-$(RM) $(OBJS) $(PROG).bin $(PROG).out $(PROG).map *.*~
