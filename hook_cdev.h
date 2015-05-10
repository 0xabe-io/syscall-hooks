#ifndef _HOOK_CDEV_H
#define _HOOK_CDEV_H
#include <linux/fs.h>
#include <linux/device.h>
int hook_dev_uevent(struct device *dev, struct kobj_uevent_env *env);
int hook_open(struct inode *inode, struct file *filp);
int hook_release(struct inode *inode, struct file *filp);
ssize_t hook_read(struct file *filp, char __user * buffer,
        size_t len, loff_t *offset);
ssize_t hook_write(struct file *filp, const char __user *buffer,
        size_t len, loff_t *offset);
int hook_cdev_create(struct device * device);
void hook_cdev_destroy(void);
#endif
