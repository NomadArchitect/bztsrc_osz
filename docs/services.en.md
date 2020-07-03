OS/Z Services
=============

There are two different kind of services: system services and user services. System services
must reside on the initial ramdisk, as they are the server counterparts of `libc`, and they
cannot be controlled from userspace. User services (UNIX daemons) on the other hand are controlled by the
init system service, and they can be loaded from external disks as well.

### Service hierarchy

```
              +----------------------------------------------------------+
EL1 / ring 0  |                       CORE (+IDLE)                       | (supervisor)
--------------+---------+----+----+--------------------------------------+---------------------
EL0 / ring 3  | Drivers | FS | UI | syslog | inet | sound | print | init | (system services)
              +---------------------------------------------------+      |
              | logind | httpd | ...                                     | (user services controlled by init)
              +----------------------------------------------------------+

              +----------------------------------------------------------+
              | identity | sh | sys | fsck | test | ...                  | (normal user applications)
              +----------------------------------------------------------+
```

CORE
----

Special service which runs in privileged supervisor mode. It's always mapped in all address space.
The lowest level code of OS/Z like ISRs, physical memory allocation, task switching, timers and such
are implemented here. Although it looks like a separate task, `IDLE` is part of core as it needs to halt the CPU.
All the other services (including the device driver tasks) are running in non-privileged user mode (EL0 / ring 3).

Typical functions: alarm(), setuid(), setsighandler(), yield(), fork(), execve().

Device Drivers
--------------

Each device driver has it's own task. The MMIO mapped into their bss segment for architectures that support mapping, and
they are allowed to access I/O address space. Only `FS` task and `CORE` allowed to send messages to them. There's one
exception, the display driver, which does not have it's own task, rather it's loaded into `UI` task's address space.

Typical functions: IRQ(), ioctl(), reset(), read(), write().

FS
--

The file system service. It's a big database server, which emulates all layers as files.
OS/Z shares UNIX's and [Plan 9](https://en.wikipedia.org/wiki/Plan_9_from_Bell_Labs)'s famous "everything is a file" approach.
This process has the initrd mapped in it's bss segment, and uses free memory as a disk cache. On boot, the file system
drivers are loaded into it's address space as shared libraries. It has one built-in file system, called "devfs", which connects
the detected devices with the device driver tasks.

Typical functions: printf(), fopen(), fread(), fclose(), mount().

UI
--

User Interface service. It's job is to manage tty consoles, window surfaces and GL contexts. And
when required, composites all of them in a single frame and flushes the result to video memory.
It receives scancodes from input devices and sends keycode messages to the focused application in return. It shares
double screen buffers with the applications that can hold a stereographic composited image. The video frame buffer is
also mapped into its address space. The video card driver for local "0" display is loaded into this task (instead of
it's own task) which accesses that frame buffer directly.

Typical functions: ui_opendisplay(), ui_createwindow(), ui_destroywindow().

Syslog
------

The system logger daemon. Syslog's bss segment is a circular buffer that periodically flushed to disk. On boot,
CORE is gathering it's logs in a temporary buffer (using [syslog_early()](https://gitlab.com/bztsrc/osz/blob/master/src/core/syslog.c) function). The size of the buffer can be
[configured](https://gitlab.com/bztsrc/osz/blob/master/docs/bootopts.en.md) with `nrlogmax=4`. That buffer is shared with the "syslog" task. You can turn logging off with the
boot paramter `syslog=false`.

Typical functions: syslog(), setlogmask().

Inet
----

Service that is responsible for handling interfaces and IP routes, and all other networking stuff. You can disable
networking with the [boot environment](https://gitlab.com/bztsrc/osz/blob/master/docs/bootopts.en.md) variable `networking=false`.

Typical functions: connect(), accept(), bind(), listen().

Sound
-----

Service that mixes several audio channels into a single stream. It opens sound card device excusively, and offers a mixer device
for applications instead. You can disable audio as a whole with the [boot parameter](https://gitlab.com/bztsrc/osz/blob/master/docs/bootopts.en.md) `sound=false`.

Typical functions: speak(), setvolume().

Print
-----

Print job queue service, that formats documents and passes them to the printer device driver. You can turn it off with `print=false`.

Typical functions: print_job_add(), print_job_cancel().

Init
----

This service manages all the other, non-system services. Those include user session daemon, mailer daemon,
print daemon, web server etc. You can command init to start only a rescue shell with the boot environment variable `rescueshell=true`.
Also, if compiled with [DEBUG = 1](https://gitlab.com/bztsrc/osz/blob/master/Config) and if you set `debug=test`
[boot parameter](https://gitlab.com/bztsrc/osz/blob/master/docs/bootopts.en.md), a special [test](https://gitlab.com/bztsrc/osz/blob/master/src/test) process will be started instead
of `init` that will do system tests.

Typical functions: start(), stop(), restart(), status().

