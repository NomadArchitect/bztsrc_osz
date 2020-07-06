OS/Z Compilation
================

Requirements
------------

- GNU toolchain (make, gcc, binutils (gas, ld, objcopy, strip), see [tools/cross-gcc.sh](https://gitlab.com/bztsrc/osz/blob/master/tools/cross-gcc.sh))
- instead of gcc and ld, you can use LLVM Clang and LLVM lld

I compile under Linux, that's guaranteed to work. Should be no issue under BSDs either. Not sure about MinGW though.

Configuration
-------------

The `core` is always compiled for a specific [machine](https://gitlab.com/bztsrc/osz/blob/master/docs/porting.md),
which can be controlled in [Config](https://gitlab.com/bztsrc/osz/blob/master/Config) with `ARCH` and `PLATFORM` variables.
Valid combinations are (note: the configurator collects these by examining directories in src/core):

| ARCH    | PLATFORM | Description |
| ------- | -------- | ----------- |
| x86_64  | ibmpc    | For old legacy machines, uses PIC, PIT, RTC, enumerates PCI bus |
| x86_64  | acpi     | For new machines, LAPIC, IOAPIC, HPET and parses ACPI tables |
| aarch64 | rpi3     | Raspberry Pi 3+ |
| aarch64 | rpi4     | Raspberry Pi 4+ |

If you have the "dialog" tool installed on your system, you can run the following to get a nice ncurses based interface.

```shell
$ make config
```

<img height="64" src="https://gitlab.com/bztsrc/osz/raw/master/docs/oszcfg1.png" alt="make config">


Compilation
-----------

The make system is created in a way that you can say `make all` in almost every directory. If you do that in the top level
directory, it will compile everything and will also build disk image for you, but surpressing messages, only showing errors.
In subdirectories make will always display the full command executed during compilation.

For example if you enter `src/fs` and issue `make all` there, then only the filesystem subsystem will be recompiled, and
you'll be able to see the exact commands used.

So to compile all the things, in the top level directory simply type

```shell
$ make all
```

You should see something similar to this:

```
$ make all
TOOLS
  src		mkfs.c
  src		elftool.c
CORE
  lnk		sys/core (x86_64-ibmpc)
  lnk		sys/lib/libc.so (x86_64)
BASE
  src		sys/ui
  src		sys/bin/sh
  src		sys/bin/test
  src		sys/bin/sys
  src		sys/syslog
  src		sys/sound
  src		sys/fs
  src		sys/inet
  src		sys/init
DRIVERS
  src		sys/drv/fs/gpt.so
  src		sys/drv/fs/pax.so
  src		sys/drv/fs/vfat.so
  src		sys/drv/fs/fsz.so
  src		sys/drv/fs/tmpfs.so
  src		sys/drv/input/ps2.so
  src		sys/drv/display/fb.so
  src		sys/driver
USERSPACE
IMAGES
  mkfs		initrd
  mkfs		boot (ESP)
  mkfs		usr
  mkfs		var
  mkfs		home
  mkfs		bin/osZ-latest-x86_64-ibmpc.img
```

Recompiling the Loader
----------------------

If you want to recompile `loader/boot.bin` and `loader/bootboot.bin`, you'll need [fasm](http://flatassembler.net).
Unfortunately GAS is not good enough at mixing 16, 32 and 64 bit instuctions, which is necessary for BIOS booting. To keep
my promise that you'll only need the GNU toolchain, I've added those pre-compiled [BOOTBOOT](https://gitlab.com/bztsrc/bootboot)
binaries to the source.

See what you've done!
---------------------

The live disk image is generated to [bin/osZ-(ver)-(arch)-(platform).img](https://gitlab.com/bztsrc/osz/blob/master/bin). Use the
[USBImager](https://gitlab.com/bztsrc/usbimager) application or the `dd` command to write it on a USB stick or SD card, or boot
with qemu, bochs or VirtualBox.

```
$ dd if=bin/osZ-latest-x86_64-acpi.img of=/dev/sdc
$ make testq
$ make testb
$ make testv
```

There's a detailed [tutorial on testing](https://gitlab.com/bztsrc/osz/blob/master/docs/howto1-testing.md).
