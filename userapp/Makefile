include ../Makefile.inc

PROG=	a

.PHONY: all
all: make.dep run

CPPFLAGS=-nostdinc -I../include -Iinclude
ASFLAGS=-Wall -O2 -D__ASSEMBLY__
CFLAGS=	-Wall -O2 \
	-fno-builtin -ffreestanding -fno-stack-check -fleading-underscore
LDFLAGS= -nostdlib -nostartfiles -nodefaultlibs \
		-Wl,-Map,$(PROG).map -Wl,-e,_start,-Ttext,0x08048000

COBJS=	main.o
OBJS=	lib/crt0.o lib/syscall-wrapper.o $(COBJS)

../lib/snprintf.o:   .FORCE
.PHONY: .FORCE
.FORCE:

$(PROG).out: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

make.dep: *.c
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
