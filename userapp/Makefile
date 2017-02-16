include ../Makefile.inc

PROG=	a

.PHONY: all
all: make.dep run

CPPFLAGS=-nostdinc -I../include -Iinclude
ASFLAGS=-m32 -Wall -D__ASSEMBLY__
CFLAGS=	-m32 -Wall -DSNPRINTF_FLOATPOINT \
	-ffreestanding -fno-stack-check -fleading-underscore \
	-mno-ms-bitfields -mno-stack-arg-probe
LDFLAGS=-m32 -nostdlib -nostartfiles -nodefaultlibs \
		-Wl,-Map,$(PROG).map

COBJS=	vm86call.o graphics.o main.o
COBJS+=	lib/sysconf.o lib/math.o lib/stdio.o lib/stdlib.o \
		lib/qsort.o
COBJS+=	../lib/softfloat.o ../lib/string.o ../lib/memcpy.o \
		../lib/memset.o ../lib/snprintf.o
COBJS+=	lib/malloc.o ../lib/tlsf/tlsf.o

OBJS=	lib/crt0.o lib/setjmp.o lib/syscall-wrapper.o $(COBJS)

$(PROG).out: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

make.dep: *.c *.h
	$(CC) $(CPPFLAGS) -M $(COBJS:.o=.c) >make.dep

.DELETE_ON_ERROR: make.dep

.PHONY: debug
debug: CFLAGS+=-O0 -ggdb
debug: $(PROG).out

.PHONY: run
run: CFLAGS+=-O2 -ggdb
run: $(PROG).out

.PHONY: clean
clean:
	-$(RM) $(OBJS) $(PROG).out $(PROG).map make.dep *.*~

-include make.dep
