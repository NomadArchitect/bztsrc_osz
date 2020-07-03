FS/Z File System
================

It was designed to store unimagineable amounts of data. It's a classic UNIX type file system with inodes and superblock
but without their limitations. There's no limit on number of files or directories in FS/Z (save storage capacity).
Logical sector numbers are stored in 2^128 bits to be future proof. Current implementation
only uses 2^64 bits (can handle drives up to 64 Zettabytes with 4096 bytes logical sector size, and 1 Yottabyte with 64k logical sector size).

Logical sector size can be 2048, 4096, 8192... etc. The suggested size matches the memory page size on the architecture. That is 4096
for x86_64 and aarch64.

The file system is designed to be recoverable by `fsck` as much as possible. A totally screwed up superblock can be reconstructed
by examining the sectors on a disk, looking for inodes of meta data. RAID mirroring is also possible with this file system.

For detailed bit level specification and on disk format see [fsZ.h](https://gitlab.com/bztsrc/osz/blob/master/include/osZ/fsZ.h). For
the higher level abstraction layer, see [VFS](https://gitlab.com/bztsrc/osz/blob/master/docs/vfs.md).

Implementations
---------------
The file system driver is implemented 3 times:
 1. in the [loader](https://gitlab.com/bztsrc/osz/blob/master/loader), which supports GPT and FAT to locate initrd, and FS/Z, cpio, tar, sfs to locate kernel inside the initrd. This is a read-only implementation, that requires defragmented files.
 2. in the [core](https://gitlab.com/bztsrc/osz/blob/master/src/core/fs.c), which is used at early boot stage to load various files (like the FS/Z file system driver) from initrd. This is also a read-only implementation for defragmented files.
 3. as a [file system driver](https://gitlab.com/bztsrc/osz/blob/master/src/drivers/fs/fsz/main.c) for the FS task, which is a fully featured implementation with read/write and fragmanetation support.

FS/Z Super Block
----------------

The super block is the first logical sector of the media and optionally repeated in the last sector. If the superblocks differ, they have
checksums to decide which sector is correct.

Logical Sector Number
---------------------

The size of a logical sector is recorded in the superblock. Sector number is a scalar value up to 2^128 and relative to
the superblock, which therefore resides on LSN 0, regardless whether it's on LBA 0 (whole disk) or any other (partition).
This means you can move the file system image around without the need of relocating sector addresses. Because LSN 0
stores the superblock which is never referenced from any file, so in mappings LSN 0 encodes a sector full of zeros,
allowing spare files and gaps inside files.

File ID
-------

The file id (fid in short) is a logical sector number, which points to a sector with an i-node structure. That can be checked
by checking the sector starting with the magic bytes "FSIN".

Inode
-----

The most important structure of FS/Z. It describes portion of the disk as files, directory entries etc. It's the first
1024 bytes of a logical sector (or 2048 if big inode flag is set in the superblock). It can contain several versions thus
allowing not only easy snapshot recovery, but per file recovery as well.

FS/Z also stores meta information (like mime type and checksum) on the contents, and supports meta labels in inodes.

Contents have their own address space mappings (other than LSNs). To support that, FS/Z has two different LSN translation
mechanisms. The first one is similar to memory mapping, and the other stores variable sized areas, so called extents.

Sector List (sl)
----------------

Used to implement extents and marking bad and free sector areas on disks. A list item contains a starting LSN and the number
of logical sectors in that area, describing varying length continuous areas on disk. Every extent is 32 bytes long.

Sector Directory (sd)
---------------------

A sector that has (logical sector size)/16 entries. The building block of memory paging like translations. As with sector lists,
sector directories can handle LSNs up to 2^128 (or 2^96 if data checksum enabled), but describe fix sized, non-continous areas on
disk. Not to confuse with common directories which are used to assign names to fids.

Directory
---------

Every directory has 128 bytes long entries which gives 2^121-1 as the maximum number of entries in one directory. The first
entry is reserved for the directory header, the others are normal fid and name assignments. Entries are alphabetically ordered
thus allowing fast 0(log n) look ups with `libc`'s [bsearch](https://gitlab.com/bztsrc/osz/blob/master/src/libc/stdlib.c).

16 bytes go for the fid, 1 byte reserved for the number of multibyte characters (not bytes) in the filename. That gives 111 bytes for
filename (maybe less in characters as UTF-8 is a variable length encoding). That's the hardest limitation in FS/Z, but let's
face it most likely you've never seen a filename longer than 42 characters in your whole life. Most filenames are below 16 characters.

