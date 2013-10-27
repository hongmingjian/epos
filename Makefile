CROSS=i486-mingw32-

CC=		$(CROSS)gcc
AS=		$(CROSS)as
LD=		$(CROSS)ld
OBJCOPY=	$(CROSS)objcopy
AR=		$(CROSS)ar
STRIP=		$(CROSS)strip
SIZE=		$(CROSS)size
STRINGS=	$(CROSS)strings
READELF=	$(CROSS)readelf
RANLIB=		$(CROSS)ranlib
NM=		$(CROSS)nm

PROG=		eposkrnl

all: $(PROG).bin

CFLAGS=		-DUSE_FLOPPY=0 -DVERBOSE=0 \
          -O -fomit-frame-pointer -fno-builtin \
					-ffreestanding -mno-stack-arg-probe \
					-mno-ms-bitfields -fleading-underscore \
					-fno-stack-check -fno-stack-protector #-Wall

LDFLAGS=	-Tldscript -nostdlib -nostartfiles -Wl,-Map,$(PROG).map

OBJS=		entry.o machdep.o printk.o vsprintf.o \
				utils.o task.o keyboard.o timer.o  \
				kmalloc.o dosfs.o page.o startup.o ide.o floppy.o \
				pe.o tlsf/tlsf.o 

$(PROG).bin: $(OBJS)
	$(CC) $(LDFLAGS) -o $(PROG).out $(OBJS) $(LIBS)
	$(OBJCOPY) -S -O binary $(PROG).out $@

hd.img: $(PROG).bin
ifeq ($(OS),Windows_NT)
	../bin/imgcpy.exe $^ $@=C:\$^
else
ifeq ($(shell uname -s),Linux)
	sudo mount -o loop,offset=32256 -t vfat $@ /mnt
	-sudo cp $^ /mnt
	sudo umount /mnt
endif
#ifeq ($(shell uname -s),Darwin)
#endif
endif

.PHONY: debug
debug: hd.img
ifeq ($(OS),Windows_NT)
	-../Bochs/bochsdbg.exe -q -f bochsrc.txt
else
ifeq ($(shell uname -s),Linux)
endif
#ifeq ($(shell uname -s),Darwin)
#endif
endif

.PHONY: run
run: hd.img
ifeq ($(OS),Windows_NT)
	-../Qemu/qemu-system-i386w.exe -L ../Qemu/Bios -m 4 \
		-boot order=c -hda $^
else
ifeq ($(shell uname -s),Linux)
	-qemu-system-i386 -m 4 -boot order=c -hda $^
endif
#ifeq ($(shell uname -s),Darwin)
#endif
endif

.PHONY: bochs
bochs: hd.img
ifeq ($(OS),Windows_NT)
	-../Bochs/bochs.exe -q -f bochsrc.txt
else
ifeq ($(shell uname -s),Linux)
endif
#ifeq ($(shell uname -s),Darwin)
#endif
endif

.PHONY: clean
clean:
	-$(RM)  *.o tlsf/*.o *.bin *.*~ $(PROG).out #$(PROG).map
	-$(RM) *.aux *.log *.out *.nav *.snm *.toc *.vrb *.lol

epos.pdf: epos.tex
	pdflatex epos
	pdflatex epos
