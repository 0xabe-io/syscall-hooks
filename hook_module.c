/* 
 * hook_example.c: Example of hook a syscall function.
 * Tested on Ubuntu 14.04 with a 3.13 kernel
 */
#include "hook_module.h"
#include <linux/module.h>   /* Needed by all modules */
#include <asm/cacheflush.h> /* Needed by set_memory_rw */
#include <linux/syscalls.h> /* define the syscall funtions */
#include <linux/delay.h>    /* Needed by loops_per_jiffy */
#include <linux/kdev_t.h>
#include <asm/uaccess.h>

#define DEVICE_NAME "hook"
#define CLASS_NAME "hook"
#define CR0_WP 0x00010000   /* Write-Protect Bit (CR0:16) */

/* 
 * Avoid tainting the kernel, however the kernel will still be tainted if the
 * module is not signed
 */
MODULE_LICENSE("GPL");

void **syscall_table;
struct device * hook_cdev;
struct file_operations fops = {
  .open = cdev_open,
  .release = cdev_release,
  .read = cdev_read,
  .write = cdev_write,
};

char * msg = "Hello\nworld!\n\0";
short msg_size = 14;
int CDEV_MAJOR, cdev_gid=1000;
struct class * class;
struct device * cdev;

module_param(cdev_gid, int, 0);
MODULE_PARM_DESC(cdev_gid, "The gid of the character device");

/*
 * @desc find the address of sys_call_table
 * @param void
 * @return unsigned long - the address or NULL
 */
unsigned long **find_sys_call_table()
{
  
  unsigned long ptr;
  unsigned long *p;
  /* the sys_call_table can be found between the addresses of sys_close and
   * loops_pre_jiffy. Look at /boot/System.map or /proc/kallsyms to see if it
   * is the case for your kernel */
  for (ptr = (unsigned long)sys_close;
      ptr < (unsigned long)&loops_per_jiffy;
      ptr += sizeof(void *)) {
         
    p = (unsigned long *)ptr;

    /* Indexes such as __NR_close can be found in
     * /usr/include/x86_64-linux-gnu/asm/unistd{,_64}.h
     * syscalls function can be found in
     * /usr/src/`uname -r`/include/linux/syscalls.h */
    if (p[__NR_close] == (unsigned long)sys_close) {
      /* the address of the sys_close function is equal to the one found
       * in the sys_call_table */
      printk(KERN_DEBUG "[HOOK] Found the sys_call_table!!!\n");
      return (unsigned long **)p;
    }
  }
  
  return NULL;
}
/*
 * @desc call the original sys_fork function
 * @param void
 * @return PID of the child process
 */
long hooked_sys_fork(void)
{
  long ret;
  /* call the original sys_fork */
  ret = orig_sys_fork();
  printk(KERN_DEBUG "[HOOK] sys_fork called, return %ld\n", ret);

  return ret;
}

/*
 * @desc initialize and start the module
 * @param void
 * @return int - 0
 */
static int __init syscall_init(void)
{
  int ret;
  unsigned long addr;
  unsigned long cr0;
  
  /* get the sys_call_table address */
  syscall_table = (void **)find_sys_call_table();

  if (!syscall_table) {
    printk(KERN_DEBUG "[HOOK] Cannot find the system call address\n"); 
    return -1;
  }
  /* get the value of the CR0 register */
  cr0 = read_cr0();
  /* disable the Write-Protect bit */
  write_cr0(cr0 & ~CR0_WP);

  /* set the memory covered by the sys_call_table writable */
  addr = (unsigned long)syscall_table;
  ret = set_memory_rw(PAGE_ALIGN(addr) - PAGE_SIZE, 3);
  if(ret) {
    printk(KERN_DEBUG 
           "[HOOK] Cannot set the memory to rw (%d) at addr %16lX\n",
           ret, PAGE_ALIGN(addr) - PAGE_SIZE);
  } else {
    printk(KERN_DEBUG "[HOOK] 3 pages set to rw\n");
  }

  create_hooks();

  /* restore the Write-Protect bit */
  write_cr0(cr0);

  cdev_create();
  return 0;
}

