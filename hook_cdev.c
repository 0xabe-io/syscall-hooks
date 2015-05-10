/*
 * TODO: add a param to set the GID of the char device
 */
#include "hook_cdev.h"
#include <linux/kdev_t.h>
#include <asm/uaccess.h>
#define DEVICE_NAME "hook"
#define CLASS_NAME "hook"

struct file_operations fops = {
  .open = hook_open,
  .release = hook_release,
  .read = hook_read,
  .write = hook_write,
};

char * msg = "Hello\nworld!\n\0";
short msg_size = 14;
int HOOK_MAJOR;
struct class * class;
struct device * device;

int hook_dev_uevent(struct device *dev, struct kobj_uevent_env *env)
{
   add_uevent_var(env, "DEVMODE=%#o", 0660);
   add_uevent_var(env, "DEVGID=%d", 1000);
   return 0;
}
int hook_cdev_create(struct device * dev)
{
  device = dev;
  printk(KERN_DEBUG "[HOOK] [cdev] initializing...\n");
  HOOK_MAJOR = register_chrdev(0, DEVICE_NAME, &fops);
  if (HOOK_MAJOR < 0)
  {
    printk(KERN_DEBUG "[HOOK] [cdev] failed to register!\n");
    return HOOK_MAJOR;
  }
  printk(KERN_DEBUG "[HOOK] [cdev] registered with major number: %d\n",
      HOOK_MAJOR);
  class = class_create(THIS_MODULE, CLASS_NAME);
  if (IS_ERR(class))
  {
    unregister_chrdev(HOOK_MAJOR, DEVICE_NAME);
    printk(KERN_DEBUG "[HOOK] [cdev] failed to create class!\n");
    return PTR_ERR(class);
  }
  class->dev_uevent = hook_dev_uevent;
  printk(KERN_DEBUG "[HOOK] [cdev] created class\n");
  device = device_create(class, NULL,
      MKDEV(HOOK_MAJOR,0), NULL, "hook");
  if (IS_ERR(device))
  {
    class_destroy(class);
    unregister_chrdev(HOOK_MAJOR, DEVICE_NAME);
    printk(KERN_DEBUG "[HOOK] [cdev] failed to create device!\n");
    return PTR_ERR(device);
  }
  printk(KERN_DEBUG "[HOOK] [cdev] device created successfully!\n");
  return 0;
}

void hook_cdev_destroy(void)
{
  device_destroy(class, MKDEV(HOOK_MAJOR, 0));
  class_unregister(class);
  class_destroy(class);
  unregister_chrdev(HOOK_MAJOR, DEVICE_NAME);
  printk(KERN_DEBUG "[HOOK] [cdev] destroyed done\n");
}


int hook_open(struct inode *inode, struct file *filp)
{
  printk(KERN_DEBUG "[HOOK] [cdev] successfuly opened\n");
  return 0;
}
int hook_release(struct inode *inode, struct file *filp)
{
  printk(KERN_DEBUG "[HOOK] [cdev] successfuly released\n");
  return 0;
}
ssize_t hook_read(struct file *filp, char __user * buffer,
    size_t len, loff_t *offset)
{
  int err;
  err = copy_to_user(buffer, msg, msg_size);
  if (err == 0)
  {
    printk(KERN_DEBUG "[HOOK] [cdev] copied %d characters to userspace\n",
        msg_size);
    return msg_size;
  }
  else
  {
    printk(KERN_DEBUG "[HOOK] [cdev] failed to copy to userspace\n");
    return -EFAULT;
  }

}
ssize_t hook_write(struct file *filp, const char __user *buffer,
    size_t len, loff_t *offset)
{
  printk(KERN_DEBUG "[HOOK] [cdev] received from userspace: %s\n", buffer);
  return len;
}
