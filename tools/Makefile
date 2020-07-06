# OS/Z - Copyright (c) 2017 bzt (bztsrc@gitlab), CC-by-nc-sa
# Use is subject to license terms.

ifneq ("$(wildcard ../Config)","")
include ../Config
endif
ifeq ($(ARCH),x86_64)
INITRD = ../bin/ESP/BOOTBOOT/X86_64
else
INITRD = ../bin/ESP/BOOTBOOT/AARCH64
endif
ESPFILE = ../bin/esp.part
SYSFILE = ../bin/sys.$(ARCH).part
USRFILE = ../bin/usr.$(ARCH).part
VARFILE = ../bin/var.part
HOMEFILE = ../bin/home.part
DDIMAGE = ../bin/osZ-latest-$(ARCH)-$(PLATFORM).img
VDIIMAGE = ../bin/disk.vdi
VMDKIMAGE = ../bin/disk.vmdk
SRCS = mkfs.c elftool.c
OPT  = -DOSZ_ARCH='"$(ARCH)"'
ifeq ($(ARCH),x86_64)
OPT += -DPART_ARCH=0x8664
else
OPT += -DPART_ARCH=0xAA64
endif
LIBS =
OBJS = $(SRCS:.c=.o)
ifneq ("$(wildcard /usr/include/fuse/fuse.h)","")
#OBJS += fsZ-fuse
endif
ifneq ("$(wildcard /usr/include/zlib.h)","")
OPT += -DHAS_ZLIB=1
LIBS+= -lz
endif

CFLAGS = -ansi -Wall -Wextra -Wpedantic -fshort-wchar -DDEBUG=$(DEBUG)

all: selfcheck $(OBJS) ../etc/logo.tga

fsZ-fuse: fsZ-fuse.c
	@echo "  src		$<"
	@gcc `pkg-config fuse --cflags --libs` fsZ-fuse.c -o fsZ-fuse

config: config.c
	@echo "  src		$<"
	@gcc config.c -o config

%.o: %.c
	@echo "  src		$<"
	@gcc $(OPT) $(CFLAGS) -g $< -o $@ $(LIBS)

clean:
	@rm *.o fsZ-fuse config 2>/dev/null || true

