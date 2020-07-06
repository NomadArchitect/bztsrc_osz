OS/Z Memory Management
======================

Address Space
-------------
```
FFFFFFFF_FFFFFFFF +----------+---------------------+---------------------------+
                  |          |                     |  2 M core code + data     |  (loader provided)
                  |          |   1 G CPU global    +---------------------------+  CORE_ADDRESS
                  |          |                     |  1022 M core dynamic data |
FFFFFFFF_C0000000 |   512 G  +---------------------+---------------------------+  CDYN_ADDRESS
                  |  shared  |   1 G CPU local     |  1024 M core dynamic data |
FFFFFFFF_80000000 |          +---------------------+---------------------------+  LDYN_ADDRESS
                  |          |    510 G shared     |  522240 M user data       |
                  |          |   dynamic memory    |         smalloc()      ^  |
FFFFFF80_00000000 +----------+---------------------+---------------------------+  SDYN_ADDRESS
0000007F_FFFFFFFF +----------+---------------------+---------------------------+
                  |          |     4 G mapped      |  4096 M special buffer    |
0000007F_00000000 |          +---------------------+---------------------------+  BUF_ADDRESS
                  |          |     504 G task      |  516096 M user data       |
                  |          |   dynamic memory    |         malloc()       ^  |
00000001_00000000 |          +---------------------+---------------------------+  DYN_ADDRESS
                  |   512 G  |                     |  4090 M user code         |
00000000_00400000 |   local  |                     +---------------------------+  TEXT_ADDRESS
                  |          |                     |  4M stack              v  |
                  |          |       4 G task      +---------------------------+
                  |          |    static memory    |  4M-4K message queue   ^  |
                  |          |                     +---------------------------+  __PAGESIZE
                  |          |                     |  4K Task Control Block    |
00000000_00000000 +----------+---------------------+---------------------------+  TCB_ADDRESS
```

