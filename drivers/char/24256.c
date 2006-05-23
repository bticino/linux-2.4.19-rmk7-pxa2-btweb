/*
 * 24256.c -- driver for BtWeb-mounted NVRAM
 *
 * Copyright (C) 2003,2004   Alessandro Rubini <rubini@linux.it>
 * Copyright (C) 2003,2004   BTicino.it
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/miscdevice.h>
#include <linux/delay.h>
#include <linux/timer.h>

#include <asm/uaccess.h>
#include <asm/hardware.h>

/* assume we have only one */
static struct i2c_client *eeprom_client;

static struct i2c_driver eeprom_driver;

static unsigned short ignore[] = { I2C_CLIENT_END };
static unsigned short normal_addr[] = { 0x53, I2C_CLIENT_END };

static struct i2c_client_address_data addr_data = {
	.normal_i2c =       normal_addr,
	.normal_i2c_range = ignore,
	.probe =            ignore,
	.probe_range =      ignore,
	.ignore =           ignore,
	.ignore_range =     ignore,
	.force =            ignore,
};

/* protect/unprotect with GPIO */
#define EEBIT            GPIO_bit(btweb_features.e2_wp)
#define PROTECT()       (GPSR(btweb_features.e2_wp) = EEBIT)
#define UNPROTECT()     (GPCR(btweb_features.e2_wp) = EEBIT)

/* use a timer to re-protect at a later time */
static void eeprom_protect (unsigned long unused)
{
	PROTECT();
}
#define TIMER_DELAY ((HZ+10)/20) /* 50ms */
static struct timer_list eeprom_timer;


static int eeprom_attach(struct i2c_adapter *adap, int addr,
			 unsigned short flags,int kind)
{
	struct i2c_client *c;

	c = (struct i2c_client *)kmalloc(sizeof(*c), GFP_KERNEL);
	if (!c)
		return -ENOMEM;
	memset (c, 0, sizeof(*c));

	strcpy(c->name, eeprom_driver.name);
	c->id = eeprom_driver.id;
	c->flags = 0;
	c->addr = addr;
	c->adapter = adap;
	c->driver = &eeprom_driver;
	c->data = NULL;

	eeprom_client = c;

	return i2c_attach_client(c);
}

static int eeprom_probe(struct i2c_adapter *adap)
{
	return i2c_probe(adap, &addr_data, eeprom_attach);
}

static int eeprom_detach(struct i2c_client *client)
{
	i2c_detach_client(client);
	return 0;
}

static struct i2c_driver eeprom_driver = {
	.name =            "24256",
	.id =              81 /* should be I2C_DRIVERID_24256 */,
	.flags =           I2C_DF_NOTIFY,
	.attach_adapter =  eeprom_probe,
	.detach_client =   eeprom_detach,
};




ssize_t eeprom_read(struct file *filp, char *ubuff, size_t count, loff_t *offp)
{
	unsigned long off = (unsigned long)*offp;
	unsigned long timeout = jiffies + 1 + HZ/10;
	unsigned char data[64]; /* read at most 64 bytes */
	int ret, exp;

	if (off >= 0x8000) return 0;

	if (count > 64) count = 64;
	if (off + count > 0x8000) count = 0x8000 - off;

 again:
	data[0] = off >> 8;
	data[1] = off & 0xff;
	exp = 2;
	ret = i2c_master_send(eeprom_client, data, 2);
	if (ret == exp) {
		exp = count;
		ret = i2c_master_recv(eeprom_client, data, count);
	}
	if (ret != exp) {
		/* the nvram might not be ready; yield and retry */
		if (time_after(jiffies, timeout))
			return -EIO;
		schedule();
		goto again;
	}
	__copy_to_user(ubuff, data, count);
	*offp += count;
	return count;
}

ssize_t eeprom_write(struct file *filp, const char *ubuff,
		     size_t count, loff_t *offp)
{
	unsigned long off = (unsigned long)*offp;
	unsigned long timeout = jiffies + 1 + HZ/10;
	unsigned char data[66]; /* address plus page-size */
	int ret, exp;

	if (off >= 0x8000) return -ENOSPC;

	del_timer(&eeprom_timer);
	/*  unprotect, and wait a while */
	UNPROTECT();
	udelay(100);

 again:
	/* send the offset and data together */
	data[0] = off >> 8;
	data[1] = off & 0xff;
	if (count > 64) count = 64;
	if ((off ^ (off + count -1)) & ~63)
		count = ((off + 64) & ~63) - off;
	__copy_from_user(data+2, ubuff, count);
	exp = 2+count;
	ret = i2c_master_send(eeprom_client, data, 2+count);

	if (ret != exp) {
		/* the nvram might not be ready; yield and retry */
		if (time_after(jiffies, timeout))
			return -EIO;
		schedule();
		goto again;
	}
	*offp += count;

	eeprom_timer.expires = jiffies + TIMER_DELAY;
	add_timer(&eeprom_timer);
	return count;
}

struct file_operations eeprom_fops = {
	.read = eeprom_read,
	.write = eeprom_write,
};

struct miscdevice eeprom_misc = {
	.minor = NVRAM_MINOR,
	.name = "nvram",
	.fops = &eeprom_fops,
};

static __init int eeprom_init(void)
{
	int ret = misc_register(&eeprom_misc);
	if (ret) {
		printk("btweb: can't register /dev/nvram (error %i)\n",
		       ret);
		return ret;
	}
	ret = i2c_add_driver(&eeprom_driver);
	if (ret) {
		misc_deregister(&eeprom_misc);
		return ret;
	}
	init_timer (&eeprom_timer);
	eeprom_timer.function = eeprom_protect;
	printk("I2C: 24256 NVRAM driver successfully loaded\n");
	return ret;
}

static __exit void eeprom_exit(void)
{
	del_timer(&eeprom_timer);
	misc_deregister(&eeprom_misc);
	i2c_del_driver(&eeprom_driver);
}

module_init(eeprom_init);
module_exit(eeprom_exit);
