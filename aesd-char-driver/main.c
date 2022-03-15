/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes 
 * 					Updated by Tanmay Mahendra Kothale on 13th March 2022
 * 					Changes for ECEN 5713 Advanced Embedded Software Development
 * 					Assignment 8
 * 
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/fs.h> // file_operations
#include "aesdchar.h"
int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Tanmay Mahendra Kothale");
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

/**
 * @desc the open call used to get the character device(cdev) from aesd_dev structure.
 * @param inode the kernel inode structure.
 * @param filp the kernel file structure passed from caller
 * @return function exit status
 */
int aesd_open(struct inode *inode, struct file *filp)
{
	struct aesd_dev *device;

	PDEBUG("open");
	device = container_of(inode->i_cdev, struct aesd_dev, cdev);
	filp->private_data = device;

	return 0;
}

/**
 * @desc the release system call used to release andy kernel resources.
 * @param inode the kernel inode structure.
 * @param filp the kernel file structure passed from caller
 * @return function exit status
 */
int aesd_release(struct inode *inode, struct file *filp)
{
	PDEBUG("release");

	return 0;
}

/**
 * @desc the release system call used to release andy kernel resources.
 * @param filp the kernel file structure passed from caller.
 * @param buf the buffer pointer at which the read data needs to be stored.
 * @param count the number of bytes required to be read from kernel buffer.
 * @param f_pos the entry offset location in kernel buffer from where data will be read.
 * @return no of bytes successfully read.
 */
ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
	ssize_t retval = 0;
	struct aesd_dev *device;
	struct aesd_buffer_entry *read_entry = NULL;
	ssize_t read_offset = 0;
	ssize_t read_data = 0;

	PDEBUG("read %zu bytes with offset %lld",count,*f_pos);

	device = (struct aesd_dev*) filp->private_data;

	if(filp == NULL || buf == NULL || f_pos == NULL)
	{
		return -EFAULT; 
	}
	
	if(mutex_lock_interruptible(&device->lock))
	{
		PDEBUG(KERN_ERR "could not acquire mutex lock");
		return -ERESTARTSYS;
	}

	read_entry = aesd_circular_buffer_find_entry_offset_for_fpos(&(device->circular_buffer), *f_pos, &read_offset); 
	if(read_entry == NULL)
	{
		goto handle_error;
	}
	else
	{
		if(count > (read_entry->size - read_offset))
		{
			count = read_entry->size - read_offset;
		}
	}

	read_data = copy_to_user(buf, (read_entry->buffptr + read_offset), count);
	
	retval = count - read_data;
	*f_pos += retval;

handle_error:
	mutex_unlock(&(device->lock));

	return retval;
}

/**
 * @desc the release system call used to release andy kernel resources.
 * @param filp the kernel file structure passed from caller.
 * @param buf the buffer pointer which contains the data to be written at kernel buffer entry
 * @param count the number of bytes required to be written to kernel buffer.
 * @param f_pos the file postion location which will be updated after each write.
 * @return no of bytes successfully written.
 */
ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{	
	struct aesd_dev *device;
	const char *new_entry = NULL;
	ssize_t retval = -ENOMEM;
	ssize_t unwritten_bytes = 0;
	PDEBUG("write %zu bytes with offset %lld",count,*f_pos);
	
	//check arguement errors
	if(count == 0)
	{
		return 0;
	}

	if(filp == NULL || buf == NULL || f_pos== NULL)
	{
		return -EFAULT;
	}

	device = (struct aesd_dev*) filp->private_data;

	if(mutex_lock_interruptible(&(device->lock)))
	{
		PDEBUG(KERN_ERR "could not acquire mutex lock");
		return -ERESTARTSYS;
	}
	
	if(device->buffer_entry.size == 0)
	{
		device->buffer_entry.buffptr = kmalloc(count*sizeof(char), GFP_KERNEL);

		if(device->buffer_entry.buffptr == NULL)
		{
			PDEBUG("kmalloc error");
			goto exit_error;
		}
	}

	else
	{
		device->buffer_entry.buffptr = krealloc(device->buffer_entry.buffptr, (device->buffer_entry.size + count)*sizeof(char), GFP_KERNEL);

		if(device->buffer_entry.buffptr == NULL)
		{
			PDEBUG("krealloc error");
			goto exit_error;
		}
	}


	unwritten_bytes = copy_from_user((void *)(device->buffer_entry.buffptr + device->buffer_entry.size), buf, count);
	retval = count - unwritten_bytes; 
	device->buffer_entry.size += retval;

	if(memchr(device->buffer_entry.buffptr, '\n', device->buffer_entry.size))
	{
		new_entry = aesd_circular_buffer_add_entry(&device->circular_buffer, &device->buffer_entry); 
		if(new_entry)
		{
			kfree(new_entry);
		}

		device->buffer_entry.buffptr = NULL;
		device->buffer_entry.size = 0;

	}

	PDEBUG("not doing k_free for now");

exit_error:
	mutex_unlock(&device->lock);

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

/**
 * @desc this function is used to initialize the device and add it.
 * @return return value of mkdev.
 */
static int aesd_setup_cdev(struct aesd_dev *dev)
{
	int err, devno = MKDEV(aesd_major, aesd_minor);

	cdev_init(&dev->cdev, &aesd_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &aesd_fops;
	err = cdev_add (&dev->cdev, devno, 1);
	if (err) 
	{
		printk(KERN_ERR "Error %d adding aesd cdev", err);
	}
	return err;
}

/**
 * @desc this function is used to register the device and initialize the kernel
 * data structures.
 * @return the return value of register and init functions.
 */
int aesd_init_module(void)
{
	dev_t dev = 0;
	int status;
	status = alloc_chrdev_region(&dev, aesd_minor, 1, "aesdchar");
	aesd_major = MAJOR(dev);
	if (status < 0) 
	{
		printk(KERN_WARNING "Can't get major %d\n", aesd_major);
		return status;
	}
	memset(&aesd_device,0,sizeof(struct aesd_dev));

	//Initialize the mutex and circular buffer
	mutex_init(&aesd_device.lock);
	aesd_circular_buffer_init(&aesd_device.circular_buffer);

	status = aesd_setup_cdev(&aesd_device);

	if(status) 
	{
		unregister_chrdev_region(dev, 1);
	}
	return status;
}

/**
 * @desc this function is used to unregister the device and deallocated all the kernel data structures and delte the device
 * @return none.
 */
void aesd_cleanup_module(void)
{
	//free circular buffer entries
	struct aesd_buffer_entry *entry = NULL;
	uint8_t index = 0; 

	dev_t devno = MKDEV(aesd_major, aesd_minor);

	cdev_del(&aesd_device.cdev);

	kfree(aesd_device.buffer_entry.buffptr);

	AESD_CIRCULAR_BUFFER_FOREACH(entry, &aesd_device.circular_buffer, index){
		if(entry->buffptr != NULL)
		{
			kfree(entry->buffptr);
		}
	}

	unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);