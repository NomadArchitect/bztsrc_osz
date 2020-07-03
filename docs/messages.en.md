OS/Z Message Queues
===================

Being a [micro-kernel](https://en.wikipedia.org/wiki/Microkernel) architecture, OS/Z separates tasks into user space. They cannot
see each other's memory, and unlike with monolithic design, the kernel does not contain any device driver code which could crash.
The main Inter Process Communication form of those tasks are message queues, supervised by the core. For convinience, tasks are
communicating with the core through the same message queue interface, by setting the destination to SYS_CORE, although core does
not have a message queue of it's own.

User level library
------------------

Normally you won't see a thing about message queues. The `libc` library hides all the details from you, so you just
use printf() and fopen() etc. as usual. For [security](https://gitlab.com/bztsrc/osz/blob/master/docs/security.en.md) reasons, you cannot use message queues directly, and you should
not use the syscall/svc interface. The `libc` functions guarantee the [portability](https://gitlab.com/bztsrc/osz/blob/master/docs/porting.en.md) of applications.

But if there's really a need to use the messaging system (you are implementing a new message protocol for example), then you have
platform independent `libc` functions for that.

### Low level user Library (aka syscalls)

There are only five functions for messaging, variations on synchronisation. They are provided by `libc`, and defined in [syscall.h](https://gitlab.com/bztsrc/osz/blob/master/include/osZ/syscall.h).
You should not use them directly (unless you're implementing your own message protocol), use one of the higher level functions instead.

```c
/* async, send a message and return it's serial (non-blocking) */
mq_send0(dst, func);
mq_send1(dst, func, arg0);
mq_send2(dst, func, arg0, arg1);
 ...
uint64_t mq_send(
    uint64_t arg0,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3,
    uint64_t arg4,
    uint64_t arg5,
    evt_t event);

/* sync, send a message and receive result (blocking) */
mq_call0(dst, func);
mq_call1(dst, func, arg0);
mq_call2(dst, func, arg0, arg1);
 ...
msg_t *mq_call(
    uint64_t arg0,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3,
    uint64_t arg4,
    uint64_t arg5,
    pid_t dst,
    uint64_t func);

/* async, is there a message? returns serial or 0 (non-blocking) */
uint64_t mq_ismsg();

/* sync, wait until there's a message (blocking) */
msg_t *mq_recv();

/* sync, dispatch events (blocking, noreturn) */
void mq_dispatch();
```
While [mq_call()](https://gitlab.com/bztsrc/osz/blob/master/src/libc/x86_64/stdlib.S) is a sequence of mq_send() and mq_recv() calls, [mq_dispatch()](https://gitlab.com/bztsrc/osz/blob/master/src/lib/libc/dispatch.c) is quite the opposite: it first
receives a message with mq_recv(), calls the handler for it, and then uses mq_send() to send the result back. For msg_t struct, see below.

You can also pass a system service number as `dst`, they can be found in [syscall.h](https://gitlab.com/bztsrc/osz/blob/master/include/osZ/syscall.h). The available function codes
are defined in the corresponding header file under [include/osZ/bits](https://gitlab.com/bztsrc/osz/blob/master/include/osZ/bits), all included in this header. For convinience,
there are mq_sendX() and mq_callX() macros.

Supervisor Mode (EL1 / Ring 0)
------------------------------

Core can't use libc, it has it's own message queue implementation. Three functions in [src/core/msg.c](https://gitlab.com/bztsrc/osz/blob/master/src/core/msg.c):

```c
msg_t *msg_recv(pid_t pid);

bool_t msg_send(
    uint64_t arg0, /* ptr */
    uint64_t arg1, /* size */
    uint64_t arg2, /* type */
    uint64_t arg3,
    uint64_t arg4,
    uint64_t arg5,
    evt_t event,
    uint64_t serial);

bool_t msg_core(
    uint64_t arg0, /* ptr */
    uint64_t arg1, /* size */
    uint64_t arg2, /* type */
    uint64_t arg3,
    uint64_t arg4,
    uint64_t arg5,
    evt_t event);
```

As `core` never blocks, so they are all async. Also they use (dst<<16 | func) as event. If it also has EVT_MSGPTR flag, then
arg0 is a virtual memory pointer, arg1 is the buffer size, and arg2 is the buffer type.

The third version handles messages sent specifically to the `core` (like SYS_exit or SYS_alarm events) efficiently.

### Low Level

From userspace (EL0 / Ring 3) the message queue routines can be accessed via `syscall` instruction on x86_64, and `svc` instruction
on AArch64. The low level user library builds on those (basically mq_send() and mq_recv() and all the other libc functions are just
wrappers). The destination and function is passed in %rax/x6. When destination is SRV_CORE (0) and function is SYS_recv (0),
it receives messages. Any other value sends.

The arguments are stored and read in System V ABI way, with one exception on x86_64: %rcx is clobbered by the syscall instruction,
so the 4th argument must be passed in %r10.
```
x6 / %rax= (pid_t task) << 16 | function,
x0 / %rdi= arg0/ptr
x1 / %rsi= arg1/size
x2 / %rdx= arg2/magic
x3 / %rbx= arg3
x4 / %r8 = arg4
x5 / %r9 = arg5
svc / syscall
```
Important that you never use this syscall (or svc) instruction from your code, you have to use `mq_send()`, `mq_call()` and the
others instead for portability reasons. Or even better, use higher level functions in `libc`, like printf() or fopen() whenever
possible.

Structures
----------

It's a so essential type that it's defined in [types.h](https://gitlab.com/bztsrc/osz/blob/master/include/osZ/types.h).

```c
msg_t *mq;
```

The first item in the array is the queue header. Because the exact implementation of msghdr_t is irrelevant to the queue's user, it scope is core only and defined
in [src/core/msg.h](https://gitlab.com/bztsrc/osz/blob/master/src/core/msg.h).

```c
msghdr = (msghdr_t *)mq[0];
```

The other items (starting from 1) form a circular buffer for a FIFO queue.

