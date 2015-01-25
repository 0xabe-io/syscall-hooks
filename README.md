# syscall-hooks

An example on how to hook a syscall.

Indexes in the sys_call_table can be found in `/usr/include/x86_64-linux-gnu/asm/unistd{,_64}.h`

syscall functions can be found in `/usr/src/$(uname -r)/include/linux/syscalls.h`

The source of strace[1] can provide a matching between the indexes and the functions

[1] http://sourceforge.net/projects/strace/
