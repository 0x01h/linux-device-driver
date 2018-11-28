/*
* Written and tested in Linux eos 4.15.0-39-generic
* Compatible with newest version of Linux kernel.
* Check mydriver's major number by inserting manually and change the corresponding line in "install.sh".
*/

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/kernel.h>	    /* printk() */
#include <linux/slab.h>		    /* kmalloc() */
#include <linux/fs.h>
#include <linux/errno.h>	    /* error codes */
#include <linux/types.h>	    /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>	    /* O_ACCMODE */
#include <linux/seq_file.h>
#include <linux/cdev.h>
#include <asm/switch_to.h>		/* cli(), *_flags */
#include <asm/uaccess.h>	    /* copy_*_user */
#include <linux/uaccess.h>
#include "mydriver_ioctl.h"

#define DRIVER_NR_DEVS 3
#define DRIVER_MAJOR 0
#define CLASS_NAME "queue"
#define MAX_LIMIT 32

int driver_major = DRIVER_MAJOR;
int driver_minor = 0;
int driver_nr_devs = DRIVER_NR_DEVS;

module_param(driver_nr_devs, int, S_IRUGO);

MODULE_AUTHOR("Furkan Cakir, Hakan Eroztekin, Orcun Ozdemir");
MODULE_DESCRIPTION("A simple in-memory character device driver which uses queue to read/write operations.");
MODULE_LICENSE("GPL v3");
MODULE_VERSION("1.0");

struct driver_dev {
    char *data;
    unsigned long size;
    struct semaphore sem;
    struct cdev cdev;
};

struct driver_dev *driver_devices;

int driver_open(struct inode *inode, struct file *filp)
{
	printk(KERN_INFO "My Driver: Open request is called.\n");

    struct driver_dev *dev;

    dev = container_of(inode->i_cdev, struct driver_dev, cdev);
    filp->private_data = dev;

    if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {
		printk(KERN_INFO "My Driver: Open request is for write only mode.\n");
        if (down_interruptible(&dev->sem)) {    // ->sem for calling semaphores and handling interrupts.
            printk(KERN_ERR "My Driver: Not interruptable.\n");
            return -ERESTARTSYS;
        }

    // If data is not NULL, so free all of previous data.
    if (dev->data) {
        kfree(dev->data);
        dev->data = NULL;
        dev->size = 0;
    }

        up(&dev->sem);  // Restore the semaphore value.
    }

	printk(KERN_INFO "My Driver: Opened successfully.\n");
    return 0;
}

int driver_release(struct inode *inode, struct file *filp)
{
	printk(KERN_INFO "My Driver: Released.\n");
    return 0;
}

 /***************************************************************************************
 *  filp: A pointer to a file object.
 *  buf: The buffer to that contains the string to write to the device.
 *  count: The length of the array of data that is being passed in the const char buffer.
 *  f_pos: Offset.
 ****************************************************************************************/

ssize_t driver_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
 {
    printk(KERN_INFO "My Driver: Read request is called.\n");
    struct driver_dev *dev = filp->private_data;    // Private data in kernel for individual devices.

    ssize_t bytes = count < (MAX_LIMIT-(*f_pos)) ? count : (MAX_LIMIT-(*f_pos));

    if (down_interruptible(&dev->sem)) {
        printk(KERN_ERR "My Driver: Not interruptable ERROR!\n");
        return -ERESTARTSYS;
    }

    if (copy_to_user(buf, dev->data, bytes)) {
        printk(KERN_ERR "My Driver: Copy data from kernel to user ERROR!\n");
        return -EFAULT;
    }

    (*f_pos) += bytes;  // Increment position iterator to synchronize buffer outputs.

    up(&dev->sem);      // Send up signal to semaphore.
    printk(KERN_INFO "My Driver: Returning from the write request.\n");
    return bytes;       // Return only the amount of bytes that you copy to user to prevent errors and bugs.
}

ssize_t driver_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    printk(KERN_INFO "My Driver: Write request is called.\n");
    struct driver_dev *dev = filp->private_data;
    ssize_t return_val = -ENOMEM;

    printk(KERN_INFO "My Driver: Written data is '%s'", buf);
    printk(KERN_INFO "My Driver: %d bytes were written.", count);

    if (down_interruptible(&dev->sem)) {
        printk(KERN_ERR "My Driver: Not interruptable ERROR!\n");
        return -ERESTARTSYS;
    }

    if (*f_pos > count) {
        printk(KERN_ERR "My Driver: f_pos ERROR!\n");
        return_val = 0;
        goto out;
    }

    if (!dev->data) {
        dev->data = kmalloc(count * sizeof(char *), GFP_KERNEL);
        memset(dev->data, 0, count * sizeof(char *));
    }

    if (copy_from_user(dev->data, buf, count)) {
        printk(KERN_ERR "My Driver: Copy from user ERROR!\n");
        return_val = -EFAULT;
        goto out;
    }

    *f_pos += count;
    dev->size = *f_pos;
    return_val = count;

