/*
 * pcf8563.c
 *
 * Driver for PCF8563T RTC, as found in the PXA BtWeb device
 *
 * Derived from ds1307.c, Copyright (C) 2002 Intrinsyc Software Inc.
 * Derived from my previous BtWeb work, Copyright (C) 2003 Alessandro Rubini
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/version.h>

#include <linux/kernel.h>
#include <linux/poll.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/rtc.h>
#include <linux/string.h>
#include <linux/miscdevice.h>
#include <linux/proc_fs.h>

#include <asm/hardware.h> /* for btweb_globals when ARCH_BTWEB is set */

#define DEBUG 0

#if DEBUG
static unsigned int rtc_debug = DEBUG;
#else
#define rtc_debug 0	/* gcc will remove all the debug code for us */
#endif

static unsigned short slave_address = 0x51;

struct i2c_driver pcf_driver;
struct i2c_client *pcf_i2c_client = 0;

static unsigned short ignore[] = { I2C_CLIENT_END };
static unsigned short normal_addr[] = { 0x51, I2C_CLIENT_END };

static struct i2c_client_address_data addr_data = {
	normal_i2c:		normal_addr,
	normal_i2c_range:	ignore,
	probe:			ignore,
	probe_range:		ignore,
	ignore: 		ignore,
	ignore_range:		ignore,
	force:			ignore,
};


static int pcf_rtc_ioctl( struct inode *, struct file *, unsigned int, unsigned long);

static struct file_operations rtc_fops = {
	owner:		THIS_MODULE,
	ioctl:		pcf_rtc_ioctl,
};

static struct miscdevice pcf_rtc_miscdev = {
	RTC_MINOR,
	"rtc",
	&rtc_fops
};

static int pcf_probe(struct i2c_adapter *adap);
static int pcf_detach(struct i2c_client *client);

struct i2c_driver pcf_driver = {
	name:		"PCF8563",
	id:		80 /* should be I2C_DRIVERID_PCF8563 */,
	flags:		I2C_DF_NOTIFY,
	attach_adapter: pcf_probe,
	detach_client:	pcf_detach,
	//command:	pcf_command
};

static spinlock_t pcf_rtc_lock = SPIN_LOCK_UNLOCKED;

static int
pcf_attach(struct i2c_adapter *adap, int addr, unsigned short flags,int kind)
{
	struct i2c_client *c;
	unsigned char buf[1], ad[1] = { 7 };
	struct i2c_msg msgs[2] = {
		{ addr	, 0,	    1, ad  },
		{ addr	, I2C_M_RD, 1, buf }
	};
	int ret;

	c = (struct i2c_client *)kmalloc(sizeof(*c), GFP_KERNEL);
	if (!c)
		return -ENOMEM;

	strcpy(c->name, "PCF8563");
	c->id		= pcf_driver.id;
	c->flags	= 0;
	c->addr 	= addr;
	c->adapter	= adap;
	c->driver	= &pcf_driver;
	c->data 	= NULL;

	ret = i2c_transfer(c->adapter, msgs, 2);

	if (ret != 2)
		printk ("pcf_attach(): i2c_transfer() returned %d.\n",ret);

	pcf_i2c_client = c;

	return i2c_attach_client(c);
}

static int
pcf_probe(struct i2c_adapter *adap)
{
	return i2c_probe(adap, &addr_data, pcf_attach);
}

static int
pcf_detach(struct i2c_client *client)
{
	i2c_detach_client(client);

	return 0;
}

#define TOBIN(v) (((v)&0x0f) + 10*((v)>>4))
#define TOBCD(v) ((v)%10   + (((v)/10)<<4))

