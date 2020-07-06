# OS/Z - Copyright (c) 2017 bzt (bztsrc@gitlab), CC-by-nc-sa
# Use is subject to license terms.

ifneq ("$(wildcard Config)","")
include Config
endif
export L = $(shell echo $(LANG)|cut -b 1-2)
export O = @
# nagyon részletes kimenet
export ECHO = true
#export ECHO = echo
OMVF = /usr/share/qemu/bios-TianoCoreEFI.bin

all: clrdd todogen util refgen system apps usrs images
mind: all

# debugger (bocs, nem szép megoldás, de nálam működik)
ifeq ($(ARCH),x86_64)
GDB = gdb
else
GDB = aarch64-linux-gnu-gdb
endif

ifeq ($(L),hu)
TESTTXT = TESZT
else
TESTTXT = TEST
endif

clrdd:
	@rm bin/disk.img bin/osZ-latest-$(ARCH)-$(PLATFORM).img 2>/dev/null || true

todogen:
	@echo "    ----------------- Hibajavítások / Error fixes ------------------" >TODO.txt
	@grep -n '[JF][AI][XV][MEÍTÁSANI]*:' `find . 2>/dev/null|grep -v './bin'` 2>/dev/null | grep -v Binary | grep -v grep >>TODO.txt || true
	@echo "    ---------- Lefejlesztendő funkciók / Feature requests ----------" >>TODO.txt
	@grep -n '[T][EO][ED][NO][DŐ]*:' `find . 2>/dev/null|grep -v './bin'` 2>/dev/null | grep -v Binary | grep -v grep >>TODO.txt || true

