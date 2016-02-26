.PHONY: all
all: hd.img

MODE = run
SUBDIRS = kernel userapp

.PHONY: subdirs $(SUBDIRS)
subdirs: $(SUBDIRS)
$(SUBDIRS):
	$(MAKE) -C $@ $(MODE)

hd.img: subdirs
ifeq ($(OS),Windows_NT)
	if [ ! -s $@ ]; then base64 -d $@.bz2.txt | bunzip2 >$@ ; fi
	imgcpy kernel/eposkrnl.bin $@=C:\eposkrnl.bin
	imgcpy userapp/a.out $@=C:\a.out
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

.PHONY: qemu
qemu: hd.img
ifeq ($(OS),Windows_NT)
	-qemu-system-i386w -m 16 -boot order=c -vga std -soundhw pcspk -hda hd.img -L $(QEMUHOME)/Bios
else
	-qemu-system-i386  -m 16 -boot order=c -vga std -soundhw pcspk -hda hd.img
endif

.PHONY: qemudbg
qemudbg: MODE=debug
qemudbg: hd.img
ifeq ($(OS),Windows_NT)
	-qemu-system-i386w -S -gdb tcp::1234,nowait,nodelay,server,ipv4 -m 16 -boot order=c -vga std -soundhw pcspk -hda hd.img -L $(QEMUHOME)/Bios
else
	-qemu-system-i386  -S -gdb tcp::1234,nowait,nodelay,server,ipv4 -m 16 -boot order=c -vga std -soundhw pcspk -hda hd.img
endif

.PHONY: bochs
bochs: hd.img
ifeq ($(OS),Windows_NT)
	-bochs -q -f bochsrc-win.txt
else
	-bochs -q -f bochsrc-unix.txt
endif

.PHONY: bochsdbg
bochsdbg: MODE=debug
bochsdbg: hd.img
ifeq ($(OS),Windows_NT)
	-bochsdbg -q -f bochsrc-win.txt
else
	-bochsdbg -q -f bochsrc-unix.txt
endif

.PHONY: tags
tags:
	ctags -R *

hd.vmdk: hd.img
	qemu-img convert -O vmdk $^ $@

.PHONY: run
run: qemu

.PHONY: debug
debug: qemudbg

.PHONY: clean
clean:
	@for subdir in $(SUBDIRS); do $(MAKE) -C $${subdir} $@; done
	$(RM) hd.img hd.vmdk tags

.PHONY: submit
submit: clean
	@all="$(shell svn status | grep '^[M?]' | cut -c9- | tr '\\' '/')"; \
	if [ -z "$${all}" ]; then echo ">>> No files changed."; exit; fi; \
	echo ">>> Files to be submitted: $${all}"; \
	size=`tar -cjO $${all} | wc -c`; \
	echo -n ">>> Total size in bytes $${size}"; \
	if [ $${size} -gt $$((64*1024)) ]; then echo " exceeds 64KiB."; exit; else echo "."; fi; \
	echo -n ">>> Enter student ID: "; \
	read sid; \
	if ! echo "$${sid}" | grep -qE '^[0-9]{8}$$'; then echo ">>> Student ID must be 8 digits."; exit; fi;\
	tar -cjf $${sid}.tbz $${all}; \
	trap "rm $${sid}.tbz; exit" 1 2 3 15; \
	curl -isS -X POST -H "Content-Type: multipart/form-data" -F "myFile=@$${sid}.tbz" \
		 http://xgate.dhis.org/osexp/submit.php | sed -n "s@<div .*>\(.*\)<\/div>@>>> \1\n@p"; \
	$(RM) $${sid}.tbz
