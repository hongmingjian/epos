include Makefile.inc

ifeq ($(OS),Windows_NT)
W=w
else
W=
endif

.PHONY: all
all: subdirs

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
	sudo mount -o loop,offset=32256,umask=0022,gid=$(shell id -g),uid=$(shell id -u) -t vfat $@ /mnt
	cp kernel/eposkrnl.bin userapp/a.out /mnt
	sudo umount /mnt
endif
ifeq ($(shell uname -s),Darwin)
	if [ ! -s $@ ]; then base64 -D -i $@.bz2.txt | bunzip2 >$@ ; fi
	hdiutil attach -imagekey diskimage-class=CRawDiskImage $@
	cp kernel/eposkrnl.bin userapp/a.out /Volumes/EPOSDISK
	hdiutil detach /Volumes/EPOSDISK
endif
endif

.PHONY: qemu
qemu: hd.img
	-qemu-system-i386$(W) -m 32 -boot order=c -vga std -drive format=raw,file=$^,index=0,media=disk

.PHONY: qemudbg
qemudbg: MODE=debug
qemudbg: hd.img
ifeq ($(OS),Windows_NT)
	-start $(GDB) -iex 'add-auto-load-safe-path .'
else
ifeq ($(shell uname -s),Linux)
	-/usr/bin/x-terminal-emulator -e $(GDB) -iex 'add-auto-load-safe-path .' &
endif
ifeq ($(shell uname -s),Darwin)
	-osascript -e 'on run argv' \
			   -e '  tell application "Terminal" to do script "cd $(shell pwd); $(GDB); exit"' \
			   -e 'end run'
endif
endif
	-qemu-system-i386$(W) -S -s -m 32 -boot order=c -vga std -drive format=raw,file=$^,index=0,media=disk

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

.PHONY: vbox
vbox: hd.vmdk
	@if ! VBoxManage -q list vms | grep epos >/dev/null; then \
		VBoxManage -q createvm --name epos --ostype Other --register; \
	else \
		VBoxManage -q controlvm epos poweroff; \
		sleep 1; \
		VBoxManage -q storagectl epos --name IDE --remove; \
		VBoxManage -q closemedium disk $^; \
	fi
	@VBoxManage -q modifyvm epos --cpus 1 --memory 32 --boot1 disk --nic1 bridged --nictype1 82540EM  --bridgeadapter1 en0 --macaddress1 auto
	@VBoxManage -q storagectl epos --add ide --name IDE
	@VBoxManage -q storageattach epos --storagectl IDE --port 0 --device 0 --type hdd --medium $^
	@VBoxManage startvm epos

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
	$(RM) hd.img hd.vmdk tags .~* *.*~ ._* .DS_Store *.stackdump

.PHONY: diff
diff:
	@svn diff -x --ignore-eol-style | tr -d '\r' | gvim -u NONE -c "set titlestring=$(shell pwd)" \
																-c "set enc=utf-8" \
																-c "set go-=T" \
																-c "syn on" \
																-c "set ft=diff" -dmMnR - 2>&1 >/dev/null

.PHONY: submit
submit: clean
	@all=`svn status | grep '^[M?]' | cut -c9- | grep -v '^[[:punct:]]' | tr '\\\\' '/' | tr '\n' ' '`; \
	if [ -z "$${all}" ]; then echo ">>> No files changed."; exit; fi; \
	printf "%s\e[1;31m%s\e[0m\n" ">>> Files to be submitted: " "$${all}"; \
	size=`tar -cf- $${all} | bzip2 | wc -c | tr -d '[:space:]'`; \
	echo -n ">>> Total size of bytes $${size}"; \
	if [ $${size} -gt $$((64*1024)) ]; then echo " exceeds the limit of 64KiB."; exit; else echo ""; fi; \
	echo -n ">>> Enter student ID: "; \
	read sid; \
	if ! echo "$${sid}" | grep -qE '^[0-9]{8}$$'; then echo ">>> Student ID must be 8 digits."; exit; fi;\
	echo -n ">>> Enter password: "; \
	read passwd; \
	if [ -z "$${passwd}" ]; then echo ">>> Password cannot be empty."; exit; fi; \
	tar -cf- $${all} | bzip2 >$${sid}.tbz; \
	trap "rm $${sid}.tbz; exit" 1 2 3 15; \
	RESP=$$(curl -s -d "passwd=$${passwd}" "http://xgate.dhis.org:8080/osexp/submit.php?sid=$${sid}" | sed -n 's@<div .*>\(.*\)<\/div>@\1@p'); \
	if echo "$${RESP}" | grep -q '^token='; then \
		curl -s -F "myFile=@$${sid}.tbz" "http://xgate.dhis.org:8080/osexp/upload.php?sid=$${sid}&$${RESP}" \
	    | sed -n 's@<div .*>\(.*\)<\/div>@>>> \1\'$$'\n@p'; \
	else \
		echo ">>> $${RESP}"; \
	fi; \
	$(RM) $${sid}.tbz;