refgen:
	@make --no-print-directory -C tools elftool.o | grep -v 'Nothing to be done' | grep -v 'up to date' || true
	@./tools/elftool.o -r docs/refsys.md src/core/*.c src/core/*.h src/core/x86_64/*.c src/core/x86_64/*.h src/core/x86_64/*.S src/core/x86_64/ibmpc/*.c src/core/x86_64/ibmpc/*.h src/core/x86_64/ibmpc/*.S src/fs/*.c
	@./tools/elftool.o -r docs/refusr.md include/osZ/stdlib.h src/libc/*.c src/libc/*.h src/libc/x86_64/*.h src/libc/x86_64/*.S src/drivers/bridge/pci/main.c

# fordításhoz szükséges eszközök

config:
	@make --no-print-directory -C tools config | grep -v 'Nothing to be done' | grep -v 'up to date' || true
	@cd tools && ./config
	@true

util: tools
	@cat etc/etc/os-release | grep -v ^BUILD | grep -v ^ARCH | grep -v ^PLATFORM >/tmp/os-release
	@mv /tmp/os-release etc/etc/os-release
	@date +'BUILD = "%Y%j"' >>etc/etc/os-release
	@echo 'ARCH = "$(ARCH)"' >>etc/etc/os-release
	@echo 'PLATFORM = "$(PLATFORM)"' >>etc/etc/os-release
ifeq ($(L),hu)
	@echo "SEGÉDPROGRAMOK"
else
	@echo "TOOLS"
endif
	@make -e --no-print-directory -C tools all | grep -v 'Nothing to be done' || true

# alaprendszer (core és libc)

system: src
ifeq ($(L),hu)
	@echo "RENDSZERMAG"
else
	@echo "CORE"
endif
	@make -e --no-print-directory -C src -Oline system | grep -v 'Nothing to be done' || true

# felhasználói programok

apps: src
ifeq ($(L),hu)
	@echo "ALAPRENDSZER"
else
	@echo "BASE"
endif
	@make -e --no-print-directory -C src libs | grep -v 'Nothing to be done' || true
	@make -e --no-print-directory -C src apps | grep -v 'Nothing to be done' || true
ifeq ($(L),hu)
	@echo "ESZKÖZMEGHAJTÓK"
else
	@echo "DRIVERS"
endif
	@make -e --no-print-directory -C src drivers | grep -v 'Nothing to be done' || true
ifeq ($(DEBUG),1)
	@make -e --no-print-directory -C src gensyms 2>&1 | grep -v 'Nothing to be done' | grep -v 'No rule to make target' || true
endif

usrs: usr
ifeq ($(L),hu)
	@echo "ALKALMAZÁSOK"
else
	@echo "USERSPACE"
endif
	@make -e --no-print-directory -C usr all | grep -v 'Nothing to be done' || true

# lemezkép kreálás

images: tools
ifeq ($(L),hu)
	@echo "LEMEZKÉPEK"
else
	@echo "IMAGES"
endif
	@make -e --no-print-directory -C tools images | grep -v 'Nothing to be done' | grep -v 'lowercase' || true

bin/disk.vdi: vdi

vdi: images
	@make -e --no-print-directory -C tools vdi | grep -v 'Nothing to be done' || true

bin/disk.vmdk: vmdk

vmdk: images
	@make -e --no-print-directory -C tools vmdk | grep -v 'Nothing to be done' || true

tiszta: clean
clean:
	@rm bin/disk.img.lock 2>/dev/null || true
	@rm bin/disk.img 2>/dev/null || true
	@make -e --no-print-directory -C src clean
	@make -e --no-print-directory -C tools clean
	@make -e --no-print-directory -C tools imgclean

# debuggolás

debug: gdbqemu test

gdbqemu:
ifeq ($(DEBUG),1)
	$(eval QEMUGDB := -s -S)
else
ifeq ($(L),hu)
	@echo Debug szimbólumok nélkül lett fordítva! Állítsd át DEBUG = 1 -re a Config-ban és fordítsd újra.
else
	@echo Compiled without debugging symbols! Set DEBUG = 1 in Config and recompile.
endif
endif

gdb:
	@$(GDB) -w -x "etc/script.$(ARCH).gdb" || true
	@pkill qemu

# tesztelés

run: test
teszt: test
ifeq ($(ARCH),x86_64)
test: testq
else
test: testr
endif

testesp:
	@echo "$(TESTTXT)"
	@echo
	qemu-system-x86_64 -name OS/Z -bios $(OMVF) -m 64 -device isa-debug-exit,iobase=0x8900,iosize=4 -hda fat:bin/ESP -enable-kvm -cpu host,+ssse3,+avx,+x2apic -serial stdio
	@printf "\033[0m"

teste: bin/osZ-latest-$(ARCH)-$(PLATFORM).img
	@echo "$(TESTTXT)"
	@echo
	@#qemu-system-x86_64 -name OS/Z -bios $(OMVF) -m 64 -device isa-debug-exit,iobase=0x8900,iosize=4 -hda bin/osZ-latest-$(ARCH)-$(PLATFORM).img -option-rom loader/bootboot.rom -d guest_errors -enable-kvm -cpu host,+avx,+x2apic -serial stdio
	qemu-system-x86_64 -name OS/Z -bios $(OMVF) -m 64 $(QEMUGDB) -d guest_errors,int -device isa-debug-exit,iobase=0x8900,iosize=4 -drive file=bin/osZ-latest-$(ARCH)-$(PLATFORM).img,format=raw -enable-kvm -cpu host,+ssse3,+avx,+x2apic -serial stdio
	@printf "\033[0m"

testq: bin/osZ-latest-$(ARCH)-$(PLATFORM).img
	@echo "$(TESTTXT)"
	@echo
	@#qemu-system-x86_64 -no-hpet -name OS/Z -sdl -m 32 -d guest_errors -device isa-debug-exit,iobase=0x8900,iosize=4 -hda bin/osZ-latest-$(ARCH)-$(PLATFORM).img -option-rom loader/bootboot.bin -enable-kvm -cpu host,+avx,+x2apic,enforce -serial stdio
	@#qemu-system-x86_64 -no-hpet -name OS/Z -sdl -m 32 -d guest_errors -device isa-debug-exit,iobase=0x8900,iosize=4 -hda bin/osZ-latest-$(ARCH)-$(PLATFORM).img -option-rom loader/bootboot.bin -enable-kvm -machine kernel-irqchip=on -cpu host,+avx,+x2apic,enforce -serial stdio
	@#qemu-system-x86_64 -name OS/Z -sdl -m 32 -d guest_errors -device isa-debug-exit,iobase=0x8900,iosize=4 -hda bin/osZ-latest-$(ARCH)-$(PLATFORM).img -enable-kvm -cpu host,+ssse3,+avx,+x2apic -serial stdio
	qemu-system-x86_64 -name OS/Z -sdl -m 32 $(QEMUGDB) -d guest_errors,int -device isa-debug-exit,iobase=0x8900,iosize=4 -drive file=bin/osZ-latest-$(ARCH)-$(PLATFORM).img,format=raw -enable-kvm -cpu host,+ssse3,+avx,+x2apic -serial stdio

testb: bin/osZ-latest-$(ARCH)-$(PLATFORM).img
	@echo "$(TESTTXT)"
	@echo
	@rm bin/disk.img.lock 2>/dev/null || true
	@ln -s osZ-latest-$(ARCH)-$(PLATFORM).img bin/disk.img 2>/dev/null || true
	@touch bin/core.sym
ifneq ($(wildcard /usr/local/bin/bochs),)
	/usr/local/bin/bochs -f etc/bochs.rc -q
else
	bochs -f etc/bochs.rc -q
endif
	@rm bin/disk.img.lock 2>/dev/null || true
	@rm bin/disk.img 2>/dev/null || true

testv: bin/disk.vdi
	@echo "$(TESTTXT)"
	@echo
	VBoxManage startvm "OS/Z"

testr: bin/osZ-latest-$(ARCH)-$(PLATFORM).img
	@echo "$(TESTTXT)"
	@echo
	qemu-system-aarch64 -name OS/Z -sdl -M raspi3 $(QEMUGDB) -semihosting -kernel loader/bootboot.img -drive file=bin/osZ-latest-$(ARCH)-$(PLATFORM).img,if=sd,format=raw -serial stdio
