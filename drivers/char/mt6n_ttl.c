/*
 *	Trizeps-2 MT6N development board TTL-IO interface for Linux	
 *
 *	Copyright (C) 2003 Luc De Cock
 *
 * 	This driver allows use of the TTL-IO interface on the MT6N
 * 	from user space. It exports the /dev/ttlio interface supporting
 * 	some ioctl() and also the /proc/driver/ttlio pseudo-file
 * 	for status information.
 *
 *	The ioctls can be used to set individual TTL output lines.
 *	Only ioctls are supported.
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 *
 *	Based on other minimal char device drivers, like Alan's
 *	watchdog, Ted's random, Paul's rtc, etc. etc.
 *
 *	1.00	Luc De Cock: initial version.
 */

#define TTLIO_VERSION		"1.00"


#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/miscdevice.h>
#include <linux/fcntl.h>
#include <linux/init.h>
#include <linux/poll.h>
#include <linux/proc_fs.h>

#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/system.h>
#include <asm/hardware.h>
#include <asm/irq.h>

/* Writing to the register sets the output lines
*  Reading from the register returns the status of the input lines
*/
static unsigned short *ttlio_base = (unsigned short *) TRIZEPS2_TTLIO_BASE;
static unsigned short ttlio_shadow = 0;

/* interrupt stuff */
static struct fasync_struct *ttlio_async_queue;
static DECLARE_WAIT_QUEUE_HEAD(ttlio_wait);
static int ttlio_irq_arrived = 0;
static spinlock_t ttlio_lock;
static unsigned short ttlio_in = 0;
static volatile unsigned long teller = 0;


static int ttlio_ioctl(struct inode *inode, struct file *file,
		       unsigned int cmd, unsigned long arg);

static int ttlio_read_proc(char *page, char **start, off_t off,
                           int count, int *eof, void *data);


static void ttlio_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	ttlio_in = *ttlio_base;

	ttlio_irq_arrived = 1;
	teller++;
	
	/* wake up the waiting process */
	wake_up_interruptible(&ttlio_wait);
	kill_fasync(&ttlio_async_queue, SIGIO, POLL_IN);
}

/*
 *	Now all the various file operations that we export.
 */

static int ttlio_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
		       unsigned long arg)
{
	unsigned long ttlio_val;

	switch (cmd) {
	case TTLIO_RESET:	/* clear all lines */
	{
		*ttlio_base = 0;
		return 0;
	}
	case TTLIO_GET:		/* get state of TTL input lines	*/
	{
		ttlio_val = *ttlio_base;
		return put_user(ttlio_val, (unsigned long *)arg);
	}
	case TTLIO_SET:		/* set state of TTL output lines */
	{
		unsigned long user_val;
		if (copy_from_user(&user_val, arg, sizeof(unsigned long)))
			return -EFAULT;
		ttlio_shadow |= (unsigned short) user_val;
		*ttlio_base = ttlio_shadow;
		return 0;
	}
	case TTLIO_UNSET:	/* unset (clear) state of TTL output lines */
	{
		unsigned long user_val;
		if (copy_from_user(&user_val, arg, sizeof(unsigned long)))
			return -EFAULT;
		ttlio_shadow &= ~((unsigned short) user_val);
		*ttlio_base = ttlio_shadow;
		return 0;
	}
	case 100:		/* get counter */
	{
		return put_user(teller, (unsigned long *)arg);
	}
	case 101:		/* reset counter */
	{
		teller = 0;
		return 0;
	}
	default:
		return -ENOTTY;
	}
	return 0;
}

static ssize_t ttlio_read(struct file *file, char *buf,
			  size_t count, loff_t *ppos)
{
	DECLARE_WAITQUEUE(wait, current);
	unsigned short data;
	ssize_t retval;

	if (count < sizeof(unsigned short))
		return -EINVAL;

	if (file->f_flags & O_NONBLOCK) {
		spin_lock_irq(&ttlio_lock);
		data = *ttlio_base;
		spin_unlock_irq(&ttlio_lock);
		retval = put_user(data, (unsigned short *) buf); 
		if (!retval)
			retval = sizeof(unsigned short);
		return retval;
	}
	/* blocking read: wait for interrupt */
	add_wait_queue(&ttlio_wait, &wait);
	set_current_state(TASK_INTERRUPTIBLE);
	for (;;) {
		spin_lock_irq(&ttlio_lock);
		data = *ttlio_base;
		if (ttlio_irq_arrived) {
			ttlio_irq_arrived = 0;
			break;
		}
		spin_unlock_irq(&ttlio_lock);

		if (signal_pending(current)) {
			retval = -ERESTARTSYS;
			goto out;
		}
		schedule();
	}

	spin_unlock_irq(&ttlio_lock);
	retval = put_user(data, (unsigned short *)buf);
	if (!retval)
		retval = sizeof(unsigned short);

out:
	set_current_state(TASK_RUNNING);
	remove_wait_queue(&ttlio_wait, &wait);
	return retval;
}

