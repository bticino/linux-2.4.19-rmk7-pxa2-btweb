/*
 * btcfg.c -- driver for Bticino configurators reader via i2c
 *
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
static struct i2c_client *conf_client;

static struct i2c_driver conf_driver;

static unsigned short ignore[] = { I2C_CLIENT_END };
static unsigned short normal_addr[] = { 0x38, I2C_CLIENT_END };

static struct i2c_client_address_data addr_data = {
	.normal_i2c =       normal_addr,
	.normal_i2c_range = ignore,
	.probe =            ignore,
	.probe_range =      ignore,
	.ignore =           ignore,
	.ignore_range =     ignore,
	.force =            ignore,
};


static int conf_attach(struct i2c_adapter *adap, int addr,
			 unsigned short flags,int kind)
{
	struct i2c_client *c;

	c = (struct i2c_client *)kmalloc(sizeof(*c), GFP_KERNEL);
	if (!c)
		return -ENOMEM;
	memset (c, 0, sizeof(*c));

	strcpy(c->name, conf_driver.name);
	c->id = conf_driver.id;
	c->flags = 0;
	c->addr = addr;
	c->adapter = adap;
	c->driver = &conf_driver;
	c->data = NULL;

	conf_client = c;

	return i2c_attach_client(c);
}

static int conf_probe(struct i2c_adapter *adap)
{
	return i2c_probe(adap, &addr_data, conf_attach);
}

static int conf_detach(struct i2c_client *client)
{
	i2c_detach_client(client);
	return 0;
}

static struct i2c_driver conf_driver = {
	.name =            "btcfg",
	.id =              I2C_DRIVERID_BTCFG,
	.flags =           I2C_DF_NOTIFY,
	.attach_adapter =  conf_probe,
	.detach_client =   conf_detach,
};




ssize_t conf_read(struct file *filp, char *ubuff, size_t count, loff_t *offp)
{
	unsigned long off = (unsigned long)*offp;
	unsigned long timeout = jiffies + 1 + HZ/10;
	unsigned char data[18]; /* read at most 16 bytes */
	int ret, exp;

	if (off >= 0xff) return 0;
//printk("\n\n\n\n\n\nREAD\n\n\n\ndata[0]=%d\n\n\n\n",data[0]);
	if (count > 16) count = 16;

 again:
	data[0] = (char) (off & 0xff);
	exp = 1;
//printk("\n\n\n\n\n\nREAD-1\n\n\n\ndata[0]=%d\n\n\n\n",data[0]);
	ret = i2c_master_send(conf_client, &data[0], 1);
//printk("\n\n\n\n\n\nREAD-2\n\n\n\n\n\n\n");
	if (ret == exp) {
		exp = count;
		ret = i2c_master_recv(conf_client, data, count);
	}
//printk("\n\n\n\n\n\nREAD-3\n\n\n\n\n\n\n");

	if (ret != exp) {
		/* the device might not be ready; yield and retry */
		if (time_after(jiffies, timeout))
			return -EIO;
		schedule();
		goto again;
	}
	__copy_to_user(ubuff, data, count);
	*offp += count;
	return count;
}

ssize_t conf_write(struct file *filp, const char *ubuff,
		     size_t count, loff_t *offp)
{
	unsigned long off = (unsigned long)*offp;
	unsigned long timeout = jiffies + 1 + HZ/10;
	unsigned char data[18]; /* address plus page-size */
	int ret, exp;
//printk("BTCONFWRITE off=%d count=%d",off,count);
	if (off >= 0xff) return -ENOSPC;


 again:
	/* send the offset and data together */
	data[0] = off & 0xff;
	if (count > 16) count = 16;
	__copy_from_user(data+1, ubuff, count);

//printk("BTCONFWRITE - 1: count=%d \ndata:%x %x %x\n\n",count,data[0],data[1],data[2]);

	exp = 1+count;
	ret = i2c_master_send(conf_client, data, exp);


	if (ret != exp) {
		/* the device might not be ready; yield and retry */
		if (time_after(jiffies, timeout))
			return -EIO;
		schedule();
		goto again;
	}
	*offp += count;

	return count;
}

struct file_operations conf_fops = {
	.read = conf_read,
	.write = conf_write,
};

struct miscdevice conf_misc = {
	.minor = BTCFG_MINOR,
	.name = "btcfg",
	.fops = &conf_fops,
};

static __init int conf_init(void)
{
	int ret = misc_register(&conf_misc);
	if (ret) {
		printk("btweb: can't register /dev/btcfg (error %i)\n",
		       ret);
		return ret;
	}
	ret = i2c_add_driver(&conf_driver);
	if (ret) {
		misc_deregister(&conf_misc);
		return ret;
	}
	printk("I2C: BTCFG driver successfully loaded \n");
	return ret;
}

static __exit void conf_exit(void)
{
	misc_deregister(&conf_misc);
	i2c_del_driver(&conf_driver);
}

module_init(conf_init);
module_exit(conf_exit);
MODULE_LICENSE("GPL");

