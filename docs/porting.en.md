OS/Z Porting to new platforms
=============================

The Hardware Abstraction Layer is divided into one plus three parts:

 * [loader](https://gitlab.com/bztsrc/osz/tree/master/loader) (not part of the OS, but required)
 * [core](https://gitlab.com/bztsrc/osz/tree/master/src/core)
 * [libc](https://gitlab.com/bztsrc/osz/tree/master/src/libc)
 * [device drivers](https://gitlab.com/bztsrc/osz/blob/master/docs/drivers.en.md)

Porting loader
--------------

In order to boot OS/Z, a [BOOTBOOT](https://gitlab.com/bztsrc/bootboot) compliant loader must exists for
the platform. The loader is not strictly part of the OS, as it can load kernels other than OS/Z `core`, and also from
`core`'s perspective the loader abstracts the boot firmware away with a [common interface](https://gitlab.com/bztsrc/osz/blob/master/loader/bootboot.h).

Porting core
------------

To port OS/Z to a new platform, that hardware must minimally support

 * 64 bits memory addresses with upper and lower half mapping, using 4k based paging tables
 * supervisor and user space execution modes
 * configurable interrupt controller (where IRQs can be enabled and disabled independently)
 * at least two timers (one per CPU timer, and one generic timer)

Note there's a distinction between **architecture** and **platform**. The former only includes the CPU with some essential
features like MMU or FPU (usually built-in the CPU, but not necessarily), while the latter includes everything shipped on
the motherboard or SoC (usually not replaceable by the end user), like interrupt controllers, timers, nvram, PCI(e) bus and
BIOS or UEFI firmware etc.

The `core` is loaded by the `loader`, and it [boots](https://gitlab.com/bztsrc/osz/blob/master/docs/boot.en.md) the
rest of the operating system by initializing lowest level parts, enumerating system buses, loading and initializing
device drivers and [system services](https://gitlab.com/bztsrc/osz/blob/master/docs/services.en.md). Once it's finished
with all of that, it passes control to the `FS` system service.

The bigger part of the `core` is platform independent, and written in C, [src/core](https://gitlab.com/bztsrc/osz/blob/master/src/core).
Hardware specific part is written in C and Assembly, in [src/core/(arch)/(platform)](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64). The
`core` is the only part of OS/Z that runs in supervisor mode (ring 0 on x86_64, and EL1 on AArch64), and it must be kept small.
Currently it's around 100k (out of which 32k being the UNICODE system font), and with
debugger compiled in it's still less than 200k. Under no circumstances should it go beyond 512k.

Although OS/Z is a micro-kernel, the `core` does a little bit more than other typical micro-kernels, like Minix. For example
it does not delegate memory memory management to user space. The goal was performance and also to provide a minimal, but
well-defined interface for each platform. So `core` abstracts the platform in a minimalistic approach: everything that's
required to support individual, pre-empted, inter communicating tasks must be in `core`. Everything else (including device drivers)
must be pushed out to user space in separate binaries, as micro-kernel architecture dictates.

Each and every source file (C and Assembly not with standing) must include one of these header files first:

```
#include <core.h>       /* if the file is totally platform independent */
#include <arch.h>       /* if the file requires architecture dependent parts */
#include <platform.h>   /* if the file requires platform dependent parts */
```

### CPU Management

Quite architecture independent, there's only one struct you have to port in __src/core/(arch)/ccb.h__. Under x86_64 to take
advantage of the hardware, this struct aligns with a TSS segment. On AArch64 it just contains the common fields.

CPU dependent core libc functions are in __src/core/(arch)/libc.S__, most notably the atomic `lockacquire()` and `lockrelease()`
functions. All the others have a common, ANSI C implementation. Exceptions are handled by `fault_intr()`, defined in
__src/core/(arch)/fault.c__.

Basic initialization must be in Assembly, done in __src/core/(arch)/start.S__, which should ultimately call `main()` on the
bootstrap processor. Throughout checks and - if needed - feature specific initializations are done in `platform_cpu()` once the
boot console is ready, see below.

### Virtual Memory Management

You have to port two files in __src/core/(arch)/vmm.h, vmm.c__. The most of the code is written architecture independently, so it's
basicly nothing more than page table defines and initial set up. Although the code uses a PAGESIZE define, there are calculations
that assume the pointer size is 8 bytes (because OS/Z is 64 bits only) and the page frame size is 4096 bytes, so stick to that.
For paging, the virtual address space is divided into 4 parts: task local, shared global, mapped per CPU (supervisor only), mapped
globally (supervisor only), see the common src/core/vmm.h for details.

### Interrupt Controller and Timers

It is defined in __src/core/(arch)/(platform)/intr.c__, and must implement the following functions:

 * intr_init() - to initialize controller and set up timers (see below)
 * intr_enable(irq) - enable an IRQ
 * intr_disable(irq) - disable an IRQ
 * intr_nextsched(nsec) - acknowledge a scheduler interrupt and set up the next
 * clock_ack() - acknowledge a wall clock interrupt

There are two special interrupt handlers, one for the scheduler and one for the wall clock (`sched_intr()` and `clock_intr()`).
First one is a per CPU core one-shot timer, and this is the only interrupt that's allowed on every core simultaneously. The other
should be a periodic interrupt (or you have to set up the next interrupt in clock_ack() but that lowers accuracy). All the other
interrupts must use the common interrupt handler, `drivers_intr()` if they are generated by the controller, and `fault_intr()`
if they are generated by the CPU (divide by zero, segmentation fault, page fault etc.).

### Platform Functions

They are either in __src/core/(arch)/plaform.S__ or in __src/core/(arch)/(platform)/platform.c__, depending on the implementation's
nature (for example platform_srand() uses RDRAND instruction on x86 and goes under arch, while on AArch64 it uses an MMIO device
therefore goes under platform. With platform_awakecpu() it's the other way around, there AArch64 has an instruction and x86_64
needs MMIO). Whether you use Assembly or C for these files is up to you, the build environment autodetects.

 * platform_cpu() - check, detect and initialize CPU specific features
 * platform_srand() - set the random number generator's seed to a reasonably unpredictable value
 * platform_env() - set up defaults for environment variables
 * platform_parse() - parse the environemnt for platform specific values
 * platform_poweroff() - turn off the computer
 * platform_reset() - reboot the computer
 * platform_halt() - stop execution, hang computer
 * platform_idle() - put the CPU in a low power consumption mode until an interrupt arrives
 * platform_awakecpu(id) - awakes CPU from idle cycle
 * platform_dbginit() - initialize the serial debug console if compiled with debug support
 * platform_dbgputc() - send one, 8 bit ASCII character to the debug console, if compiled with debug support
 * platform_waitkey() - wait for a key and return keycode by polling the keyboard controller and the debug console (if compiled
    with debug support). This is used by pause when a whole screen scrolled on the boot console, and also used by the debugger

### Internal Debugger (optional)

The __src/core/(arch)/dbg.c__ and the __src/core/(arch)/disasm.h__ files has to be written. In the beginning of the common dbg.c
you can find the list of functions to be implemented (not much). As for the disassembler, you have to
[create a plain text file](https://gitlab.com/bztsrc/udisasm) with the instruction table of the processor.

Porting Device Drivers
----------------------

Drivers that can't be or worth not be written as separate user space tasks (only interrupt controllers, timers,
CPU and memory management) are implemented in `core`, all the rest have separate device driver tasks.

Device drivers have to be written for each and every platform. Some [drivers are written in C](https://gitlab.com/bztsrc/osz/blob/master/docs/howto3-driver.md)
and can be used on different platforms as-is, but it's most likely they have some Assembly parts and platform specific
implementation in C. To help with that, the build system looks for a [platforms](https://gitlab.com/bztsrc/osz/blob/master/docs/drivers.en.md)
file which lists all platforms that the driver supports. Likewise there's a `devices` file which lists device specifications
that `core` uses during system bus enumeration to detect if any of the supported devices exist on the configuration
it's booting on.

Device driver tasks are running at a [higher priority level](https://gitlab.com/bztsrc/osz/blob/master/docs/scheduler.en.md)
than other user space tasks, and they are allowed to use some driver specific `libc` functions, like
[drv_regirq()](https://gitlab.com/bztsrc/osz/blob/master/src/drivers/include/driver.h). Also they are allowed to access IO space
(which can mean to use specific instructions like inb/outb or accesing MMIO addresses depending which one is supported by the
architecture).

Porting libc
------------

This shared library is called `libc` for historic reasons, in reality this is the platform independent interface
to all OS/Z functions, so should be called `libosz`. It provides library functions for user space services, libraries
and applications as well as for device drivers. It's mostly written in C, but some parts had to be written in Assembly
(like memcpy() for performance, and calling a supervisor mode routine in `core`). It hides all the details of low level
messaging, but just in case also provides a platform independent, [user level abstraction for messaging](https://gitlab.com/bztsrc/osz/blob/master/docs/messages.en.md#low-level-user-library-aka-syscalls).

All programs compiled for OS/Z must be dynamically linked with this `libc`, and must not use other, lower level abstractions
(like syscall/svc interface) directly. At `libc` level, OS/Z is totally platform independent.

Porting System Services
-----------------------

They are platform independent, written in C only, so compiling them on a new platform should be easy and straightforward.

Porting Libraries and Applications
----------------------------------

As mentioned before, `libc` does not only provide the usual C library functions, but also serves as a common interface to
all OS/Z services. This ease the development for new programs, but also makes porting code written for other operating
systems a little bit harder. You have to use `#ifdef _OS_Z_` pre-compiler blocks. There's no way around this as
[OS/Z is not POSIX by design](https://gitlab.com/bztsrc/osz/blob/master/docs/posix.en.md). Although I did my best to make
it similar to POSIX to ease porting, that's still OS/Z's own interface. If you aim for full POSIX compatibility, then you'll
have to use the ported `musl` package, but with that you won't be able to use many OS/Z additions.

OS/Z is designed in a way that at shared library level all applications are binary compatible for a specific
architecture, and source compatible for all architectures and platforms. Therefore libraries should not and applications
must not use any Assembly or platform specific C code. If such a thing is required and cannot be solved with device
drivers, then that code must be placed in a separate library with an interface common on all platforms, and should be
implemented for all the platforms. The pixbuf library in `libui` would be a perfect example for that, which requires a very
effective blitter optimized for every architecture.

Because it is the `libc` level where OS/Z becames platform independent, unit and functionality
[tests](https://gitlab.com/bztsrc/osz/blob/master/src/test) are provided for this level and above, but not for lower
levels. This means you can compile the system tests to a new platform as-is, and if it runs without error, your core and libc
port is correct.

For device drivers, binary distribution is allowed, being open source is not mandatory. For these there has to be a pre-compiled
"(ARCH).so" file.

Porting bytecode
----------------

OS/Z supports execution of platform-independent WASM [bytecode](https://gitlab.com/bztsrc/osz/blob/master/docs/bytecode.en.md).
For that, the [wasi](https://gitlab.com/bztsrc/osz/blob/master/usr/wasi) application has to be ported.
