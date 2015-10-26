ifeq ($(OS),Windows_NT)
CROSS=
else
ifeq ($(shell uname -s),Linux)
# For Ubuntu
#CROSS=i586-mingw32msvc-

# For ArchLinux
#CROSS=i486-mingw32-
endif
ifeq ($(shell uname -s),Darwin)
CROSS=i586-mingw32-
endif
endif

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

CFLAGS=	-DUSE_FLOPPY=0 -DVERBOSE=1 \
	-O -fomit-frame-pointer -fno-builtin \
	-ffreestanding -mno-stack-arg-probe \
	-mno-ms-bitfields -fleading-underscore \
	-fno-stack-check #-fno-stack-protector #-Wall

LDFLAGS=-Tldscript -nostdlib -nostartfiles -Wl,-Map,$(PROG).map

OBJS=	entry.o machdep.o printk.o vsprintf.o \
	task.o kbd.o timer.o mktime.o sem.o  \
	kmalloc.o dosfs.o page.o startup.o ide.o \
	floppy.o pci.o pe.o utils.o tlsf/tlsf.o e1000.o

$(PROG).bin: $(OBJS)
	$(CC) $(LDFLAGS) -o $(PROG).out $(OBJS) $(LIBS)
	$(OBJCOPY) -S -O binary $(PROG).out $@

hd.img: $(PROG).bin
ifeq ($(OS),Windows_NT)
	if [ ! -s $@ ]; then base64 -d $@.bz2.txt | bunzip2 >$@ ; fi
	imgcpy.exe $^ $@=C:\$^
else
ifeq ($(shell uname -s),Linux)
	if [ ! -s $@ ]; then base64 -d $@.bz2.txt | bunzip2 >$@ ; fi
	sudo mount -o loop,offset=32256 -t vfat $@ /mnt
	sudo cp $^ /mnt
	sudo umount /mnt
endif
ifeq ($(shell uname -s),Darwin)
	if [ ! -s $@ ]; then base64 -D $@.bz2.txt | bunzip2 >$@ ; fi
	hdiutil attach -imagekey diskimage-class=CRawDiskImage $@
	cp $^ /Volumes/EPOSDISK
	hdiutil detach /Volumes/EPOSDISK
endif
endif

.PHONY: run
run: qemu

.PHONY: debug
debug: bochsdbg

.PHONY: qemu
qemu: hd.img
ifeq ($(OS),Windows_NT)
	-qemu-system-i386w.exe -L $(QEMUHOME)/Bios -m 4 -boot order=c -vga std -soundhw pcspk -hda hd.img
else
	-qemu-system-i386 -m 4 -boot order=c -vga std -soundhw pcspk -hda hd.img
endif

.PHONY: bochs
bochs: hd.img
ifeq ($(OS),Windows_NT)
	-bochs.exe -q -f bochsrc-win.txt
else
	-bochs -q -f bochsrc-unix.txt
endif

.PHONY: bochsdbg
bochsdbg: hd.img
ifeq ($(OS),Windows_NT)
	-bochsdbg.exe -q -f bochsrc-win.txt
else
	-bochsdbg -q -f bochsrc-unix.txt
endif

.PHONY: clean
clean:
	-$(RM)  *.o tlsf/*.o *.bin *.*~ $(PROG).out $(PROG).map hd.img
	-$(RM) *.aux *.log *.out *.nav *.snm *.toc *.vrb *.lol

epos.pdf: epos.tex
	pdflatex epos
	pdflatex epos
