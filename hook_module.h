#ifndef _HOOK_MODULE_H
#define _HOOK_MODULE_H

#include <trace/syscall.h>
#include <linux/fs.h>
#include <linux/device.h>

/* Module's functions */
unsigned long **find_sys_call_table(void);
static int __init syscall_init(void);
static void __exit syscall_release(void);
void create_hooks(void);
void delete_hooks(void);

int cdev_dev_uevent(struct device *dev, struct kobj_uevent_env *env);
int cdev_open(struct inode *inode, struct file *filp);
int cdev_release(struct inode *inode, struct file *filp);
ssize_t cdev_read(struct file *filp, char __user * buffer,
        size_t len, loff_t *offset);
ssize_t cdev_write(struct file *filp, const char __user *buffer,
        size_t len, loff_t *offset);
int cdev_create(void);
void cdev_destroy(void);
/*
 * Original syscalls pointer
 */

asmlinkage long (*orig_sys_fork)(void);

/*
 * HOOKED syscalls prototypes
 */

asmlinkage long hooked_sys_fork(void);

#endif
