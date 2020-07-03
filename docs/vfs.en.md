Virtual File System
===================

For it's native file system, OS/Z uses [FS/Z](https://gitlab.com/bztsrc/osz/blob/master/docs/fs.en.md). The root
filesystem on initrd must be FS/Z, but other mount points can use other file systems as well, for example FAT or ISO9660.

Fully Qualified File Path
-------------------------

A fully qualified path looks like:

```
 (drive):(directory)/(filename);(version)#(offset)
```

Drive specifies the media where the file system is stored. It's optional, and fallbacks to the root file system.
Drives will be translated to `/dev/(drive)/` where automount will take care of the rest. It is useful for removable media,
like `dvd0:`. These drive names cannot be mixed with network protocols (http, ftp, ssh etc.), but that's okay since it's not
the case. Even if a drive and a network protocol happen to use the same name, drive names always end with a number, while protocols
never.

Directory part is a list of directories, separated by '/' (see hierarchy below). A special joker directory name '...'
can be used to dive into every sub-directories. In that case the first full path that matched will be used. Directories
can be unions, a special construct [unknown to UNIX](https://gitlab.com/bztsrc/osz/blob/master/docs/posix.en.md)es. It
works like the joker, but limits the match to the listed directories.

Filename can be 111 bytes in length (up to 27-111 in characters), and may consist of any characters save control characters
(<32), delete (127), separators (device ':', directory '/', version ';', offset '#'), as well as invalid UTF-8 sequences.
Any other UTF-8 encoded UNICODE character up to 4 bytes are allowed. (I know many FS allows to use any character in the filename,
but that is just wrong, and results in unhappy (or even angry) users when they forgot to escape properly. Imagine a file named
"/bin" in the root folder for example. I'd rather deny the use of a few symbols.)

Version part by default is ';0' which points to the current version of the file. The version ';-1' refers to the version
before the current one, ';-2' the version before that and so forth up to 5 pervious versions.

Offset part defaults to '#0' which is the beginning of the file.
Positive numbers are offsets into the file, while negative ones are relative offsets from the end of the file.

File Hierarchy
--------------

| Path   | Location | Description |
| ------ | -------- | ----------- |
| /bin/  | initrd  | Union of all directories in the system that contain executables |
| /etc/  | initrd  | Union of all system wide configuration directories |
| /lib/  | initrd  | Union of all directories that contain shared libraries |
| /root/ | initrd  | Home directory for the root user (must work without further mounts) |
| /sys/  | initrd  | System files |
| /tmp/  | mounted | Temporary files, mount point for a in-memory file system |
| /dev/  | mounted | Mount point for devfs (mount point hardcoded) |
| /home/ | mounted | Mount point for user data |
| /usr/  | mounted | Mount point for [user applications](https://gitlab.com/bztsrc/osz/blob/master/usr) |
| /var/  | mounted | Mount point for variable application data |
| /mnt/  | mounted | Directory for removable media's mount points |

### /bin
**Purpose**
Executable binaries. Union of '/sys/bin/' (system utilities) and '/usr/.../bin/' (executables installed as
userspace programs, each in it's own directory).

**Special files**
The shell '/sys/bin/sh' and the utility to interact with the system '/sys/bin/sys' must exists. Also all
programs that are required to create, mount and repair other file systems must be here.

### /etc
**Purpose**
Static configuration files. Union of '/sys/etc/' (system configuration files) and '/usr/.../etc/' (user
program's configuration files).

**Special files**
The current OS version is stored in '/sys/etc/os-release'. The name of the machine must be in '/usr/sys/etc/hostname'.

### /lib
**Purpose**
Available libraries. Union of '/sys/lib/' (essential system libraries) and '/usr/.../lib/' (user program libraries).

**Special files**
At least '/sys/lib/crt0.o' and '/sys/lib/libc.so'. The POSIX compatibility layer library is in '/usr/musl/lib/libmusl.so',
if it's installed.

### /root
**Purpose**
Root user's home directory. Must work if /home cannot be mounted, and it must be resetted on system reboot. Normally
root user never logs in directly, this is for emergency cases.

### /sys
**Purpose**
System files. All the files that belong the operating system and needed on boot. Other files (not needed on boot)
are stored in '/usr/sys' which is a mounted directory.

**Special files**
The supervisor is stored in '/sys/core', as well as the other system services (like '/sys/fs', '/sys/ui' etc.).
Executables (at least shell and system command) stored in '/sys/bin/sh' and '/sys/bin/sys'. Device drivers are stored
in '/sys/drv/'. Device id driver mappings database is in '/sys/drivers'. Other system configuration files in '/sys/etc'.

### /tmp
**Purpose**
Mount point for an in-memory file system, used to create temporary files by user programs.

### /dev
**Purpose**
Mount point for the built-in devfs, provided by the FS task. Device drivers and services can create special files
in it with mknod() calls.

### /home
**Purpose**
User home directories. All files that are interest to a specific user must be stored here.

**Special files**
First level is a directory named after the user's login name, '/home/(user)/', eg. '/home/bzt/'. For large systems
more levels may exists, grouping users, eg.: '/home/devdepartment/bzt/'. Applications are NOT allowed to create
user specific configuration files in the home directory. Those must be placed in '/home/(user)/.etc/(application)/'.

### /usr
[User space](https://gitlab.com/bztsrc/osz/blob/master/usr) applications, libraries and services. Originally
cames from the abbreviation of UNIX Shared Recources, but also stands for userland.

**Special files**
First level MUST BE a package name. Similar to FHS' /opt and /usr/X11R6. Sub-directories can be:

/usr/(package)/bin/ - executables and scripts (if any)

/usr/(package)/lib/ - shared libraries (if any)

/usr/(package)/include/ - C header files for the libraries provided (if any)

/usr/(package)/etc/ - configuration files (if any)

/usr/(package)/share/ - static files (if any)

/usr/(package)/src/ - source files (if any)

The package name 'sys' and the directory '/usr/sys/' is reserved for system files that are not required for booting.

### /var
**Purpose**
Variable data for user programs.

**Special files**
First level MUST BE a package name. Similar to FHS' /srv and /var/lib. Sub-directories are unspecified, but suggested
directories are:

/var/(package)/cache - generated cache files (if any)

/var/(package)/log - generated log files (if any)

### /mnt
**Purpose**
Directory containing mount points directories for removable media. The FS task will generate it's sub-directories according
to the attached media's labels. If a removable media has a GPT, the partition names are used as labels, if not, they
will be either read from the fs' superblock or automatically generated. Similar to MacOSX's /Volumes.

Mounts
------

You can refer to a piece of data on the disk as a directory or as a file. Directories are always terminated by '/'. This is
also the case if the directory is a mount point for a block device. There refering to as a file (no trailing '/') would
allow to fread() and fwrite() the storage at block level, whilst referencing with a terminating '/' would allow to use opendir()
and readdir(), which would return the root directory of the file system stored on that device.

Examples, all pointing to the same directory, if boot partition (the first bootable partition of the first bootable disk,
FS0: in EFI) is mounted on the root volume at /boot/:

```
 /boot/EFI/
 root:/boot/EFI/
 boot:/EFI/
 /dev/boot/EFI/
```
```
 /dev/boot       // sees as a block device, you can use fread() and fwrite()
 /dev/boot/      // root directory of the file system on the device, use with opendir(), readdir()
```

It worth mentioning this allows automounting just by referring to a file on the device.

