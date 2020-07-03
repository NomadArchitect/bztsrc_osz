OS/Z Processes and Tasks
========================

Object format
-------------

OS/Z uses ELF64 format to store executables on disk. The 7th byte of the ELF file, the EI_OSABI is
```
#define ELFOSABI_OSZ            5
```
The ELF files must have their entire Program Headers in the first 4k (or whatever the __PAGESIZE is on the architecture),
and the first three segments in it must be in order: text, data+bss and dynamic linkage.

Processes
---------

Each process has a different [memory mapping](https://gitlab.com/bztsrc/osz/tree/master/docs/memory.en.md).
They differ in their code segment, which holds their code and the required shared libraries. They differ in their bss segment
as well, only the shared memory is globally mapped.

Whenever the [scheduler](https://gitlab.com/bztsrc/osz/blob/master/src/core/sched.c) chooses a task to execute that belongs to a different process than the process of the
interrupted task, the whole TLB cache is flushed and the whole memory mapping is reloaded.

Each process has at least one task. When the last task exists, the process is terminated.

Tasks
-----

Tasks of the same process share almost exactly the same memory mapping. They only differ in the first two page tables, the
Task Control Block, the [message queue](https://gitlab.com/bztsrc/osz/blob/master/docs/messages.en.md) and the stack.
Core has an optimized method for switching between tasks (when the text segment of the current and the new task are the same). This time only the first
two pageslots (4M) is invalidated, the TLB cache is not flushed which gives much better performance.

This concept is closer to Solaris' Light Weight Processes than the well known pthread library where the
kernel does not know about threads at all. Note that in OS/Z pid_t points to a task, so userpace programs
see each task as a separate process, meaning real processes are totally transparent to them.

