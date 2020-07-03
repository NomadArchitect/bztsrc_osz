OS/Z - operating system
=======================

<img align="left" style="margin-right:10px;" alt="OS/Z" src="https://gitlab.com/bztsrc/osz/raw/master/logo.png">
<a href="https://gitlab.com/bztsrc/osz/tree/master/bin/">Download live images</a>,  <small>(~8 Mbyte)</small><br>
<a href="https://gitlab.com/bztsrc/osz/blob/master/docs/README.en.md">Documentation</a> and <a href="https://bztsrc.gitlab.io/osz">homepage</a><br>
<a href="https://gitlab.com/bztsrc/osz/issues">Support</a><br><br>

The [OS/Z](https://bztsrc.gitlab.io/osz) is a modern, effective, and scalable Operating System. It's aim is to be small, elegant,
[portable](https://gitlab.com/bztsrc/osz/blob/master/docs/porting.en.md) and to handle enormous amounts of data in a user friendly way.

To achieve its goal, I've eliminated as many limits as possible by design, therefore it is POSIX-ish, but deliberately
[not POSIX compliant](https://gitlab.com/bztsrc/osz/blob/master/docs/posix.md). For example only storage capacity limits the number
of inodes on a disk. And only amount of RAM limits the number of concurent tasks at any given time. If I couldn't eliminate a hard
limit, I've created a [boot option](https://gitlab.com/bztsrc/osz/blob/master/docs/bootopts.en.md) for it so that you can tweek it
without recompilation. This makes OS/Z a very scalable system.

Features
--------

 - [GNU or LLVM toolchain](https://gitlab.com/bztsrc/osz/blob/master/docs/compile.en.md)
 - Microkernel architecture with an effective [messaging system](https://gitlab.com/bztsrc/osz/blob/master/docs/messages.en.md)
 - Single disk image for [booting](https://gitlab.com/bztsrc/osz/blob/master/docs/boot.en.md) from BIOS or from UEFI or on Raspberry Pi.
 - [Higher half kernel](https://gitlab.com/bztsrc/osz/blob/master/docs/memory.en.md) mapping, full 64 bit support
 - It's [filesystem](https://gitlab.com/bztsrc/osz/blob/master/docs/fs.en.md) can handle YottaBytes of data (unimagineable as of writing)
 - ELF64 object format support
 - UNICODE support with UTF-8
 - Multilingual

Hardware Requirements
---------------------

 - 10 Mb free disk space
 - 32 Mb RAM
 - 800 x 600 / ARGB display
 - IBM PC with x86_64 processor  - or -  Raspberry Pi 3 with AArch64 processor
 - [Supported devices](https://gitlab.com/bztsrc/osz/blob/master/docs/drivers.en.md)

Testing
-------

The [latest live image](https://gitlab.com/bztsrc/osz/raw/master/bin/osZ-latest-x86_64-ibmpc.img) should boot OS/Z in emulators and on real machines. For example type

```shell
qemu-system-x86_64 -hda bin/osZ-latest-x86_64-ibmpc.img
```
For more options, see [Testing How To](https://gitlab.com/bztsrc/osz/blob/master/docs/howto1-testing.en.md). I usually test the image
with [qemu](http://www.qemu.org/), [bochs](http://bochs.sourceforge.net/) and [VirtualBox](https://www.virtualbox.org/).

License
-------

The boot loader and the [BOOTBOOT](https://gitlab.com/bztsrc/bootboot) Protocol, the [memory allocator](http://gitlab.com/bztsrc/bztalloc) and the
[on disk format of FS/Z](https://gitlab.com/bztsrc/osz/blob/master/include/osZ/fsZ.h) are licensed under MIT licence. Because the libfuse
library is licensed under GPL, therefore the [FS/Z FUSE](https://gitlab.com/bztsrc/osz/blob/master/tools/fsZ-fuse.c) driver is licensed under GPL as well.
All the other parts of OS/Z (including the native [FS/Z](https://gitlab.com/bztsrc/osz/blob/master/docs/fs.en.md) implementation) licensed under CC-by-nc-sa-4.0.
The device drivers are an exception, for them closed-source proprietary binary distribution is allowed.

This means although OS/Z is **Open Source**, it is **not a Free Software**. You can use it and play with it for your own pleasure at home, but
there’s absolutely no warranty applied. You are not allowed to use it in business environment or for commertial purposes without preliminary
written permission from the author. Open an [issue](https://gitlab.com/bztsrc/osz/issues) or drop me an email if you need one.

 Copyright (c) 2016-2020 bzt (bztsrc@gitlab) [CC-by-nc-sa-4.0](https://creativecommons.org/licenses/by-nc-sa/4.0/)

**You are free to**:

 - **Share** — copy and redistribute the material in any medium or format

 - **Adapt** — remix, transform, and build upon the material
     The licensor cannot revoke these freedoms as long as you follow
     the license terms.

**Under the following terms**:

 - **Attribution** — You must give appropriate credit, provide a link to
     the license, and indicate if changes were made. You may do so in
     any reasonable manner, but not in any way that suggests the
     licensor endorses you or your use.

 - **NonCommercial** — You may not use the material for commercial purposes.

 - **ShareAlike** — If you remix, transform, or build upon the material,
     you must distribute your contributions under the same license as
     the original.

Project Structure
=================

```
(git repository)
|
|_ bin/                 generated files
|   |_ ESP/             files of the boot partition (FAT, boot files)
|   |_ home/            files of the home partition (private user data)
|   |_ initrd/          files of the initrd (ESP/BOOTBOOT/INITRD or boot partition)
|   |_ usr/             files of the usr partition (system partition)
|   |_ var/             files of the var partition (public variable data)
|   |_ *.part           partition images
|   \_ *.img            disk images
|
|_ docs/                documentation
|   \_ README.en.md     main documentation index
|
|_ etc/                 et cetera, miscelangelous files
|   |_ root/            skeleton for root's home, /root on initrd
|   \_ sys/             skeleton for system configuration files
|       |_ etc/         skeleton for /sys/etc on initrd
|       |_ lang/        language dictionaries for core and libc
|       \_ config       boot environment configuration file (ESP/BOOTBOOT/CONFIG, generated by "make config")
|
|_ include/             C header files, used by the build system (also installed to /usr/sys/include)
|
|_ loader/              pre-compiled boot loader images, see BOOTBOOT Protocol
|
|_ src/                 source files
|   |_ core/            supervisor
|   |   |_ aarch64/     architecture specific sources
|   |   |   |_ rpi3/    platform specific sources
|   |   |   |_ rpi4/    platform specific sources
|   |   \_ x86_64/      architecture specific sources
|   |       |_ ibmpc/   platform specific sources
|   |       \_ apic/    platform specific sources
|   |_ drivers/         source of device drivers
|   |   |_input/        example device category
|   |   |   \_ps2/      a device driver
|   |   |_display/      example device category
|   |   |   \_bga/      a device driver
|   |   \_*             device categories
|   |_ libc/            source of C library
|   |_ sh/              source of shell
|   |_ */               necessary system services, libraries and applications
|   \_ *.ld             linker scripts
|
|_ tools/               scripts and utilities used by the build system
|
|_ usr/                 userspace packages, installed to usr partition
|   \_ sys/etc/         skeleton for /usr/sys/etc
|
|_ Config               build system configuration file (generated by "make config")
|_ TODO.txt             generated to-do list
\_ Makefile             the main project GNU make file
```

For the generated file system directory structure, see [VFS](https://gitlab.com/bztsrc/osz/blob/master/docs/vfs.en.md).

Authors
-------

qsort: Copyright The Regents of the University of California

BOOTBOOT, FS/Z, OS/Z, bztalloc: bzt

Contributions
-------------

Geri: testing

jahboater: testing, ARM optimization suggestions

Octocontrabass: testing

BenLunt: constructive FS/Z criticism
