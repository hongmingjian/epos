CROSS=

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
	../bin/imgcpy.exe $@ hd.img=C:\$@
#	../bin/winimage.exe hd.img /q /h /i $@
#	../bin/fat_imgen.exe -m -g -i $@ -f floppy.img

debug:
	-../Bochs/bochsdbg.exe -q -f bochsrc.txt

run:
	-../Qemu/qemu-system-i386w.exe -L ../Qemu/Bios -m 4 \
		-boot order=c -hda hd.img
#	-../Qemu-1.5.1/qemu-system-i386w.exe -L ../Qemu/Bios -m 4 \
#		-boot order=a -fda floppy.img

bochs:
	-../Bochs/bochs.exe -q -f bochsrc.txt

clean:
	-$(RM)  *.o tlsf/*.o *.bin *.*~ $(PROG).out #$(PROG).map

epos.pdf: epos.tex
	pdflatex epos
	pdflatex epos
	-$(RM) *.aux *.log *.out *.nav *.snm *.toc *.vrb *.lol
