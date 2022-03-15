/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
			Updated by Tanmay Mahendra Kothale on March 13 2022
			Changes for ECEN 5713 - Advanced Embedded Software
			Development Assignent 8
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h> // file_operations
#include "aesdchar.h"

#define SUCCESS	(0)

int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Tanmay Mahendra Kothale"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
	PDEBUG("open");

	struct aesd_dev *device;
	device = container_of(inode->i_cdev, struct aesd_dev, cdev);
	filp->private_data = device;

	return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
	PDEBUG("release");
	/**
	 * TODO: handle release
	 */
	return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
	int status;
	ssize_t retval = 0;
	ssize_t read_offset = 0, read_size = 0, uncopied_count = 0;
	struct aesd_buffer_entry *read_entry = NULL;
	struct aesd_dev *device;

	PDEBUG("read %zu bytes with offset %lld",count,*f_pos);

	if (count == 0)
	{
		return 0;
	}

	if (filp == NULL || buf == NULL || f_pos == NULL)
	{
		return NULL;
	}

	device = (struct aesd_dev*)filp->private_data;

	status = mutex_lock_interruptible(&(device->lock));
	if (status != SUCCESS)
	{
		PDEBUG (KERN_ERR "mutex_lock_interruptible failed\n");
	}

	read_entry = aesd_circular_buffer_find_entry_offset_for_fpos(&(device->cbuf), *f_pos, &read_offset);
	if (read_entry == NULL)
	{
		goto handle_error;
	}
	else
	{
		read_size = read_entry->size - read_offset;
		if (read_size > count) 
		{
			read_size = count; 
		}
	}

	if (!access_ok(buf, count))
	{
		PDEBUG(KERN_ERR "aesd_read: access_ok fail");
		retval = -EFAULT;
		goto handle_error;
	}
	uncopied_count = copy_to_user(buf, read_entry->buffptr + read_offset, read_size); 
	retval = read_size - uncopied_count;
	*f_pos += retval;

	handle_error:
		mutex_unlock(&(dev->lock));
		return retval;

}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
	ssize_t retval = -ENOMEM;
	PDEBUG("write %zu bytes with offset %lld",count,*f_pos);
	/**
	 * TODO: handle write
	 */
	return retval;
}
struct file_operations aesd_fops = 
{
	.owner =    THIS_MODULE,
	.read =     aesd_read,
	.write =    aesd_write,
	.open =     aesd_open,
	.release =  aesd_release,
};

static int aesd_setup_cdev(struct aesd_dev *dev)
{
	int err, devno = MKDEV(aesd_major, aesd_minor);

	cdev_init(&dev->cdev, &aesd_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &aesd_fops;
	err = cdev_add (&dev->cdev, devno, 1);
	if (err) 
	{
		PDEBUG(KERN_ERR "cdev_add failed with error code %d\n", err);
	}
	return err;
}

int aesd_init_module(void)
{
	dev_t dev = 0;
	int status;
	status = alloc_chrdev_region(&dev, aesd_minor, 1, "aesdchar");
	aesd_major = MAJOR(dev);
	if (status < 0) 
	{
		PDEBUG(KERN_WARNING "MAJOR failed\n", aesd_major);
		return status;
	}
	memset(&aesd_device,0,sizeof(struct aesd_dev));

	/**
	 * TODO: initialize the AESD specific portion of the device
	 */

	status = aesd_setup_cdev(&aesd_device);

	if(status) 
	{
		unregister_chrdev_region(dev, 1);
	}
	return status;

}

void aesd_cleanup_module(void)
{
	dev_t devno = MKDEV(aesd_major, aesd_minor);

	cdev_del(&aesd_device.cdev);

	/**
	 * TODO: cleanup AESD specific poritions here as necessary
	 */

	unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
