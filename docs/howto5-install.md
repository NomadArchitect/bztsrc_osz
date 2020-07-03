OS/Z - How To Series #5 - Installing
====================================

Preface
-------

In the last tutorial, we've checked what can we do with the [rescue shell](https://gitlab.com/bztsrc/osz/blob/master/docs/howto4-rescueshell.md).
In this episode we'll take a system administator's approach again, and we'll see how to install OS/Z and how to install packages.

System Install
--------------

### From Development Environment

It's quite easy to install it on a hard drive or removable media, like an USB stick or SD card.
Just [download live image](https://gitlab.com/bztsrc/osz/tree/master/bin/) and under Linux use

```sh
dd if=bin/osZ-latest-x86_64-ibmpc.img of=/dev/sdc
```

Where `/dev/sdc` is the device where you want to install (use `dmesg` to figure out). On MacOSX you can list the available devices
with `diskutil list`.

If you don't like command line, then I would recommend [USBImager](https://gitlab.com/bztsrc/usbimager) which is a minimal GUI
application for Windows, MacOSX and Linux (portable executable, no installation needed).

### Inside OS/Z

Once you've booted up OS/Z, you can clone the running system with

```sh
sys clone /dev/disk2
```

Where `/dev/disk2` is the device where you want to install the system.

Package Install
---------------

You can also individually install, upgrade and remove applications.

### Refresh package list

The list of repositories from where packages can be installed are stored in `/etc/repos`.
The meta information for all available packages from all repositories are stored in `/sys/var/packages`.
To refresh that list, issue

```sh
sys update
```

### Upgrade the system

Without second argument, all packages with newer versions (so the whole system) will be upgraded.
When a package name is given as second argument, only that package and it's dependencies are upgraded.

```sh
sys upgrade
sys upgrade (package)
```

### Search for packages

You can search the package meta information database with

```sh
sys search (string)
```

### Install a package

In order to install a package that was not previously on the computer, use

```sh
sys install (package)
```

### Remove a package

If you decide that an application is no longer useful, you can delete it by issuing

```sh
sys remove (package)
```

Next time we'll see how to manage [services](https://gitlab.com/bztsrc/osz/blob/master/docs/howto6-services.md).
