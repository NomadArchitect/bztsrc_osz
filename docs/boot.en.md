OS/Z Boot Process
=================

Loader
------

OS/Z uses the [BOOTBOOT](https://gitlab.com/bztsrc/bootboot) protocol to get the system running.
The compatible [loader](https://gitlab.com/bztsrc/osz/tree/master/loader) is loaded by the firmware as the last step of POST.
On every platform, the loader initializes the hardware (including the framebuffer), loads initrd and locates `sys/core` inside.
When found, it maps that at the top of the address space (-2M..0) and passes control to it.

Core
----

First of all we have to clearify that `core` consist of two parts: one is platform independent, the other part is
architecture and platform specific (see [porting](https://gitlab.com/bztsrc/osz/blob/master/docs/porting.en.md)).
During boot, the execution is jumping forth and back from one part to the other.

The entry point that the loader calls is `_start`, which can be found in [src/core/(arch)/start.S](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/start.S).
It disables interrupts, sets up segments registers, distinguishes CPU cores, and puts APs in a spinloop.
Finally on the BSP jumps to the C startup code `main()` in [src/core/main.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/main.c) which is a platform independent code.

To boot up the computer, `core` does the following:

1. `krpintf_init()` initializes the [boot console](https://gitlab.com/bztsrc/osz/blob/master/src/core/kprintf.c).
2. `platform_dbginit()` initializes the debug console on serial line in [src/core/(arch)/(platform)/platform.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/ibmpc/platform.c).
3. `platform_cpu()` initializes the CPU specific features in [src/core/(arch)/platform.S](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/platform.S).
4. `platform_srand()` initializes the random generator in [src/core/(arch)/platform.S](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/platform.S).
5. `env_init` parses the [environment variables](https://gitlab.com/bztsrc/osz/blob/master/docs/bootopts.en.md) in [src/core/env.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/env.c) passed by the loader.
6. `pmm_init()` sets up Physical Memory Manager in [src/core/pmm.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/pmm.c).
7. `vmm_init()` sets up Virtual Memory Manager and paging in [src/core/(arch)/vmm.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/x86_64/vmm.c). The core here switches to per CPU stacks.
8. `drivers_init()` in [src/core/drivers.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/drivers.c), creates `IDLE` task (the CPU "driver"), initializes the interrupt controller and the IRQ Routing Table (IRT).
9. next loads the first [system service](https://gitlab.com/bztsrc/osz/blob/master/docs/services.en.md) with `service_add(SRV_FS)`, which is a normal system service, except it has the initrd entirely mapped in in it's bss.
10. enumerates drivers database to load the required [device drivers](https://gitlab.com/bztsrc/osz/blob/master/docs/drivers.en.md) with `drivers_add()`.
11. drops supervisor privileges and switches to the FS task, which arranges memory so that it can receive mknod() calls from drivers.
12. then the scheduler, `sched_pick()` in [src/core/sched.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/sched.c) chooses driver tasks one by one. Note that pre-emption is not enabled at this point.
13. driver tasks perform hardware initialization and they fill up the IRT. Optionally drivers might load additional device drivers (typically PCI and ACPI).
14. when all tasks are blocked and the `IDLE` task is scheduled for the first time, a call to `drivers_ready()` in [src/core/drivers.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/drivers.c) is made, which
15. loads user interface with `service_add(SRV_UI)`. These first three services (aka `IDLE`, `FS`, `UI`) are mandatory, unlike the rest (hence the upper-case).
16. loads additional, non-critical tasks by several `service_add()` calls, like the `syslog`, `inet`, `sound`, `print` and `init` system services.
17. enables IRQs with entries in the IRT. It also enables scheduler and wallclock IRQ and with that pre-emption may begin. At this point the `core` has done with booting.
18. as soon as it's scheduled, one of the system services, the `init` task will load and start user session services.

If rescueshell was requested in [environment](https://gitlab.com/bztsrc/osz/blob/master/etc/config), then [bin/sh](https://gitlab.com/bztsrc/osz/blob/master/src/sh/main.c) is loaded instead of `init`.

(NOTE: If OS/Z was compiled with debug support and `debug=tests` passed in [environment](https://gitlab.com/bztsrc/osz/blob/master/etc/config),
then `core` loads [bin/test](https://gitlab.com/bztsrc/osz/blob/master/src/test/main.c) instead of `init`, which will perform various unit and functionality tests on the system.)

User Land
---------

The first real 100% user space process is started by the [init](https://gitlab.com/bztsrc/osz/blob/master/src/init/main.c) system
service. If it is the first run ever, then `bin/identity` is called to query computer's identity (such as hostname and other
configuration) from the user. Finally `init` starts all user services. Unlike system services, user services are classic UNIX
daemons, among others the user session service that provides a login prompt.

User Session
------------

As OS/Z is a multi-user operating system, it needs to authenticate the user. It is done by the `logind` daemon. When
the identity of the user is known, a session can be started by executing a user specific script in the name of that particular user.
As soon as that script ends the session is over, and execution is returned to `logind`, which will display the user login screen again.

This differs from POSIX operating systems, which usually have several ttys (provided by getty), one of which is the graphical
interface (typically tty8, provided by X). In OS/Z there's a graphical interface first (provided by the `UI` task), and you can
start a shell to get a tty (called pseudo-tty in UNIX terminology, usually provided by terminal emulators).

End Game
--------

When the `init` system service quits, the execution is [passed back](https://gitlab.com/bztsrc/osz/blob/master/src/core/msg.c) to
`core`. Then OS/Z says "Good Bye" with `kprintf_reboot()` or with `kprintf_poweroff()` (depending on the exit status), and the
platform dependent part restarts or turns off the computer.
