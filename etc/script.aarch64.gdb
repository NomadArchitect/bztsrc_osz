set architecture aarch64
target remote localhost:1234
symbol-file bin/initrd/sys/core
set style enabled off
display/i $pc
