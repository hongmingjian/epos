include Makefile.inc

ifeq ($(OS),Windows_NT)
W=w
SERIAL=COM3
else
W=
SERIAL=stdio
endif

QFLAGS =-M raspi2 -cpu arm1176 -m 512 -dtb bcm2709-rpi-2-b.dtb -serial $(SERIAL)
QFLAGS+=-nographic

.PHONY: all
all: subdirs

MODE = release
SUBDIRS = kernel

.PHONY: subdirs $(SUBDIRS)
subdirs: $(SUBDIRS)
$(SUBDIRS):
	$(MAKE) -C $@ $(MODE)

.PHONY: qemu
qemu: MODE=qemu
qemu: subdirs
	-qemu-system-arm$(W) -kernel kernel/kernel.img $(QFLAGS)

.PHONY: qemudbg
qemudbg: MODE=qemudbg
qemudbg: subdirs
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
	-qemu-system-arm$(W) -kernel kernel/kernel.img $(QFLAGS) -S -gdb tcp::1234,nowait,nodelay,server,ipv4

.PHONY: run
run: qemu

.PHONY: debug
debug: qemudbg

.PHONY: clean
clean:
	@for subdir in $(SUBDIRS); do $(MAKE) -C $${subdir} $@; done

