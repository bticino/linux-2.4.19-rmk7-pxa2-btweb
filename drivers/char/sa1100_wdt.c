/*
 *	Watchdog driver for the SA11x0/PXA
 *
 *      (c) Copyright 2000 Oleg Drokin <green@crimea.edu>
 *          Based on SoftDog driver by Alan Cox <alan@redhat.com>
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 *
 *	Neither Oleg Drokin nor iXcelerator.com admit liability nor provide
 *	warranty for any of this software. This material is provided
 *	"AS-IS" and at no charge.
 *
 *	(c) Copyright 2000           Oleg Drokin <green@crimea.edu>
 *
 *      27/11/2000 Initial release
 */

#include <linux/module.h>
#include <linux/config.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/miscdevice.h>
#include <linux/watchdog.h>
#include <linux/reboot.h>
#include <linux/smp_lock.h>
#include <linux/init.h>
#include <asm/uaccess.h>
#include <asm/hardware.h>
#include <asm/bitops.h>

#define TIMER_MARGIN	60		/* (secs) Default is 1 minute */

static int timer_margin = TIMER_MARGIN;	/* in seconds */
static int sa1100wdt_users;
static int pre_margin;
static int wdt_force;

MODULE_PARM(timer_margin,"i");
MODULE_PARM(wdt_force, "i"); /* force activation at init time */

#ifndef MODULE
/* kernel command-line management */
static int __init wdt_set_margin(char *str)
{
	int par;
	if (get_option(&str,&par)) timer_margin = par;
        return 1;
}
static int __init wdt_set_force(char *str)
{
	int par;
	if (get_option(&str,&par)) wdt_force = par;
	return 1;
}

__setup("wdt_margin=", wdt_set_margin);
__setup("wdt_force=", wdt_set_force);
#endif

static void sa1100dog_ping( void)
{
	/* reload counter with (new) margin */
	pre_margin=3686400 * timer_margin;
	OSMR3 = OSCR + pre_margin;
}

/*
 *	Allow only one person to hold it open
 */

static int sa1100dog_open(struct inode *inode, struct file *file)
{
	if(test_and_set_bit(1,&sa1100wdt_users))
		return -EBUSY;
	MOD_INC_USE_COUNT;
	sa1100dog_ping();
	OSSR = OSSR_M3;
	OWER = OWER_WME;
	OIER |= OIER_E3;
	return 0;
}

static int sa1100dog_release(struct inode *inode, struct file *file)
{
	/*
	 *	Shut off the timer.
	 * 	Lock it in if it's a module and we defined ...NOWAYOUT
	 */
	OSMR3 = OSCR + pre_margin;
#ifndef CONFIG_WATCHDOG_NOWAYOUT
	OIER &= ~OIER_E3;
#endif
	sa1100wdt_users = 0;
	MOD_DEC_USE_COUNT;
	return 0;
}

static ssize_t sa1100dog_write(struct file *file, const char *data, size_t len, loff_t *ppos)
{
	/*  Can't seek (pwrite) on this device  */
	if (ppos != &file->f_pos)
		return -ESPIPE;

	/* Refresh OSMR3 timer. */
	if(len) {
		OSMR3 = OSCR + pre_margin;
		return 1;
	}
	return 0;
}

static int sa1100dog_ioctl(struct inode *inode, struct file *file,
	unsigned int cmd, unsigned long arg)
{
	static struct watchdog_info ident = {
		identity: "PXA/SA1100 Watchdog",
		options: WDIOF_SETTIMEOUT,
		firmware_version: 0,
	};
	int new_margin;

	switch(cmd){
	default:
		return -ENOIOCTLCMD;
	case WDIOC_GETSUPPORT:
		return copy_to_user((struct watchdog_info *)arg, &ident, sizeof(ident));
	case WDIOC_GETSTATUS:
		return put_user(0,(int *)arg);
	case WDIOC_GETBOOTSTATUS:
		return put_user((RCSR & RCSR_WDR) ? WDIOF_CARDRESET : 0, (int *)arg);
	case WDIOC_KEEPALIVE:
		OSMR3 = OSCR + pre_margin;
		return 0;
	case WDIOC_SETTIMEOUT:
		if (get_user(new_margin, (int *)arg))
			return -EFAULT;
		if (new_margin < 1)
			return -EINVAL;
		timer_margin = new_margin;
		sa1100dog_ping();
		/* Fall */
	case WDIOC_GETTIMEOUT:
		return put_user(timer_margin, (int *)arg);
	}
}

static struct file_operations sa1100dog_fops=
{
	owner:		THIS_MODULE,
	write:		sa1100dog_write,
	ioctl:		sa1100dog_ioctl,
	open:		sa1100dog_open,
	release:	sa1100dog_release,
};

static struct miscdevice sa1100dog_miscdev=
{
	WATCHDOG_MINOR,
	"PXA_watchdog",
	&sa1100dog_fops
};

static int __init sa1100dog_init(void)
{
	int ret;

	ret = misc_register(&sa1100dog_miscdev);

	if (ret)
		return ret;

	if (wdt_force) {
		sa1100dog_ping();
		OSSR = OSSR_M3;
		OWER = OWER_WME;
		OIER |= OIER_E3;
	}

	printk("SA1100/PXA Watchdog Timer: timer margin %d sec,"
	       " currently %sactive\n", timer_margin, wdt_force ? "" : "in");

	return 0;
}

static void __exit sa1100dog_exit(void)
{
	misc_deregister(&sa1100dog_miscdev);
	if (wdt_force) {
		OSMR3 = OSCR + pre_margin;
#ifndef CONFIG_WATCHDOG_NOWAYOUT
		OIER &= ~OIER_E3;
#endif
	}
}

module_init(sa1100dog_init);
module_exit(sa1100dog_exit);