out:
    up(&dev->sem);
    printk(KERN_INFO "My Driver: Returning from the read request.\n");
    return return_val;
}

long driver_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) // Called by the ioctl system call
{

	int err = 0, tmp;
	int retval = 0;

	/*
	 * extract the type and number bitfields, and don't decode
	 * wrong cmds: return ENOTTY (inappropriate ioctl) before access_ok()
	 */
	if (_IOC_TYPE(cmd) != DRIVER_IOC_MAGIC) return -ENOTTY;
	if (_IOC_NR(cmd) > DRIVER_IOC_MAXNR) return -ENOTTY;

	/*
	 * the direction is a bitmask, and VERIFY_WRITE catches R/W
	 * transfers. `Type' is user-oriented, while
	 * access_ok is kernel-oriented, so the concept of "read" and
	 * "write" is reversed
	 */
	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err =  !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
	if (err) return -EFAULT;

	switch(cmd) {
	  case DRIVER_IOCRESET:
		break;

	  default:  /* redundant, as cmd was checked against MAXNR */
		return -ENOTTY;
	}
	return retval;
}


loff_t driver_llseek(struct file *filp, loff_t off, int whence) // Change current read/write position in a file 
{
    struct driver_dev *dev = filp->private_data;
    loff_t newpos;

    switch(whence) {
        case 0: /* SEEK_SET */
            newpos = off;
            break;

        case 1: /* SEEK_CUR */
            newpos = filp->f_pos + off;
            break;

        case 2: /* SEEK_END */
            newpos = dev->size + off;
            break;

        default: /* can't happen */
            return -EINVAL;
    }
    if (newpos < 0)
        return -EINVAL;
    filp->f_pos = newpos;
    return newpos;
}

struct file_operations driver_fops = { 
    .owner =    THIS_MODULE,
    .llseek =   driver_llseek,
    .read =     driver_read,
    .write =    driver_write,
    .unlocked_ioctl =  driver_ioctl,
    .open =     driver_open,
    .release =  driver_release,
};

void driver_cleanup_module(void)
{
	printk(KERN_INFO "My Driver: RMMOD request is called, cleaning up the module...\n");
    
    dev_t devno = MKDEV(driver_major, driver_minor);
    int i;

    if (driver_devices) {
        for (i = 0; i < driver_nr_devs; i++) {
            cdev_del(&driver_devices[i].cdev);
        }
    kfree(driver_devices);
    }

    unregister_chrdev_region(devno, driver_nr_devs);
    printk(KERN_INFO "My Driver: Cleanup was completed, goodbye from the module!\n");
    return;
}

int driver_init_module(void)
{
	printk(KERN_INFO "My Driver: Initializing the module...\n");
    
    int result, i;
    int err;
    dev_t devno = 0; // start naming from 0
    struct driver_dev *dev;

    if (driver_major) { // for the first device (!!)
        devno = MKDEV(driver_major, driver_minor);
        result = register_chrdev_region(devno, driver_nr_devs, "driver");
    } else {
        result = alloc_chrdev_region(&devno, driver_minor, driver_nr_devs, "driver");
        driver_major = MAJOR(devno);
    }
    if (result < 0) {
        printk(KERN_ERR "My Driver: Register a major number %d ERROR!\n", driver_major);
        return result;
    }

    printk(KERN_INFO "My Driver: Registered correctly with major number %d.\n", driver_major);
    
    driver_devices = kmalloc(driver_nr_devs * sizeof(struct driver_dev), GFP_KERNEL);

    if (!driver_devices) {
        result = -ENOMEM;
        printk(KERN_ERR "My Driver: Creating device ERROR!\n");
        goto fail;
    }
    memset(driver_devices, 0, driver_nr_devs * sizeof(struct driver_dev));

    for (i = 0; i < driver_nr_devs; i++) {
        dev = &driver_devices[i];
        sema_init(&dev->sem,1);
        devno = MKDEV(driver_major, driver_minor + i);
        cdev_init(&dev->cdev, &driver_fops);
        dev->cdev.owner = THIS_MODULE;
        dev->cdev.ops = &driver_fops;
        err = cdev_add(&dev->cdev, devno, 1);
        if (err)
            printk(KERN_NOTICE "My Driver: Error %d adding %d.\n", err, i);
    }

    printk(KERN_INFO "My Driver: Device initialized correctly.\n");
    return 0;

  fail:
    driver_cleanup_module();
    return result;
}

module_init(driver_init_module);
module_exit(driver_cleanup_module);