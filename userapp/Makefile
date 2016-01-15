include ../Makefile.inc

PROG=	a

.PHONY: all
all: $(PROG).out

ASFLAGS=-m32 -nostdinc -I../include -Iinclude
CFLAGS=	-m32 -nostdinc -I../include -Iinclude -fno-builtin \
	-ffreestanding -fno-stack-check \
	-fleading-underscore -mno-ms-bitfields \
	-mno-stack-arg-probe -O #-fno-stack-protector #-Wall
LDFLAGS=-m32 -nostdlib -nostartfiles -Wl,-Map,$(PROG).map

OBJS=	lib/crt0.o
OBJS+=	vm86call.o graphics.o main.o
OBJS+=	lib/sysconf.o lib/math.o lib/stdio.o lib/stdlib.o \
		lib/syscall-wrapper.o
OBJS+=	../lib/div.o ../lib/string.o ../lib/memcpy.o \
		../lib/memset.o ../lib/snprintf.o ../lib/tlsf/tlsf.o

$(PROG).out: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

.PHONY: clean
clean:
	-$(RM) $(OBJS) $(PROG).bin $(PROG).out $(PROG).map *.*~
