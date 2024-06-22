include ../Makefile.inc

PROG=	a

.PHONY: all
all: run

CPPFLAGS=-nostdinc -I../include -Iinclude
ASFLAGS=-m32 -march=i386 -Wall -D__ASSEMBLY__
CFLAGS=	-m32 -march=i386 -Wall -pipe\
	-DSNPRINTF_FLOATPOINT \
	-ffreestanding -fleading-underscore -mno-ms-bitfields \
	-fno-pie -fno-stack-check -mno-stack-arg-probe -fno-stack-protector
LDFLAGS=-m32 -nostdlib -nostartfiles -nodefaultlibs \
		-Wl,-Map,$(PROG).map -static
ifneq ($(OS),Windows_NT)
LDFLAGS+=-Wl,-melf_i386
endif

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
	$(CC) $(CPPFLAGS) -M $(COBJS:.o=.c) lib/syscall-wrapper.S >make.dep

.DELETE_ON_ERROR: make.dep
.FORCE:
.PHONY: .FORCE

../lib/tlsf/tlsf.o: .FORCE
../lib/memcpy.o: .FORCE
../lib/memset.o: .FORCE
../lib/snprintf.o: .FORCE
../lib/softfloat.o: .FORCE
../lib/string.o: .FORCE

.PHONY: debug
debug: CFLAGS+=-O0 -ggdb
debug: make.dep $(PROG).out

.PHONY: run
run: CFLAGS+=-O2 -ggdb
run: make.dep $(PROG).out

.PHONY: clean
clean:
	-$(RM) $(OBJS) $(PROG).out $(PROG).map make.dep .~* *.*~ ._* .DS_Store *.stackdump

-include make.dep