static int
pcf_convert_to_time( struct rtc_time *dt, char *buf)
{
	dt->tm_sec = TOBIN(buf[2] & 0x7f);
	dt->tm_min = TOBIN(buf[3] & 0x7f);
	dt->tm_hour = TOBIN(buf[4] & 0x3f);
	dt->tm_mday = TOBIN(buf[5] & 0x3f);
	dt->tm_mon = TOBIN(buf[7] & 0x1f) -1; /* tm_mon is 0-based */
	dt->tm_year = TOBIN(buf[8]) + 100; /* tm_year is 1900-based */

	if( rtc_debug > 2)
	{
		printk("pcf_get_datetime: year = %d\n", dt->tm_year);
		printk("pcf_get_datetime: mon  = %d\n", dt->tm_mon);
		printk("pcf_get_datetime: mday = %d\n", dt->tm_mday);
		printk("pcf_get_datetime: hour = %d\n", dt->tm_hour);
		printk("pcf_get_datetime: min  = %d\n", dt->tm_min);
		printk("pcf_get_datetime: sec  = %d\n", dt->tm_sec);
	}
	return (buf[2] & 0x80)!= 0; /* 0: valid; 1: invalid */
}

static int
pcf_get_datetime(struct i2c_client *client, struct rtc_time *dt)
{
	unsigned s1, s2;
	unsigned char buf[9], addr[1] = { 0 };
	struct i2c_msg msgs[2] = {
		{ client->addr, 0,	  1, addr },
		{ client->addr, I2C_M_RD, 9, buf  }
	};
	int ret = -EIO;



	/* re-do this untile the second is stable */
	s1 = 0; s2 = 1;
	ret = i2c_transfer(client->adapter, msgs, 2);

	while (s1 != s2 && ret == 2) {
		if (ret != 2) break;
		s1 = buf[2];
		ret = i2c_transfer(client->adapter, msgs, 2);
		s2 = buf[2];
	}

	if (ret != 2) {
                printk("pcf_get_datetime(), i2c_transfer() returned %d\n",ret);
		#ifdef CONFIG_MACH_BTWEB
			btweb_globals.rtc_invalid = 1;
		#endif
		return ret;
	}
	ret = pcf_convert_to_time( dt, buf);
#ifdef CONFIG_MACH_BTWEB
	btweb_globals.rtc_invalid = ret;
#endif

#if 0 
	if (rtc_debug > 0 && ret)
		printk(KERN_WARNING "rtc: date is invalid: setting to old millennium midnight\n");
	if (ret) {
		dt->tm_sec = 1;
		dt->tm_min = 0;
		dt->tm_hour = 0;
		dt->tm_mday = 1;
		dt->tm_mon = 0; /* tm_mon is 0-based */
		dt->tm_year = 100; /* tm_year is 1900-based */
	}
#endif 

	return 0;
}

static int
pcf_set_datetime(struct i2c_client *client, struct rtc_time *dt, int datetoo)
{
	unsigned char buf[9];
	int ret, len = 4; /* addr + 2..4 */

	if( rtc_debug > 2)
	{
		printk("pcf_set_datetime: tm_year = %d\n", dt->tm_year);
		printk("pcf_set_datetime: tm_mon  = %d\n", dt->tm_mon);
		printk("pcf_set_datetime: tm_mday = %d\n", dt->tm_mday);
		printk("pcf_set_datetime: tm_hour = %d\n", dt->tm_hour);
		printk("pcf_set_datetime: tm_min  = %d\n", dt->tm_min);
		printk("pcf_set_datetime: tm_sec  = %d\n", dt->tm_sec);
	}

	/* To use the same register numbers as in reading, start from 1 */
	buf[1] = 2;	/* register address on PCF */
	buf[2] = TOBCD(dt->tm_sec);
	buf[3] = TOBCD(dt->tm_min);
	buf[4] = TOBCD(dt->tm_hour);

	if (datetoo) {
		len = 8 /* addr + 2..8 */;
		buf[5] = TOBCD(dt->tm_mday);
		/* we skip buf[6] as we don't use day-of-week. */
		buf[7] = TOBCD(dt->tm_mon + 1);
		/* The year only ranges from 0-99, we are being passed an offset from 1900,
		 * and the chip calulates leap years based on 2000, thus we adjust by 100.
		 */
		buf[8] = (TOBCD(dt->tm_year - 100));
	}
	ret = i2c_master_send(client, (char *)buf+1, len);
	if (ret == len)
		ret = 0;
	else
		printk("pcf_set_datetime(), i2c_master_send() returned %d\n",ret);
#ifdef CONFIG_MACH_BTWEB
	if (!ret) btweb_globals.rtc_invalid = 0;
#endif

	return ret;
}

