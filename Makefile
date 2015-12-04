all: hd.img

SUBDIRS = kernel userapp

.PHONY: subdirs $(SUBDIRS)
subdirs: $(SUBDIRS)
$(SUBDIRS):
	$(MAKE) -C $@

hd.img: subdirs
ifeq ($(OS),Windows_NT)
	if [ ! -s $@ ]; then base64 -d $@.bz2.txt | bunzip2 >$@ ; fi
	imgcpy.exe kernel/eposkrnl.bin $@=C:\eposkrnl.bin
	imgcpy.exe userapp/a.out $@=C:\a.out
else
ifeq ($(shell uname -s),Linux)
	if [ ! -s $@ ]; then base64 -d $@.bz2.txt | bunzip2 >$@ ; fi
	sudo mount -o loop,offset=32256 -t vfat $@ /mnt
	sudo cp kernel/eposkrnl.bin userapp/a.out /mnt
	sudo umount /mnt
endif
ifeq ($(shell uname -s),Darwin)
	if [ ! -s $@ ]; then base64 -D $@.bz2.txt | bunzip2 >$@ ; fi
	hdiutil attach -imagekey diskimage-class=CRawDiskImage $@
	cp kernel/eposkrnl.bin userapp/a.out /Volumes/EPOSDISK
	hdiutil detach /Volumes/EPOSDISK
endif
endif

.PHONY: tags
tags:
	ctags -R *

hd.vmdk: hd.img
	qemu-img convert -O vmdk $^ $@

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
	$(MAKE) -C kernel $@
	$(MAKE) -C userapp $@
	$(RM) hd.img hd.vmdk tags
