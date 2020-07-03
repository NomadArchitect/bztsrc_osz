OS/Z Notable differences to POSIX
=================================

First of all, OS/Z has [it's own, clean POSIXish interface](https://gitlab.com/bztsrc/osz/blob/master/docs/refusr.md), which is
deliberately **not** fully POSIX compliant. Its interface is very similar, but without the complexity and the backward compatiblity
layers, and it is strictly platform independent; extended with the OS/Z's specific features. If you point your compiler's -I to
"include/osZ", then any trivial ISO C source will compile just as-is without problems. If your aim is full POSIX compatibility,
then you have to install the musl package ported to OS/Z: "sys install musl". But remember, with strictly POSIX you'll loose all
the cool OS/Z features listed below.

Include Files
-------------

Most of them are just like in POSIX, but there's a strict restriction that every system service must have exactly one header, so
there's no unistd.h for example, its function declarations moved into either stdlib.h (handled by core) or into stdio.h (provided
by the FS task). But this is insignifanct, since all headers are included in one, so at the end you need to include only one:
```
#include <osZ.h>
```

Limits
------

Most of the defines in [limits.h](https://gitlab.com/bztsrc/osz/blob/master/include/limits.h) are meaningless in OS/Z.
Either because I have used an unlimited algorithm or because the limit is boot time [configurable](https://gitlab.com/bztsrc/osz/blob/master/docs/bootopts.en.md).

Errno
-----

In OS/Z's libc, `errno()` is a function and not a global variable. You can set it with `seterr(errno)`. Because
the system was designed to be limitless as possible, a lot of POSIX [errno values](https://gitlab.com/bztsrc/osz/blob/master/include/osZ/errno.h)
are meaningless and therefore non-existent in OS/Z. Like ENFILE (File table overflow) or EMFILE (Too many open files). Using [FS/Z](https://gitlab.com/bztsrc/osz/blob/master/docs/fs.en.md) you'll
never see EFBIG (File too large) or EMLINK (Too many links) errors either.

It also has new values, like ENOTUNI (Not an union) or ENOTSHM (Not a shared memory buffer) unknown to POSIX.

Straightforwardness
-------------------

The usual POSIX libc provides many functions for exactly the same functionality. OS/Z on the other hand prefers ANSI C's
philosophy, as in provide only one clear way to do a certain job. Therefore there are no open/fopen, write/fwrite, getc/fgetc,
dprintf/fprintf duplications, only one version exists of each function. Where the function deals with a file stream, it's
prefixed with an "f", and expects `fid_t` (and integer index) instead of `FILE *` struct pointer. Also I've standardized the
order of the arguments. Therefore for example writing to a file looks like `size_t fwrite(fid_t f, void *ptr, size_t size)`,
which is definitely not POSIX standard, but makes more sense.

There's no mblen() in stdlib.h, which is insane of POSIX, instead there's an mbstrnlen() in string.h where it really belongs.
Some additions to string.h, for example chr() and ord() functions which converts from UTF-8 to UNICODE forth and back.

All str* functions are UTF-8 aware, and strcasecmp() handles the non ASCII characters and numeral digits correctly too.
For example strcasecmp("ÁÄÉÍŐŠÇÑÆΔДѤ٤٢", "áäéíőšçñæδдѥ42") == 0.

Types of uid_t and gid_t
------------------------

Unlike in POSIX, they are not numeric integers, but both types are uuid_t (16 bytes). Because OS/Z handles access control
lists, files and tasks can have more than one groups, so for example getgid() does not return gid_t, rather a pointer to
a list of group ids, `gid_t *getgid()` (note the asterisk). Access flags are stored in the last byte of the UUID. Because
of this, formatting functions (in sprintf, fprintf, printf etc.) also support %U to print UUIDs.

Access flags
------------

Unlike most UNIX with 'rwx', OS/Z also supports append only and delete access rights, therefore uses 'rwxad', where
'a' implies 'w'. In most cases 'w' and 'x' are mutually exclusive, and 'x' does not require 'r'. Similairly to UNIX,
the 'x' flag is required on directories to allow opendir() and readdir() calls.

Time representation
-------------------

Yet another difference is that OS/Z stores timestamps in microsec (millionth of a second) precision in 64 bits. Therefore
`time_t` stores the microseconds (and not seconds as POSIX expects) passed since the UNIX EPOCH, 1970-01-01 00:00:00
UTC. Also it is not affected by the year 2038 bug, it can represent dates in the range 290501-01-01 BC to 294441-12-31 AD
(1970 +/- 2^63 /1000000/60/60/24/365).

Paths
-----

OS/Z does not store `.` and `..` directories on disk. Instead it will resolve paths with string manipulations, eliminating a
lot of disk access and speed up things. For that it is mandatory to end directory names with the '/' character. It's very
similar how VMS worked. OS/Z also implements per file versioning from FILES-11 (the file system the author used and liked a lot
at his university times).

As an addition, OS/Z also handles a special `...` directory as a joker. It means all sub-directories at that level, and will be
resolved as the first full path that matches. For example, assuming we have '/a/b/c', '/a/d/a' and '/a/e/a' directories, then
'/a/.../a' will return '/a/d/a' only. Unlike in VMS, the three dots only matches one directory level, and does not dive into
their sub-directories recursively (yet).

Another addition, called directory unions. These are symbolic link like constructs (not like Plan9 unions which are overloaded
mounts on the same directory). They have one or more absolute paths (in which the joker directory also allowed). All paths listed
in an union will be checked looking for a full path match. Example: the '/bin/' directory in OS/Z is an union of '/sys/bin/' and
'/usr/.../bin/'. All files under these directories show up in '/bin/'. Therefore OS/Z does not use the classic PATH environment
variable at all, since all executables can be found in '/bin/'.

OS/Z also allows versions (with ';') and file offsets (with '#') in file names.

File hierarchy
--------------

OS/Z deliberately does not follow the [File Hieracrchy Standard](http://www.pathname.com/fhs/). Although main directories
are pretty much the same, but there's no '/sbin/' nor '/proc/', and '/sys/' is just a normal directory containing operating
system's files. The '[/usr/](https://gitlab.com/bztsrc/osz/blob/master/usr/)' directory must have sub-directories named
after the package installed it. Below that the main directory structure repeats (like /usr/(package)/etc/, /usr/(package)/bin/,
/usr/(package)/lib/ etc.) The /mnt/ directory also handled specially, labels for removable storage device partitions are
created automatically under. Read more about the [file hierarchy](https://gitlab.com/bztsrc/osz/blob/master/docs/vfs.en.md).