static int
pcf_rtc_ioctl( struct inode *inode, struct file *file,
		unsigned int cmd, unsigned long arg)
{
	unsigned long	flags;
	struct rtc_time wtime;
	int status = 0;

	switch (cmd) {
		default:
		case RTC_UIE_ON:
		case RTC_UIE_OFF:
		case RTC_PIE_ON:
		case RTC_PIE_OFF:
		case RTC_AIE_ON:
		case RTC_AIE_OFF:
		case RTC_ALM_SET:
		case RTC_ALM_READ:
		case RTC_IRQP_READ:
		case RTC_IRQP_SET:
		case RTC_EPOCH_READ:
		case RTC_EPOCH_SET:
		case RTC_WKALM_SET:
		case RTC_WKALM_RD:
			status = -EINVAL;
			break;

		case RTC_RD_TIME:
			spin_lock_irqsave(&pcf_rtc_lock, flags);
			pcf_get_datetime(pcf_i2c_client, &wtime);
			spin_unlock_irqrestore(&pcf_rtc_lock,flags);

			if( copy_to_user((void *)arg, &wtime, sizeof (struct rtc_time)))
				status = -EFAULT;
			break;

		case RTC_SET_TIME:
			if (!capable(CAP_SYS_TIME))
			{
				status = -EACCES;
				break;
			}

			if (copy_from_user(&wtime, (struct rtc_time *)arg, sizeof(struct rtc_time)) )
			{
				status = -EFAULT;
				break;
			}

			spin_lock_irqsave(&pcf_rtc_lock, flags);
			pcf_set_datetime( pcf_i2c_client, &wtime, 1);
			spin_unlock_irqrestore(&pcf_rtc_lock,flags);
			break;
	}

	return status;
}

static __init int pcf_init(void)
{
        struct rtc_time wtime;
	unsigned long t, flags;
	int retval=0;

	normal_addr[0] = slave_address;

	if(normal_addr[0] >= 0x80)
	{
		printk(KERN_ERR"I2C: Invalid slave address for RTC (0x%x)\n",
			normal_addr[0]);
		return -EINVAL;
	}

	retval = i2c_add_driver(&pcf_driver);
	if (retval) return retval;
	retval = misc_register (&pcf_rtc_miscdev);
	if (retval) {
		i2c_del_driver(&pcf_driver);
		return retval;
	}

	printk("I2C: PCF8563 RTC driver successfully loaded\n");

	/* set system time from HC time */
	spin_lock_irqsave(&pcf_rtc_lock, flags);
	pcf_get_datetime(pcf_i2c_client, &wtime);
	spin_unlock_irqrestore(&pcf_rtc_lock,flags);
	
	t = mktime(wtime.tm_year+1900, wtime.tm_mon+1, wtime.tm_mday,
		   wtime.tm_hour, wtime.tm_min, wtime.tm_sec);
	xtime.tv_sec = t;
	printk("RTC: system time set to %i-%02i-%02i %02i:%02i:%02i UTC\n",
	       wtime.tm_year+1900, wtime.tm_mon+1, wtime.tm_mday,
	       wtime.tm_hour, wtime.tm_min, wtime.tm_sec);

	return 0;
}

static __exit void pcf_exit(void)
{
	misc_deregister(&pcf_rtc_miscdev);
	i2c_del_driver(&pcf_driver);
}

module_init(pcf_init);
module_exit(pcf_exit);

MODULE_PARM (slave_address, "i");
MODULE_PARM_DESC (slave_address, "I2C slave address for iPCF8563 RTC.");

MODULE_AUTHOR ("Alessandro Rubini");
MODULE_LICENSE("GPL");
