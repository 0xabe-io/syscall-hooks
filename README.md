# syscall-hooks

An example on how to hook a syscall.

Indexes in the sys_call_table can be found in `/usr/include/x86_64-linux-gnu/asm/unistd{,_64}.h` or `/usr/include/asm/unistd_{32,64}.h` depending on the distro.

A match between the indexes and the functions can be found in `/usr/src/$(uname -r)/arch/x86/syscalls/syscall_{32,64}.tbl`

The declaration of syscall functions can be found in `/usr/src/$(uname -r)/include/linux/syscalls.h`
