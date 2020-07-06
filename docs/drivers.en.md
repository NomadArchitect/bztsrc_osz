OS/Z Device Drivers
===================

Supported devices
-----------------

 * VESA 2.0 VBE, GOP, VideoCore (set up by [loader](https://gitlab.com/bztsrc/osz/blob/master/loader), 32 bit frame buffer)
 * x86_64: syscall, NX protection
 * x86_64-ibmpc: [PIC](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/ibmpc/pic.S), [PIT](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/ibmpc/pit.S), [RTC](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/ibmpc/rtc.S)
 * PS2 [keyboard](https://gitlab.com/bztsrc/osz/blob/master/src/drivers/input/ps2_8042/keyboard.h) and [mouse](https://gitlab.com/bztsrc/osz/blob/master/src/drivers/input/ps2_8042/mouse.h)
 * AArch64: SVC, WNX protection
 * aarch64-rpi: [BCM interrupt controller](https://gitlab.com/bztsrc/osz/blob/master/src/core/aarch64/rpi/intr.c), ARM Generic Timers, ARM Local Timer

Planned drivers
---------------

 * x86_64-apic: HPET

Description
-----------

There are two different kind of drivers: software drivers and hardware drivers. Software drivers are loaded into one
of the system service task's address space (like file system drivers or tty and x11 drivers), while hardware drivers
have their own tasks.

Drivers are shared libraries which are loaded into separate address spaces after a common event dispatcher, [driver.c](https://gitlab.com/bztsrc/osz/blob/master/src/drivers/driver.c).
They are allowed to access IO address space with in/out instructions and to map MMIO in their address space. Otherwise driver tasks
are normal userspace applications. You can compile the `core` for a platform with ARCH and PLATFORM variables in [Config](https://gitlab.com/bztsrc/osz/blob/master/Config).
For supported platforms, see [compilation](https://gitlab.com/bztsrc/osz/blob/master/docs/compile.en.md).

### Directories

Device drivers are located in [src/drivers](https://gitlab.com/bztsrc/osz/blob/master/src/drivers), groupped in categories.
Each [driver class](https://gitlab.com/bztsrc/osz/blob/master/src/drivers/README.en.md) has one directory, and
under these directories each driver has exactly one sub-directory. The compiled
driver will be placed in `sys/drv/(class)/(sub-directory).so`. For example, `src/drivers/input/ps2_8042/main.c` will be compiled
to `sys/drv/input/ps2_8042.so`.

Note that for performance, CPU, memory management, timers and interrupt controllers do not have separate drivers, they are built
into [core](https://gitlab.com/bztsrc/osz/tree/master/src/core). To switch among them, you'll have to edit `PLATFORM` and recompile.

| PLATFORM     | Interrupt controller | Scheduler timer | Wallclock timer  | System bus  |
| ------------ | -------------------- | --------------- | ---------------- | ----------- |
| x86_64-ibmpc | PIC                  | PIT             | RTC              | PCI         |
| x86_64-apic  | IOAPIC               | LAPIC Timer     | HPET             | PCI express |
| aarch64-rpi  | BCM Interrupt Ctrl   | ARM CNTP_TVAL   | ARM Local Timer  | -           |

### Files

Among the source files of the driver, there are two special ones:

| Name | Description |
| ---- | ----------- |
| devices | List of device ids, that this driver supports |
| platforms | List of platforms for which this driver should be compiled |

Both files are newline (0x0A) separated list of words. The devices file has the following entries:

| Entry | Description |
| ----- | ----------- |
| *     | Any, means the driver should be loaded regardless to bus enumeration. |
| pciXXXX:XXXX:XXXX:XXXX | A PCI vendor and device id pair with subsystem vendor id and subsystem id |
| pciXXXX:XXXX | A PCI vendor and device id pair, without subsystem identifiers |
| clsXX:XX | A PCI Class definition |
| FS    | File system driver |
| UI    | User interface driver |
| inet  | Networking protocol driver |
| (etc) |  |

These informations along with the driver's path will be gathered to `sys/drivers` by
[tools/elftool.c](https://gitlab.com/bztsrc/osz/blob/master/tools/elftool.c). This database
will be looked up at boot time in order to find out which shared object should be loaded for a
detected device.

Platforms file just list the platforms, like "x86_64-ibmpc" or "aarch64-rpi". Joker can be used, like "x86_64-*". At compilation time
this will be checked, and if the platform it's compiling for not listed there, the driver won't be compiled. This
is an easy way to avoid having platform specific drivers in non-compatible images, yet
you won't have to rewrite multi-platform drivers for every architecture (like an USB mass-storage
driver). If the platforms file is missing, the driver will be compiled on all platforms.

Device drivers are exceptions to licensing, they can be distributed in binary form without source. For that, one has to
put the pre-compiled shared dynamically linkable ELF binary in the directory under the name "(ARCH).so" next to the devices
and platforms files.

### Developing drivers

If you want to write a device driver for OS/Z, [here](https://gitlab.com/bztsrc/osz/blob/master/docs/howto3-driver.md) you can find more information.
