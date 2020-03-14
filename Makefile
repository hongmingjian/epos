include Makefile.inc

ifeq ($(OS),Windows_NT)
W=w
SERIAL=COM3
else
W=
SERIAL=stdio
endif

QFLAGS =-M raspi2 -cpu arm1176 -m 512 -serial $(SERIAL)
QFLAGS+=-nographic

MODE = release
SUBDIRS = kernel userapp

.PHONY: all
all: subdirs

.PHONY: subdirs $(SUBDIRS)
subdirs: $(SUBDIRS)
$(SUBDIRS):
	$(MAKE) -C $@ $(MODE)

hd.img: subdirs
ifeq ($(OS),Windows_NT)
	if [ ! -s $@ ]; then base64 -d $@.bz2.txt | bunzip2 >$@ ; fi
	#imgcpy kernel/kernel.img $@=C:\kernel.img
	imgcpy userapp/a.out $@=C:\a.out
else
ifeq ($(shell uname -s),Linux)
	if [ ! -s $@ ]; then base64 -d $@.bz2.txt | bunzip2 >$@ ; fi
	sudo mount -o loop,offset=32256,umask=0022,gid=$(shell id -g),uid=$(shell id -u) -t vfat $@ /mnt
	#cp kernel/kernel.img /mnt
	cp userapp/a.out /mnt
	sudo umount /mnt
endif
ifeq ($(shell uname -s),Darwin)
	if [ ! -s $@ ]; then base64 -D $@.bz2.txt | bunzip2 >$@ ; fi
	hdiutil attach -imagekey diskimage-class=CRawDiskImage $@
	#cp kernel/kernel.img /Volumes/EPOSDISK
	cp userapp/a.out /Volumes/EPOSDISK
	hdiutil detach /Volumes/EPOSDISK
endif
endif

.PHONY: qemu
qemu: MODE=qemu
qemu: hd.img
	-qemu-system-arm$(W) -kernel kernel/kernel.img -sd $^ $(QFLAGS)

.PHONY: qemudbg
qemudbg: MODE=qemudbg
qemudbg: hd.img
ifeq ($(OS),Windows_NT)
	-start $(GDB)
else
ifeq ($(shell uname -s),Linux)
	-/usr/bin/x-terminal-emulator -e $(GDB)
endif
ifeq ($(shell uname -s),Darwin)
	-osascript -e 'on run argv' \
	           -e '  tell application "Terminal" to do script "cd $(shell pwd); $(GDB); exit"' \
	           -e 'end run'
endif
endif
	-qemu-system-arm$(W) -kernel kernel/kernel.img -sd $^ $(QFLAGS) -S -gdb tcp::1234,nowait,nodelay,server,ipv4

.PHONY: run
run: qemu

.PHONY: debug
debug: qemudbg

.PHONY: clean
clean:
	@for subdir in $(SUBDIRS); do $(MAKE) -C $${subdir} $@; done

