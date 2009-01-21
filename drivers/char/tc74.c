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

#include <asm/uaccess.h>
#include <asm/hardware.h>

/* assume we have only one */
static struct i2c_client *temp_client;

static struct i2c_driver temp_driver;

static unsigned short ignore[] = { I2C_CLIENT_END };
static unsigned short normal_addr[] = { 0x49, I2C_CLIENT_END };

static struct i2c_client_address_data addr_data = {
	.normal_i2c =       normal_addr,
	.normal_i2c_range = ignore,
	.probe =            ignore,
	.probe_range =      ignore,
	.ignore =           ignore,
	.ignore_range =     ignore,
	.force =            ignore,
};

static int temp_attach(struct i2c_adapter *adap, int addr,
			 unsigned short flags,int kind)
{
	unsigned char bytes[2] = {0,};
	struct i2c_client *c;
	int ret;

	c = (struct i2c_client *)kmalloc(sizeof(*c), GFP_KERNEL);
	if (!c)
		return -ENOMEM;
	memset (c, 0, sizeof(*c));

	strcpy(c->name, temp_driver.name);
	c->id = temp_driver.id;
	c->flags = 0;
	c->addr = addr;
	c->adapter = adap;
	c->driver = &temp_driver;
	c->data = NULL;

	temp_client = c;

	ret = i2c_attach_client(c);
	/* place it out of standby -- just in case */
	if (!ret) i2c_master_send(temp_client, bytes, 2);
	return ret;
}

static int temp_probe(struct i2c_adapter *adap)
{
	return i2c_probe(adap, &addr_data, temp_attach);
}

static int temp_detach(struct i2c_client *client)
{
	i2c_detach_client(client);
	return 0;
}

static struct i2c_driver temp_driver = {
	.name =            "TC74",
	.id =              82 /* should be I2C_DRIVERID_TC74 */,
	.flags =           I2C_DF_NOTIFY,
	.attach_adapter =  temp_probe,
	.detach_client =   temp_detach,
};




ssize_t temp_read(struct file *filp, char *ubuff, size_t count, loff_t *offp)
{
	unsigned char bytes[2] = {0,};
	char result[8];

	/* behave naively: if not-beginning-of-file return end-of-file */
	if (*offp) return 0;

	/* Assume that it is not in standby and that data is ready */
	i2c_master_send(temp_client, bytes, 1);
	i2c_master_recv(temp_client, bytes+1, 1);
       sprintf(result, "%i\n", (signed char)(bytes[1]));
       if (count < strlen(result)) return -EINVAL;
       count = strlen(result);
       __copy_to_user(ubuff, result, count);
       *offp += count;
       return count;
}

struct file_operations temp_fops = {
	.read = temp_read,
};

struct miscdevice temp_misc = {
	.minor = TEMP_MINOR,
	.name = "temperature",
	.fops = &temp_fops,
};

static __init int temp_init(void)
{
	int ret = misc_register(&temp_misc);
	if (ret) {
		printk("btweb: can't register /dev/temperature (error %i)\n",
		       ret);
		return ret;
	}
	ret = i2c_add_driver(&temp_driver);
	if (ret) {
		misc_deregister(&temp_misc);
		return ret;
	}
	printk("I2C: TC74 thermometer driver successfully loaded\n");
	return ret;
}

static __exit void temp_exit(void)
{
	misc_deregister(&temp_misc);
	i2c_del_driver(&temp_driver);
}

module_init(temp_init);
module_exit(temp_exit);
