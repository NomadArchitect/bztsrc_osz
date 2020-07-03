OS/Z - How To Series #3 - Developing
====================================

Preface
-------

Last time we've checked how to [debug OS/Z](https://gitlab.com/bztsrc/osz/blob/master/docs/howto2-debug.md) in a virtual machine. Now we'll take a look on how to develop for the system.

The [next turorial](https://gitlab.com/bztsrc/osz/blob/master/docs/howto4-rescueshell.md) is more user than developer oriented as it's about how to use the rescue shell.

Headers
-------

For every application in OS/Z, one header must be included:

```c
#include <osZ.h>
```

This will include all the other headers required to access standard C library. Although I did everything to keep
function call names and arguments from POSIX to ease porting existing software, OS/Z never
intened to be and is [not POSIX compatible](https://gitlab.com/bztsrc/osz/blob/master/docs/posix.md). OS/Z has it's own interface.

Console Applications
--------------------

The mandatory Hello World example is very similar to UNIX like systems, looks like

```c
#include <osZ.h>

public int main(int argc, char **argv)
{
    printf( "Hello World!\n" );
    return 0;
}
```

Windowed Applications
---------------------

The function prototypes and definitions are in ui.h, which is already included with osZ.h. But windowed applications have to be
linked against `libui` adding `-Lui` command line argument to gcc. Their main() function should do the initialization (like opening
a window), then dispatch messages received by `mq_recv()` because windowed applications are event-driven by nature.

```c
#include <osZ.h>

public int main(int argc, char **argv)
{
    /* initialization */
    win_t *win;
    win = ui_openwindow( "pixbuf", 10, 10, 640, 480, 0 );

    ui_setcursor(UIC_progress);
    load_a_lot_of_data();
    ui_setcursor(UIC_default);

    /* event loop */
    while(true) {
        /* get events, block if none */
        msg_t *msg = mq_recv();

        /* what kind of event has just arrived? */
        switch( EVT_FUNC(msg->evt) ) {

            case UI_keyevent:
                ui_keyevent_t *keyevt = msg;
                ...
                break;

            case UI_ptrevent: ...
        }
    }
}
```

The basic UI functionality (like opening a window) is provided by the `libc`. This will give you a raw pixel buffer to draw to, but
nothing more. Functions that deal with the pixel buffer (drawing primitives, buttons, widgets, loading images etc.) are implemented
in `libui`.

Device drivers
--------------

Drivers are shared libraries, and hardware device drivers use a common [event dispatcher](https://gitlab.com/bztsrc/osz/blob/master/src/libc/dispatch.c) mechanism.
Drivers are [categorized](https://gitlab.com/bztsrc/osz/blob/master/src/drivers/README.md), each in it's own sub-directory.
Also they can use some privileged `libc` functions, and a provide messaging interface. Therefore a minimal driver looks like

```c
#include <osZ.h>
#include <driver.h>

public void drv_irq(uint16_t irq)
{
    /* handle your irq here */
}

public void *drv_ioctl(uint64_t fmt,...)
{
    /* implement ioctl calls here */
}

public void drv_init()
{
    /* Initialize your driver here. */
    /* If the device cannot be detected or initialized, exit(EX_UNAVAILABLE). */
    /* Set up interrupt handlers with regirq(irq). */
    /* Set up 1 sec update calls with regtmr(). */
    /* Create device files with mknod() calls (if applicable) here. */
}
```

### Configure

If a device driver requires a configuration, it can't load config files as filesystems are not yet mounted
when its `drv_init()` is executed. Instead it should use the following `libc` functions:

```c
uint64_t env_num(char *key, uint64_t default, uint64_t min, uint64_t max);
bool_t env_bool(char *key, bool_t default);
char *env_str(char *key, char *default);
```

to parse the [environment](https://gitlab.com/bztsrc/osz/blob/master/etc/sys/config) file, mapped by the run-time linker. These
functions will return the appropriate value for the key, or *default* if key not found. Number format can be decimal or
hexadecimal (with the '0x' prefix). Boolean understands 0,1,true,false,enabled,disabled,on,off. The `env_str()` returns a
malloc'd string, which must be freed later by the caller.

Note that you don't have to do any message receive calls, that's taken care for you by the event dispatcher. Just create public
functions for your message types.

Additionaly the build system looks for [two more files](https://gitlab.com/bztsrc/osz/blob/master/docs/drivers.md#files) for drivers:

 * `platforms` - if this file exists, it can limit the compilation of the driver for specific platform(s)
 * `devices` - specifies which devices are supported by the driver, used by `core` during system bus enumeration.

Services
--------

Services (just like device drivers) are special applications, which use an event dispatcher and not crt0, therefore their
`init()` is mostly used to initialize their structures (and/or devices), and may do an infinite `mq_recv()` loop. On error,
they must exit() during the initialization with the codes listed in [sysexits.h](https://gitlab.com/bztsrc/osz/blob/master/include/osz/sysexits.h).

```c
#include <osZ.h>

public void init()
{
    /* Initialize your service here. exit() on error. */

    mq_dispatch();
}
```

And implement one function for each message type. The elftool will generate a message code include file automatically. Or if you
want more control over how to handle messages:

```c
#include <osZ.h>

public void init()
{
    /* Initialize your service here. exit() on error. */

    while(true) {
        msg_t *msg = mq_recv();
        /* do work */
    }
}
```

### Configuration

[System services](https://gitlab.com/bztsrc/osz/blob/master/docs/services.md) are allowed to use the `env_` environment
interface, and they can also use files (once they have received the SYS_ack from the FS task). User services on the other
hand must load their configuration from files only (stored under /usr/(package)/etc/).

Entry points
------------

### Applications

They start at label `_start` defined in [libc/(platform)/crt0.S](https://gitlab.com/bztsrc/osz/blob/master/src/libc/x86_64/crt0.S).

### Shared libraries

The entry point is called `init`. This function should initialize the library, and return immedately. When an executable loads
several shared libraries, their `init` will be called one by one before the main executable's `_start` gets called. Therefore
if a library `init` hangs (for example blocks waiting for a system call), the execution of the main function is postponed.

### Drivers

Device driver's entry point is `drv_init`, which must not call `mq_dispatch()` nor `mq_recv()` in a loop.

### Services

Service's entry point is also called `init`, which usually calls `mq_dispatch()`. The service must initialize its structures, and
may call `mq_recv()` in an infinite loop.

Application packages
--------------------

Once you have compiled your application, you probably want to deploy it to OS/Z machines. For that, you have to create an
archive file (cpio or tar) of it's files. Use only [relative paths](https://gitlab.com/bztsrc/osz/blob/master/docs/vfs.md#usr) in it,
like 'bin/', 'etc/', 'share/', etc. Copy the archive to the remote repository webserver, and add it's meta information to
the remote 'packages' file. Done! All machines that have your webserver configured as a repository will be able to install
your package. Your application will be unpacked to '/usr/(package)/'. For more information, see [userspace](https://gitlab.com/bztsrc/osz/blob/master/usr/README.md)
and [installation howto](https://gitlab.com/bztsrc/osz/blob/master/docs/howto5-install.md#package-install).

The [next turorial](https://gitlab.com/bztsrc/osz/blob/master/docs/howto4-rescueshell.md) is about how to use the rescue shell.