imgclean:
	@rm -rf ../bin/*.txt ../bin/*.sym ../bin/*.part ../bin/*.vdi ../bin/*.vmdk ../bin/*.rom ../bin/initrd ../bin/usr ../bin/ESP ../bin/var ../bin/home 2>/dev/null || true

../etc/logo.tga: ../etc/logo.png
	@convert ../etc/logo.png -type Palette -flip -compress rle ../etc/logo.tga

esp: initrd
	@echo "  mkfs		boot (ESP)"
ifneq ($(NOINITRD),1)
	$(O)cp -f ../etc/config ../bin/ESP/BOOTBOOT/CONFIG 2>/dev/null || true
ifneq ("$(wildcard ../loader/bootboot.bin)","")
	$(O)cp ../loader/bootboot.bin ../bin/ESP/BOOTBOOT/LOADER || true
endif
endif
ifeq ($(ARCH),x86_64)
ifneq ("$(wildcard ../loader/bootboot.efi)","")
	@mkdir ../bin/ESP/EFI ../bin/ESP/EFI/BOOT 2>/dev/null || true
	$(O)cp ../loader/bootboot.efi ../bin/ESP/EFI/BOOT/BOOTX64.EFI || true
endif
else
ifneq ("$(wildcard ../loader/bootboot.img)","")
	$(O)cp ../loader/bootboot.img ../bin/ESP/kernel8.img || true
ifneq ("$(wildcard ../../bootboot/aarch64-$(ARCH)/start.elf)","")
	$(O)cp ../../bootboot/aarch64-$(ARCH)/LIC* ../../bootboot/aarch64-$(ARCH)/*.bin ../../bootboot/aarch64-$(ARCH)/*.elf ../../bootboot/aarch64-$(ARCH)/*.dat ../bin/ESP/ || true
endif
endif
endif
	@dd if=/dev/zero of=$(ESPFILE) bs=1024 count=$(ESPSIZE) >/dev/null 2>/dev/null
	$(O)mkfs.vfat -F 16 -n "EFI System" $(ESPFILE) 2>/dev/null >/dev/null
	@mkdir ../bin/tmp 2>/dev/null || true
	@sudo /usr/bin/mount -o loop,user $(ESPFILE) ../bin/tmp
	@sudo /usr/bin/cp -r ../bin/ESP/* ../bin/tmp/ || true
	@sudo /usr/bin/umount -f /dev/loop* 2>/dev/null || true
	@rmdir ../bin/tmp

initrd:
ifeq ($(NOINITRD),1)
	@echo "  mkfs		root"
else
	@echo "  mkfs		initrd"
endif
	@# feltöltés statikus fájlokkal
	@cp -r ../etc/etc ../bin/initrd/sys || true
	@./mkfs.o lang ../etc/lang ../bin/initrd/sys/lang || true
ifeq ($(NOINITRD),1)
	@cp -f ../etc/config ../bin/initrd/sys 2>/dev/null || true
	@echo "/dev/disk0p2	/        	rw		1" >../bin/initrd/sys/etc/fstab
	@cat ../etc/etc/fstab | grep -v "/dev/mem" >>../bin/initrd/sys/etc/fstab
endif
	@cp -r ../etc/root ../bin/initrd 2>/dev/null || true
ifeq ($(NOINITRD),1)
ifneq ("$(wildcard ../loader/bootboot.bin)","")
	$(O)cp ../loader/bootboot.bin ../bin/initrd/sys/loader || true
endif
	$(O)./mkfs.o $(SYSFILE) ../bin/initrd
	@./mkfs.o $(SYSFILE) union bin/ "/sys/bin/" "/usr/.../bin/"
	@./mkfs.o $(SYSFILE) union etc/ "/sys/etc/" "/usr/.../etc/"
	@./mkfs.o $(SYSFILE) union lib/ "/sys/lib/" "/usr/.../lib/"
else
	$(O)./mkfs.o $(INITRD) ../bin/initrd
	@./mkfs.o $(INITRD) union bin/ "/sys/bin/" "/usr/.../bin/"
	@./mkfs.o $(INITRD) union etc/ "/sys/etc/" "/usr/.../etc/"
	@./mkfs.o $(INITRD) union lib/ "/sys/lib/" "/usr/.../lib/"
ifneq ($(PLATFORM),rpi3)
ifneq ($(PLATFORM),rpi4)
	@gzip $(INITRD) && mv $(INITRD).gz $(INITRD) || true
endif
endif
	@#./mkfs.o $(INITRD) initrdrom ../bin/initrd.rom
endif

usr: $(USRFILE)

$(USRFILE): ../bin/usr
	@echo "  mkfs		usr"
	@cp -r ../usr/sys/etc ../bin/usr/sys || true
	@cp -r ../include ../bin/usr/sys || true
	@dd if=/dev/zero of=$(USRFILE) bs=1024 count=$(USRSIZE) >/dev/null 2>/dev/null
	$(O)./mkfs.o $(USRFILE) ../bin/usr
	@./mkfs.o $(USRFILE) union bin/ "/usr/.../bin"
	@./mkfs.o $(USRFILE) union include/ "/usr/.../include"
	@./mkfs.o $(USRFILE) union lib/ "/usr/.../lib"
	@./mkfs.o $(USRFILE) union share/ "/usr/.../share"

var: $(VARFILE)

$(VARFILE): ../bin/var
	@echo "  mkfs		var"
	@dd if=/dev/zero of=$(VARFILE) bs=1024 count=$(VARSIZE) >/dev/null 2>/dev/null

home: $(HOMEFILE)

$(HOMEFILE): ../bin/home
	@echo "  mkfs		home"
	@dd if=/dev/zero of=$(HOMEFILE) bs=1024 count=$(HOMESIZE) >/dev/null 2>/dev/null

images: esp $(USRFILE) $(VARFILE) $(HOMEFILE)
	@echo "  mkfs		bin/osZ-latest-$(ARCH)-$(PLATFORM).img"
ifneq ($(NOINITRD),1)
	$(O)./mkfs.o diskiso "osZ-latest-$(ARCH)-$(PLATFORM).img"
else
	$(O)./mkfs.o disk "osZ-latest-$(ARCH)-$(PLATFORM).img"
endif

vdi: $(DDIMAGE)
	@echo "  mkfs		bin/disk.vdi"
	@rm -f $(VDIIMAGE) 2>/dev/null || true
	@VBoxManage convertfromraw $(DDIMAGE) $(VDIIMAGE)
	@VBoxManage internalcommands sethduuid $(VDIIMAGE) 45a68a8d-9a2b-42ba-91f9-99c7259bfa8a 2>/dev/null || true

vmdk: $(DDIMAGE)
	@#echo "  mkfs		bin/disk.vdmk"
	@VBoxManage convertdd $(DDIMAGE) $(VMDKIMAGE) --format VMDK

selfcheck:
ifeq ("$(wildcard ../bin)","")
	@mkdir ../bin
endif
ifneq ($(NOINITRD),1)
ifeq ("$(wildcard ../bin/ESP/BOOTBOOT)","")
	@mkdir -p ../bin/ESP/BOOTBOOT
endif
endif
ifeq ("$(wildcard ../bin/initrd)","")
	@mkdir ../bin/initrd || true
	@mkdir ../bin/initrd/dev || true
	@mkdir ../bin/initrd/sys || true
	@mkdir ../bin/initrd/sys/bin || true
	@mkdir ../bin/initrd/sys/etc || true
	@mkdir ../bin/initrd/sys/lib || true
	@mkdir ../bin/initrd/root || true
endif
ifeq ("$(wildcard ../bin/usr)","")
	@mkdir ../bin/usr || true
	@mkdir ../bin/usr/sys || true
	@mkdir ../bin/usr/sys/bin
	@mkdir ../bin/usr/sys/etc
	@mkdir ../bin/usr/sys/include
	@mkdir ../bin/usr/sys/lib || true
	@mkdir ../bin/usr/sys/share || true
endif
ifeq ("$(wildcard ../bin/var)","")
	@mkdir ../bin/var || true
endif
ifeq ("$(wildcard ../bin/home)","")
	@mkdir ../bin/home || true
	@mkdir ../bin/home/bzt || true
endif
