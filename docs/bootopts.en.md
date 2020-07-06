OS/Z Boot Options
=================

Compilation Time Options
------------------------

A few options (mostly for performance reasons) moved to [Config](https://gitlab.com/bztsrc/osz/blob/master/Config). Changing these
will require a recompilation. But most of the features can be configured in boot-time. You can easily configure by running

```sh
$ make config
```

<img height="64" src="https://gitlab.com/bztsrc/osz/raw/master/docs/oszcfg1.png" alt="make config">


Environment Configuration File
------------------------------

This is similar to Linux command line. The boot options are kept on the first bootable partition on the first bootable disk under
`FS0:\BOOTBOOT\CONFIG` or `/sys/config`. When you're creating a disk image, the contents of that file are taken from
[etc/config](https://gitlab.com/bztsrc/osz/blob/master/etc/config).

This file is a plain UTF-8 file with `key=value` pairs, parsed by [core/env.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/env.c)
and [libc/env.c](https://gitlab.com/bztsrc/osz/blob/master/src/libc/env.c). No whitespaces allowed, and each pair is separated by a newline (0x0A) character.
The file can't be longer than a page (4096 bytes on x86_64 and AArch64). You can put comments in it with '#", '//' and '/*'.

Keys are ASCII names without spaces, values can be decimal and hexadecimal [numbers, booleans or UTF-8 strings](https://gitlab.com/bztsrc/osz/blob/master/docs/howto3-driver.md).

Boot Parameters
---------------

| Parameter  | Default   | Type    | Subsystem | Description |
| ---------- | :-------: | ------- | --------- | ----------- |
| screen     | 1024x768  | num<i>x</i>num | [loader](https://gitlab.com/bztsrc/osz/blob/master/loader) | required screen resolution, minimum 640x480 |
| kernel     | sys/core  | string  | loader   | the name of kernel executable on initrd |
| tz         | default   | string/num | core  | time zone in minutes (-1440..1440) or "default" or "ask", see below |
| lang       | en        | string  | core     | language code, specifies the user interface's [language](https://gitlab.com/bztsrc/osz/blob/master/docs/translate.en.md) in "xx" or "xx_XX" format |
| debug      | 0         | special | core     | specifies which debug information to show (if [compiled with debugging](https://gitlab.com/bztsrc/osz/blob/master/Config), see below) |
| dmabuf     | 0         | number  | core     | DMA buffer size in pages (allocated in lowest 16M of RAM, defaults to 256 (1M) on ibmpc) |
| quantum    | 1000      | number  | core     | a task can allocate CPU continously for so many microseconds, 1/(quantum) taskswitches per second |
| display    | mc     | flag[,drv] | core     | selects visual, and optionally the driver, see below |
| aslr       | false     | boolean | core     | enable Address Space Layout Randomization |
| syslog     | true      | boolean | core     | disable syslog [service](https://gitlab.com/bztsrc/osz/blob/master/docs/services.en.md) |
| internet   | true      | boolean | core     | disable inet service |
| sound      | true      | boolean | core     | disable sound service |
| print      | true      | boolean | core     | disable print job queue service |
| rescueshell | false    | boolean | core     | if true, starts `/bin/sh` instead of user services |
| spacemode  | false     | boolen  | core     | on unrecoverable error, the OS will automatically reboot |
| pathmax    | 512       | number  | fs       | maximum length of path in bytes, minimum 256 |
| cachelines | 16        | number  | fs       | number of block cache lines, minimum 16 |
| cachelimit | 5      | percentage | fs       | flush and free up block cache if free RAM drops below this limit, 1%-50% |
| keyboard   | pc105,en_us | str,str[,str] | ui | keyboard layout and mapping(s), see [etc/kbd](https://gitlab.com/bztsrc/osz/blob/master/etc/sys/etc/kbd) |
| focusnew   | true      | boolean | ui       | automatically focus new windows |
| dblbuf     | true      | boolean | ui       | use double buffering for window composition (memory consuming but fast) |
| lefthanded | false     | boolean | ui       | swap pointers |
| identity   | false     | boolean | init     | force run of first time setup before servies to get machine's identity |
| hpet       | -         | hexdec  | platform | x86_64-acpi override autodetected HPET address |
| apic       | -         | hexdec  | platform | x86_64-acpi override autodetected LAPIC address |
| ioapic     | -         | hexdec  | platform | x86_64-acpi override autodetected IOAPIC address |

Debugging
---------

This can be a numeric value, or a comma separated list of flags, see [debug.h](https://gitlab.com/bztsrc/osz/blob/master/include/osZ/debug.h).

| Value  | Flag | Define | Description |
| -----: | ---- | ------ | ----------- |
| 0      |      | DBG_NONE | no debug information |
| 1      | pr   | DBG_PROMPT | give a debugger prompt during boot before the first task switch |
| 2      | me   | DBG_MEMMAP | dump memory map on console (also dumped to syslog) |
| 4      | ta   | DBG_TASKS | dump [system services](https://gitlab.com/bztsrc/osz/blob/master/docs/services.en.md) with TCB physical addresses |
| 8      | el   | DBG_ELF | debug [ELF loading](https://gitlab.com/bztsrc/osz/blob/master/src/core/elf.c) |
| 16     | ri   | DBG_RTIMPORT | debug [run-time linker](https://gitlab.com/bztsrc/osz/blob/master/src/core/elf.c) imported values |
| 32     | re   | DBG_RTEXPORT | debug run-time linker exported values |
| 64     | ir   | DBG_IRQ | dump [IRQ Routing Table](https://gitlab.com/bztsrc/osz/blob/master/src/core/drivers.c) |
| 128    | de   | DBG_DEVICES | dump [System Tables](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/acpi/acpi.c) and [PCI devices](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/ibmpc/pci.c) |
| 256    | sc   | DBG_SCHED | debug [scheduler](https://gitlab.com/bztsrc/osz/blob/master/src/core/sched.c) |
| 512    | ms   | DBG_MSG | debug [message sending](https://gitlab.com/bztsrc/osz/blob/master/src/core/msg.c) |
| 1024   | lo   | DBG_LOG | dump [early syslog](https://gitlab.com/bztsrc/osz/blob/master/src/core/syslog.c) |
| 2048   | pm   | DBG_PMM | debug [physical memory manager](https://gitlab.com/bztsrc/osz/blob/master/src/core/pmm.c) |
| 4096   | vm   | DBG_VMM | debug [virtual memory manager](https://gitlab.com/bztsrc/osz/blob/master/src/core/vmm.c) |
| 8192   | ma   | DBG_MALLOC | debug [libc memory allocation](https://gitlab.com/bztsrc/osz/blob/master/src/libc/bztalloc.c) |
| 16384  | bl   | DBG_BLKIO | debug [block level I/O](https://gitlab.com/bztsrc/osz/blob/master/src/fs/vfs.c) |
| 32768  | fi   | DBG_FILEIO | debug [file level I/O](https://gitlab.com/bztsrc/osz/blob/master/src/fs/taskctx.c) |
| 65536  | fs   | DBG_FS | debug [file systems](https://gitlab.com/bztsrc/osz/blob/master/src/fs/main.c) |
| 131072 | ca   | DBG_CACHE | debug [block cache](https://gitlab.com/bztsrc/osz/blob/master/src/fs/cache.c) |
| 262144 | ui   | DBG_UI | debug [user interface](https://gitlab.com/bztsrc/osz/blob/master/src/ui/main.c) |
| 524288 | te   | DBG_TESTS | run [tests](https://gitlab.com/bztsrc/osz/blob/master/src/test) instead of [init](https://gitlab.com/bztsrc/osz/blob/master/src/init) task |

Most of these only available when compiled with [DEBUG = 1](https://gitlab.com/bztsrc/osz/blob/master/Config). Without you can only use two of these to troubleshoot booting: DBG_DEVICES and DBG_LOG.

Display
-------

A numeric value or exactly one flag, which sets the pixel buffer type (visual), and optionally a driver name after a comma.

| Value | Flag  | Define | Description |
| ----: | ----  | ------ | ----------- |
| 0     | mm    | PBT_MONO_MONO | a simple 2D pixelbuffer with 8 bit monochrome pixels |
| 1     | mc    | PBT_MONO_COLOR | a simple 2D pixelbuffer with 32 bit ARGB color pixels |
| 2     | sm,an | PBT_STEREO_MONO | two 2D pixelbuffers*, they are converted grayscale and a red-cyan filtering applied, anaglyph |
| 3     | sc,re | PBT_STEREO_COLOR | two 2D pixelbuffers*, the way of combining left and right eye's view is 100% driver specific, real 3D |

(* the two buffers are concatenated in a one big double heighted buffer)

Device drivers can be found under "sys/drv/display/", example: `display=mc,nvidia` will look for "sys/drv/display/nvidia.so". If
not given, then the first autodetected display driver will be used. If none was found during boot, then fallbacks to a CPU driven
linear framebuffer driver, which is slow, but works. Alternatively you can use a remote driver to force all windows to be sent to
another machine with "remote:(ip)", like `display=mc,remote:192.168.1.1`.

OS/Z supports several displays, the one loaded with the UI task is "0", which is the first local monitor.

Time Zone
---------

Either a numeric value in the range -1440..1440 (offset in minutes) or the strings "default" and "ask". Default means only ask
the user for the current time if date could not be detected (no RTC). On the other hand "ask" prompts for it regardless (because
RTC could be inaccurate). If you use "tz=0", then date and time will be in UTC, and boot will continue even if time is
unknown. In this case you'll see dates "0000-00-00T00:00:00+00:00" in the system log until ntpd sets the correct time.

Low Memory Footprint
--------------------

If you want to have an OS/Z for an embedded system where memory is expensive, you can do the following (considering UI task requires the most memory by far):
1. Turn off double buffering with `dblbuf=false`. This will cut the memory consumption of the compositor in half, but may produce visual artifacts.
2. Use a monochrome visual with low resolution screen. That's a bit slower, but requies only 8 bits per pixel instead of 32, cutting the memory requirement even further in quater.
3. In extreme cases, replace `logind` with a custom application for the job to avoid unnecessary processes and windows.
4. Alternatively use a remote display driver, so that no pixel buffer will be allocated on the embedded system at all.

Performance Tunning
-------------------

If you aim for a fast system, and you have plenty of RAM and SSSE3 (x86_64) or Neon (AArch64) capable processor, do:
1. Use `OPTIMIZE = 1` in [Config](https://gitlab.com/bztsrc/osz/blob/master/Config). This will utilize SIMD whenever possible (requires recompilation).
2. Use double buffering with true color visual, on a card that natively uses ARGB packed pixel format (and not ABGR). In this case a much faster blit implementation is used (and optimized even further with point 1).