/*
 * @desc restore the hooks and release the module
 * @param void
 * @return void
 */
static void __exit syscall_release(void)
{
  unsigned long cr0;

  cdev_destroy();
  /* get the value of the CR0 register */
  cr0 = read_cr0();
  /* disable the Write-Protect bit */
  write_cr0(cr0 & ~CR0_WP);
  
  delete_hooks();

  /* restore the Write-Protect bit */
  write_cr0(cr0);
  printk(KERN_DEBUG "[HOOK] released module\n");
}

/*
 * @desc replace the addresses of the syscall functions by custom ones
 * @param void
 * @return void
 */
void create_hooks(void)
{
  /* save the original address of the syscall, we need to restore it when the
   * module is unregistered */
  orig_sys_fork = syscall_table[__NR_fork];
  syscall_table[__NR_fork] = hooked_sys_fork;
}

/*
 * @desc restore the original addresses of the syscall functions
 * @param void
 * @return void
 */
void delete_hooks(void)
{
  syscall_table[__NR_fork] = orig_sys_fork;
}

int hook_dev_uevent(struct device *dev, struct kobj_uevent_env *env)
{
   add_uevent_var(env, "DEVMODE=%#o", 0660);
   add_uevent_var(env, "DEVGID=%d", cdev_gid);
   return 0;
}

int cdev_create(void)
{
  printk(KERN_DEBUG "[HOOK] [cdev] initializing...\n");
  CDEV_MAJOR = register_chrdev(0, DEVICE_NAME, &fops);
  if (CDEV_MAJOR < 0)
  {
    printk(KERN_DEBUG "[HOOK] [cdev] failed to register!\n");
    return CDEV_MAJOR;
  }
  printk(KERN_DEBUG "[HOOK] [cdev] registered with major number: %d\n",
      CDEV_MAJOR);
  class = class_create(THIS_MODULE, CLASS_NAME);
  if (IS_ERR(class))
  {
    unregister_chrdev(CDEV_MAJOR, DEVICE_NAME);
    printk(KERN_DEBUG "[HOOK] [cdev] failed to create class!\n");
    return PTR_ERR(class);
  }
  class->dev_uevent = hook_dev_uevent;
  printk(KERN_DEBUG "[HOOK] [cdev] created class\n");
  cdev = device_create(class, NULL,
      MKDEV(CDEV_MAJOR,0), NULL, "hook");
  if (IS_ERR(cdev))
  {
    class_destroy(class);
    unregister_chrdev(CDEV_MAJOR, DEVICE_NAME);
    printk(KERN_DEBUG "[HOOK] [cdev] failed to create device!\n");
    return PTR_ERR(cdev);
  }
  printk(KERN_DEBUG "[HOOK] [cdev] device created successfully!\n");
  return 0;
}

void cdev_destroy(void)
{
  device_destroy(class, MKDEV(CDEV_MAJOR, 0));
  class_unregister(class);
  class_destroy(class);
  unregister_chrdev(CDEV_MAJOR, DEVICE_NAME);
  printk(KERN_DEBUG "[HOOK] [cdev] destroyed done\n");
}

int cdev_open(struct inode *inode, struct file *filp)
{
  printk(KERN_DEBUG "[HOOK] [cdev] successfuly opened\n");
  return 0;
}

int cdev_release(struct inode *inode, struct file *filp)
{
  printk(KERN_DEBUG "[HOOK] [cdev] successfuly released\n");
  return 0;
}

ssize_t cdev_read(struct file *filp, char __user * buffer,
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

ssize_t cdev_write(struct file *filp, const char __user *buffer,
    size_t len, loff_t *offset)
{
  printk(KERN_DEBUG "[HOOK] [cdev] received from userspace: %s\n", buffer);
  return len;
}

/* set the entry point of the module */
module_init(syscall_init);
/* set the exit point of the module */
module_exit(syscall_release);
