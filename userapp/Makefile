include ../Makefile.inc

PROG=	a

.PHONY: all
all: make.dep $(PROG).out

CPPFLAGS=-nostdinc -I../include -Iinclude
ASFLAGS=-Wall -O2 -D__ASSEMBLY__
CFLAGS=	-Wall -O2 \
	-ffreestanding -fleading-underscore
LDFLAGS= -nostdlib \
		-Wl,-Map,$(PROG).map -Wl,-e,_start,-Ttext,0x08048000

COBJS=	main.o lib/stdio.o ../lib/snprintf.o ../lib/softfloat.o ../lib/string.o

COBJS=	main.o
COBJS+=	lib/stdio.o
COBJS+=	../lib/softfloat.o ../lib/string.o ../lib/memcpy.o \
		../lib/memset.o ../lib/snprintf.o

OBJS=	lib/crt0.o lib/syscall-wrapper.o $(COBJS)

.PHONY: .FORCE
.FORCE:

../lib/softfloat.o: .FORCE
../lib/string.o: .FORCE
../lib/memcpy.o: .FORCE
../lib/memset.o: .FORCE
../lib/snprintf.o: .FORCE

$(PROG).out: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

.DELETE_ON_ERROR: make.dep
make.dep: $(COBJS:.o=.c)
	$(CC) $(CPPFLAGS) -M $^ >make.dep

.PHONY: clean
clean:
	-$(RM) $(OBJS) $(PROG).out $(PROG).map make.dep *.*~

-include make.dep
