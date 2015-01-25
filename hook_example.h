#ifndef _HOOK_EXAMPLE_H
#define _HOOK_EXAMPLE_H

#include <trace/syscall.h>

/* Module's functions */
unsigned long **find_sys_call_table(void);
static int __init syscall_init(void);
static void __exit syscall_release(void);
void create_hooks(void);
void delete_hooks(void);

/*
 * Original syscalls pointer
 */

asmlinkage long (*orig_sys_fork)(void);

/*
 * HOOKED syscalls prototypes
 */

asmlinkage long hooked_sys_fork(void);

#endif
