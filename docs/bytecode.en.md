OS/Z Bytecode
=============

A modern operating system must support platform-independent bytecode. After a long and through examination of existing
technologies, I've decided to provide a shell-script like execution with a user space interpreter. As for the bytecode
itself I have examined:

- python: although it seemed a pretty good candidate at first, I dropped because of the non-standardized horrible bytecode format.
- java: another possible candidate, but it's bytecode is way to tied to the Java language, and the existing VMs are bloated
    like hell, and pretty hard to port to a hobby OS. Complete reimplementation is also complicated at least to say.
- lua: it looked nice, with a simple to port ANSI C library. Finally dropped it because of being language-tied, non C-style
    syntax, and also because it lacks clear compiler/interpreter toolchain (the two mixed up badly in a single library).
- wasm: I decided to go on with WebAssembly. It's simple, language agnostic, easy to implement, and considered to be the
    de facto standard in the future by many independent experts. It also already has a minimal C implementation, [wac](https://github.com/kanaka/wac)
    and a fully featured C++ implementation [wavm](https://github.com/WAVM/WAVM), neither of which tied to any browsers thankfully.

The executable's format is checked in [task_execinit](https://gitlab.com/bztsrc/osz/blob/master/src/core/task.c) and the
interpreter is in [usr/wasmvm](https://gitlab.com/bztsrc/osz/blob/master/usr/wasmvm).