static ssize_t ttlio_write(struct file *file,
		           const char *buf, size_t count, loff_t *ppos)
{
	unsigned short content;

	if (count < sizeof(unsigned short))
		return -EINVAL;
	
	if (copy_from_user (&content, buf, sizeof(unsigned short)))
		return -EFAULT;

	ttlio_shadow = content;
	*ttlio_base = ttlio_shadow;

	*ppos += sizeof(unsigned short);

	return sizeof(unsigned short);
}

static int ttlio_open(struct inode *inode, struct file *file)
{
	ttlio_irq_arrived = 0;
	return 0;
}

static int ttlio_fasync(int fd, struct file *filp, int on)
{
	return fasync_helper(fd, filp, on, &ttlio_async_queue);
}

static unsigned int ttlio_poll(struct file *file, poll_table *wait)
{
	poll_wait(file, &ttlio_wait, wait);
	return ttlio_irq_arrived ? 0 : POLLIN | POLLRDNORM;
}

static loff_t ttlio_llseek(struct file *file, loff_t offset, int origin)
{
	return -ESPIPE;
}

/*
 *	The various file operations we support.
 */

static struct file_operations ttlio_fops = {
	owner:		THIS_MODULE,
	llseek:		ttlio_llseek,
	read:		ttlio_read,
	poll:		ttlio_poll,
	write:		ttlio_write,
	ioctl:		ttlio_ioctl,
	open:		ttlio_open,
	fasync:		ttlio_fasync,
};

static struct miscdevice ttlio_dev = {
	TTLIO_MINOR,
	"ttlio",
	&ttlio_fops
};

static int __init ttlio_init(void)
{
	printk(KERN_INFO "MT6N TTL-I/O driver (release %s)\n",
			TTLIO_VERSION);
	
	misc_register(&ttlio_dev);
	create_proc_read_entry ("driver/ttlio", 0, 0, ttlio_read_proc, NULL);

	set_GPIO_IRQ_edge(GPIO_TTLIO_IRQ, GPIO_FALLING_EDGE);
	if (request_irq(TTLIO_IRQ, ttlio_interrupt, SA_INTERRUPT, "ttlio irq", NULL)) {
		printk(KERN_ERR "ttlio: irq %d already in use\n", TTLIO_IRQ);
		return 1;
	}
	return 0;
}

static void __exit ttlio_exit(void)
{
	free_irq(TTLIO_IRQ, NULL);
	remove_proc_entry ("driver/ttlio", NULL);
	misc_deregister(&ttlio_dev);
}

module_init(ttlio_init);
module_exit(ttlio_exit);
EXPORT_NO_SYMBOLS;

/*
 *	Info exported via "/proc/driver/ttlio".
 */

static int ttlio_proc_output(char *buf)
{
	char *p;
	unsigned short val;
	int i;

	p = buf;

	p += sprintf(p, "input : ");
	/* write the state of the input lines */
	val = *ttlio_base;
	for (i = 0; i < 8*sizeof(unsigned short); i++) {
		*p++ = (val & 1) ? '1' : '0';
		val >>= 1;
	}
	*p = 0;
	p += sprintf(p, "\noutput: ");
	/* write the state of the output lines */
	val = ttlio_shadow;
	for (i = 0; i < 8*sizeof(unsigned short); i++) {
		*p++ = (val & 1) ? '1' : '0';
		val >>= 1;
	}
	*p = 0;
	p += sprintf(p, "\n");

	return  p - buf;
}

static int ttlio_read_proc(char *page, char **start, off_t off,
                           int count, int *eof, void *data)
{
        int len = ttlio_proc_output (page);
        if (len <= off+count) *eof = 1;
        *start = page + off;
        len -= off;
        if (len > count) len = count;
        if (len < 0) len = 0;
        return len;
}

MODULE_AUTHOR("Luc De Cock");
MODULE_DESCRIPTION("MT6N TTL-I/O driver");
MODULE_LICENSE("GPL");
