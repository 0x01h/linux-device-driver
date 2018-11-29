/*
* Written and tested in Linux eos 4.15.0-39-generic
* Compatible with newest version of Linux kernel.
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
#include <linux/sched.h>
#include <linux/string.h>

#define DRIVER_NR_DEVS 5        /* Default number of devices, if not given in arguments. */
#define DRIVER_MAJOR 0          // Dynamically get major number of driver.
#define MAX_LIMIT 32            // String limit to echo into a file.

MODULE_AUTHOR("Orcun Ozdemir, Furkan Cakir, Hakan Eroztekin");
MODULE_DESCRIPTION("A simple in-memory character device driver which uses queue to read/write operations.");
MODULE_LICENSE("GPL and additional rights");
MODULE_VERSION("1.0");

unsigned int driver_major = DRIVER_MAJOR;
unsigned int driver_nr_devs = DRIVER_NR_DEVS;
static struct cdev c_dev;       // Global variable for the character device structure.
static struct class *cl;        // Global variable for the device class.
char messageArray[1024];

module_param(driver_nr_devs, int, S_IRUGO);

// Queue Variables
int len,temp,i=0,ret;
char *empty="Queue Empty  \n";
int emp_len,flag=1;


// START - Queue Operations

struct k_list {
    struct list_head queue_list;
    char *data;
};

static struct k_list *node;
struct list_head *head;
struct list_head test_head;
int new_node=1;
int msgCounter = 0;

char *msg;

ssize_t pop_queue(struct file *filp,char *buf,size_t count,loff_t *offp)
{
    if(list_empty(head)){

        if(flag==1) {
            ret=emp_len;
            flag=0;
        }
        else if(flag==0){
            ret=0;
            flag=1;
        }
        temp=copy_to_user(buf,msg, count);
        printk(KERN_INFO "\nStack empty\n");
        return temp;
    }

    if(new_node == 1) {
        node=list_first_entry(head,struct k_list ,queue_list);
        msg=node->data;
        ret=strlen(msg);

        new_node=0;

    }
    if(count>ret) {
        count=ret;

    }

    ret=ret-count;

    temp=copy_to_user(buf,msg, count);
    printk(KERN_INFO "\n data = %s \n" ,msg);

    if(count==0) {
        list_del(&node->queue_list);
        new_node=1;
    }
    return temp;
}

ssize_t push_queue(struct file *filp,const char *buf,size_t count,loff_t *offp)
{
    msgCounter++;
    msg=kmalloc(10*sizeof(char),GFP_KERNEL);

    temp=copy_from_user(msg,buf,count);

    node=kmalloc(sizeof(struct k_list *),GFP_KERNEL);
    node->data=msg;
    list_add_tail(&node->queue_list,head);
    return count;
}

//struct file_operations proc_fops = {
//	.owner =    THIS_MODULE,
//	.read =     pop_queue,
//	.write =    push_queue,
//};
//void create_new_proc_entry(void)
//{
//	proc_create("queue",0,NULL,&driver_fops);
//}


int queue_init (void) {
//	create_new_proc_entry();
    head=kmalloc(sizeof(struct list_head *),GFP_KERNEL);
    INIT_LIST_HEAD(head);
    emp_len=strlen(empty);
    return 0;
}

void queue_cleanup(void) {
    remove_proc_entry("queue",NULL);
}
// END

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


if (pop_queue(filp, buf, count, f_pos)) {
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

printk(KERN_INFO "My Driver: Written data is %s", buf);
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

if (!(dev->data)) {
dev->data = kmalloc(count * sizeof(char *), GFP_KERNEL);
memset(dev->data, 0, count * sizeof(char *));
}

if (push_queue(filp, buf, count, f_pos) == 0) {
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

long driver_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
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


loff_t driver_llseek(struct file *filp, loff_t off, int whence) // Change current read/write position in a file.
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

        default: /* Invalid value! */
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
    printk(KERN_INFO "My Driver: Major %d will be cleaned.\n", driver_major);
    queue_cleanup(); // queue cleanup

    int i;

    unregister_chrdev_region(driver_major, driver_nr_devs);

    for (i = 0; i < driver_nr_devs; i++) {
        dev_t devno = MKDEV(driver_major, i);
        device_destroy(cl, devno);
    }

    cdev_del(&c_dev);
    class_destroy(cl);

    printk(KERN_INFO "My Driver: Cleanup was completed, goodbye from the module!\n");
    return;
}

int driver_init_module(void)
{
    printk(KERN_INFO "My Driver: Initializing the module...\n");

    int result, i;
    int err;
    dev_t devno = 0; // Start naming from 0.
    struct driver_dev *dev;

    result = alloc_chrdev_region(&devno, 0, driver_nr_devs, "mydriver");
    driver_major = MAJOR(devno);
    queue_init(); // queue initialization

    if (result < 0) {
        printk(KERN_ERR "My Driver: Register a major number %d ERROR!\n", driver_major);
        return result;
    }

    printk(KERN_INFO "My Driver: Registered correctly with major number %d.\n", driver_major);

    if (!(cl = class_create(THIS_MODULE, "mydriver_class"))) {
        printk(KERN_ERR "My Driver: Register class ERROR!\n");
        unregister_chrdev_region(devno, driver_nr_devs);
        return EINVAL;
    }

    cdev_init(&c_dev, &driver_fops);

    printk(KERN_INFO "My Driver: %d devices will be created.\n", driver_nr_devs);

    if (cdev_add(&c_dev, devno, driver_nr_devs) == -1) {
        printk(KERN_ERR "My Driver: Device add ERROR!\n");
        device_destroy(cl, devno);
        class_destroy(cl);
        unregister_chrdev_region(devno, driver_nr_devs);
        return EINVAL;
    }

    for (i = 0; i < driver_nr_devs; i++) {
        if (!device_create(cl, NULL, MKDEV(driver_major, i), NULL, "queue%d", i)) {
            printk(KERN_ERR "My Driver: Device create ERROR!\n");
            class_destroy(cl);
            unregister_chrdev_region(devno, driver_nr_devs);
            return EINVAL;
        }
    }

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
        devno = MKDEV(driver_major, i);
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