Memory is allocated at several levels:
 - [pmm_alloc()](https://gitlab.com/bztsrc/osz/tree/master/src/core/pmm.c) allocate physical RAM pages (core only)
 - [vmm_map()](https://gitlab.com/bztsrc/osz/tree/master/src/core/vmm.c) allocate virtual pages (core only)
 - [kmalloc()](https://gitlab.com/bztsrc/osz/tree/master/src/libc/bztalloc.c), krealloc(), kfree() allocate core (kernel) dynbss memory
 - [malloc()](https://gitlab.com/bztsrc/osz/tree/master/src/libc/bztalloc.c), realloc(), calloc(), free() allocate dynbss memory
 - [smalloc()](https://gitlab.com/bztsrc/osz/tree/master/src/libc/bztalloc.c), srealloc(), scalloc(), sfree() allocate shared memory

The address space is divided into upper half and lower half. The lower half is task specific, and on a task switch, the whole lower
half is replaced. The upper half is global, the same for all tasks, and except 1G the same for all CPUs.

User Tasks
----------

| Virtual Address  | Scope   | Description |
| ---------------- | ------- | ----------- |
| 2^48-4G .. 2^48  | [process](https://gitlab.com/bztsrc/osz/tree/master/docs/process.en.md) | pre-allocated / mapped system buffers (screen, initrd, MMIO, code loading temporary area etc.) |
|    4G .. 2^48-4G | process | dynamically [allocated tls memory](https://gitlab.com/bztsrc/osz/tree/master/src/libc/bztalloc.c) (growing upwards, read/write) |
|     x .. 4G-1    | process | shared libraries (text read only / data read/write) |
|    4M .. x       | process | user program (text read only / data read/write) |
|    2M .. 4M-1    | task    | local stack (read/write, growing downwards) |
| 16384 .. 2M-1    | task    | Message Queue circular buffer (read-only, growing upwards) |
|  4096 .. 16383   | task    | Message Queue (read-only)) |
|     0 .. 4095    | task    | Task Control Block (supervisor only, NULL references will throw an exception) |
| -512G .. -2G-1   | global  | global [shared memory](https://gitlab.com/bztsrc/osz/tree/master/src/libc/bztalloc.c) space (user accessible, read/write) |

If two mappings are identical save the TCB, message queue and stack, then they belong to the same process.

### UI

User interface has the linear frame buffer mapped at 2^48-4G. It's used when the compositor buffer has to be flushed.

### FS

Filesystem service has the initrd mapped at 2^48-4G. Initrd maximum size is 16M on boot, but it can grow
dynamically in run-time up to 4G. The /dev/root device uses it, implemented in "devfs" as a in-memory disk.

### Drivers

Device drivers have the MMIO and the ACPI tables mapped at 2^48-4G, if that's supported on the platform.

Process Memory
--------------

Shared among tasks.

| Virtual Address | Scope   | Description |
| --------------- | ------- | ----------- |
|    ...          | ...     | more libraries may follow |
|    c ..         | task    | libc's data segment, read-write |
|    b .. c-1     | process | libc shared object's text segment, read-only |
|    a .. b-1     | task    | ELF executable's data segment, read-write |
|   4M .. a-1     | process | ELF executable's text segment, read-only |

The elf's local data segment comes right after the text segment, so it can take advantage of
rip-relative (position independent) addressing. This creates altering process / task mappings.

Core Memory
-----------

The pages for the core are marked as supervisor only, meaning userspace programs cannot access it. They are also globally
mapped to every task's address space, just like shared memory. It has two parts, one is unique to the CPU.

| Virtual Address     | Scope   | Description |
| ------------------- | ------- | ----------- |
|      x .. 0         | cpu     | boot time per CPU core stacks, 1k each (growing downwards) |
|      x .. x         | global  | [free memory entries](https://gitlab.com/bztsrc/osz/tree/master/src/core/pmm.c) (growing upwards) |
|    -2M .. x         | global  | [Core text segment](https://gitlab.com/bztsrc/osz/tree/master/src/core/main.c) |
|   -64M .. -2M-1     | global  | Boot console framebuffer mapping (used by [kprintf](https://gitlab.com/bztsrc/osz/tree/master/src/core/kprintf.c) and kpanic) |
|  -128M .. -64M-1    | global  | MMIO area (used by platform specific code) |
| -1G+4M .. -128M-1   | global  | Dynamic memory (kernel heap, CDYN_ADDRESS) |
|    -1G .. -1G+4M-1  | global  | CPU Control Blocks, one page for each (CCBS_ADDRESS) |
| -2G+4M .. -2G+6M-1  | cpu     | Temporary mapped destination message queue (LBSS_tmpmq) |
| -2G+2M .. -2G+4M-1  | cpu     | Temporary mapped slot (LBSS_tmpslot) |
| -2G+5p .. -2G+6p-1  | cpu     | Temporary mapped page (LBSS_tmpmap1) |
| -2G+4p .. -2G+5p-1  | cpu     | 4K mapping table for this core (-2G..-2G+2M) |
| -2G+3p .. -2G+4p-1  | cpu     | 2M mapping table for this core (-2G..-1G) |
| -2G+2p .. -2G+3p-1  | cpu     | 1G mapping table for this core (-2G..0) |
| -2G+1p .. -2G+2p-1  | cpu     | TCB of next task in alarm queue (LBSS_tcbalarm) |
|    -2G .. -2G+1p-1  | cpu     | CPU Control Block for this core (LBSS_ccb) |

The core dynbss can be allocated by [kalloc()](https://gitlab.com/bztsrc/osz/tree/master/src/core/core.h).

The core uses three separate stacks:
1. boot time stack (1k, -1M..0): set up by the loader, we hardly use it, we just store the paging table root there for the APs
2. per task stack (512b, 3584..4096): used to store the task's state (iretq) in ISRs
3. per cpu stack (3k, -2G+1024..-2G): used by system calls

