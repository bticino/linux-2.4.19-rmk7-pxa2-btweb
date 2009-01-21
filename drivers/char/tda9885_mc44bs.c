/*
 * linux/drivers/char/tda9885_mc44bs.c
 *
 * Author: Raffaele Recalcati 
 * Created: 2006
 * Copyright: BTicino S.p.A.
 *
 * Driver for TDA9885 and MC44BS, as found in the PXA F453AV BTicino device
 * These chips don't require data address to be sent for read and write session:
 * - write 4 bytes
 * - read 1 byte (status)
 * - change os chip address made by ioctl
 * 
 * Derived from pcf8563.c Copyright (C) 2003 Alessandro Rubini
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

/* Disabled default */
static int tda_power=0;
static int mc_power=0;


#define DEBUG 3

#if DEBUG
static unsigned int tdamc_debug = DEBUG;
#else
#define pdamc_debug 0	/* gcc will remove all the debug code for us */
#endif

#define SLAVE_ADDRESS_TDA 0X43
#define SLAVE_ADDRESS_MC  0X65

static unsigned short slave_address = 0;

struct i2c_driver tdamc_driver;
struct i2c_client *tdamc_i2c_client = 0;

static unsigned short ignore[] = { I2C_CLIENT_END };
static unsigned short normal_addr[] = { SLAVE_ADDRESS_TDA, I2C_CLIENT_END };

static struct i2c_client_address_data addr_data = {
	normal_i2c:		normal_addr,
	normal_i2c_range:	ignore,
	probe:			ignore,
	probe_range:		ignore,
	ignore: 		ignore,
	ignore_range:		ignore,
	force:			ignore,
};


static int tdamc_ioctl( struct inode *, struct file *, unsigned int, unsigned long);
static int tdamc_probe(struct i2c_adapter *adap);
static int tdamc_detach(struct i2c_client *client);


struct i2c_driver tdamc_driver = {
	name:		"TDA9885 and MC44BS",
	id:		80 /* should be ?I2C_DRIVERID_PCF */,
	flags:		I2C_DF_NOTIFY,
	attach_adapter: tdamc_probe,
	detach_client:	tdamc_detach,
	//command:	tdamc_command
};


static int
tdamc_attach(struct i2c_adapter *adap, int addr, unsigned short flags,int kind)
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

	strcpy(c->name, "TDA988_MC44BS");
	c->id		= tdamc_driver.id;
	c->flags	= 0;
	c->addr 	= addr;
	c->adapter	= adap;
	c->driver	= &tdamc_driver;
	c->data 	= NULL;

	ret = i2c_transfer(c->adapter, msgs, 2);

	if (ret != 2)
		printk ("tdamc_attach(): i2c_transfer() returned %d.\n",ret);
	tdamc_i2c_client = c;
	
	return i2c_attach_client(c);
}

static int
tdamc_probe(struct i2c_adapter *adap)
{
	return i2c_probe(adap, &addr_data, tdamc_attach);
}

static int
tdamc_detach(struct i2c_client *client)
{
	i2c_detach_client(client);

	return 0;
}

ssize_t tdamc_get_status(struct file *filp, char *ubuff, size_t count, loff_t *offp)
{
	unsigned long off = (unsigned long)*offp;
	unsigned long timeout = jiffies + 1 + HZ/10;
	unsigned char data[64]; /* read at most 64 bytes */
	int ret, exp;

	if (off > 0x0) return 0;
	count=1;
	exp=1;

        if (slave_address==0) return -ENXIO;

 again:
	/* TDA9885 and MC44BS don't require address like eepromS to read data */
	/* so I don't use i2c_master_send                                     */
	/*	data[0] = 0;                                                  */
	/*	exp = 1;                                                      */
	/*	ret = i2c_master_send(tdamc_i2c_client, data, 1);             */
	/*	if (ret == exp) {                                             */
	/*		exp = count;                                          */

	ret = i2c_master_recv(tdamc_i2c_client, data, count);
	if (ret != exp) {
		/* tda9885 or mc44bs might not be ready; yield and retry */
		if (time_after(jiffies, timeout))
			return -EIO;
		schedule();
		goto again;
	}

	if (tdamc_debug)
	  printk("tdamc_get_status: data=%x\n",data[0]);

	__copy_to_user(ubuff, data, count);
	//	*offp += count;
	return count;
}


ssize_t tdamc_set_status(struct file *filp, const char *ubuff,
		     size_t count, loff_t *offp)
{
	unsigned long off = (unsigned long)*offp;
	unsigned long timeout = jiffies + 1 + HZ/10;
	unsigned char data[66]; /* address plus page-size */
	int ret, exp;

        if (slave_address==0) return -ENXIO;

	if (off > 0x0) return -ENOSPC;
	count=4;
	exp=4;

 again:
	/* TDA9885 and MC44BS don't require address like eepromS to write data */
	/* data[0] = off >> 8;                                                 */
	/* data[1] = off & 0xff;                                               */
	/* if (count > 64) count = 64;                                         */
	/* if ((off ^ (off + count -1)) & ~63)                                 */
	/*	count = ((off + 64) & ~63) - off;                              */

	__copy_from_user(data, ubuff, count);
	
	if (tdamc_debug)
	  printk("tdamc_set_status: data=%x.%x.%x.%x\n",data[0],data[1],data[2],data[3]);

	exp = count;
	ret = i2c_master_send(tdamc_i2c_client, data, count);

	if (ret != exp) {
		/* tda9885 or mc44bs might not be ready; yield and retry */
		if (time_after(jiffies, timeout))
			return -EIO;
		schedule();
		goto again;
	}
	*offp += count;

 return count;
}

static struct file_operations tdamc_fops = {
        owner:          THIS_MODULE,
        ioctl:          tdamc_ioctl,
        read:           tdamc_get_status,
        write:          tdamc_set_status,
                                                                                                                 
};
static struct miscdevice tdamc_miscdev = {
        TDAMC_MINOR,
        "tdamc",
        &tdamc_fops
};

static int
tdamc_ioctl( struct inode *inode, struct file *file,
		unsigned int cmd, unsigned long arg)
{

	switch (cmd) {
		default:
 		case 5500:
			tda_power=1;
                        GPSR(btweb_features.abil_dem_video) =
                                GPIO_bit(btweb_features.abil_dem_video);
                        slave_address = SLAVE_ADDRESS_TDA;
		        normal_addr[0] = slave_address;
                        printk("tda9885_mc44bs: tda9885 registered and selected tda \n");
			tdamc_i2c_client->addr=slave_address;
		break;
                case 5501:
                        mc_power=1;
                        GPSR(btweb_features.abil_mod_video) =
                                GPIO_bit(btweb_features.abil_mod_video);
                        slave_address = SLAVE_ADDRESS_MC;
                        normal_addr[0] = slave_address;
                        printk("tda9885_mc44bs: tda9885 registered and selected mc \n");
                        tdamc_i2c_client->addr=slave_address;
                break;
		case 5502:
			printk("tdamc9885_mc44bs.c: Switching off mod/demod power\n");
//			if (tda_power) {
				GPCR(btweb_features.abil_dem_video) =
					GPIO_bit(btweb_features.abil_dem_video);
//			}
//			if (mc_power) {
				GPCR(btweb_features.abil_mod_video) =
					GPIO_bit(btweb_features.abil_mod_video);
//			}

			slave_address=0;
		        mc_power=0; tda_power=0;
                        normal_addr[0] = slave_address;
		break;
	}

	return 0;
}


static __init int tdamc_init(void)
{
	int retval=0;

	printk(KERN_ERR"tdamc_init mc_power=%d (otherwise power up tda for attach only)\n",mc_power);
	printk(KERN_ERR"btweb_features.abil_dem_video=%d,btweb_features.abil_mod_video=%d\n",btweb_features.abil_dem_video,btweb_features.abil_mod_video);

	if ( (!mc_power) ){
	        /* Switching on demodulator only for i2c attach */
        	GPSR(btweb_features.abil_dem_video) =
                	GPIO_bit(btweb_features.abil_dem_video);
	        slave_address = SLAVE_ADDRESS_TDA;
	}
	else{
		/* Switching on modulator only for i2c attach */
		GPSR(btweb_features.abil_mod_video) =
			GPIO_bit(btweb_features.abil_mod_video);
		slave_address = SLAVE_ADDRESS_MC;
	}

        normal_addr[0] = slave_address;

        if(normal_addr[0] >= 0x80)
        {
                printk(KERN_ERR"I2C: Invalid slave address for TDA (0x%x)\n",
                        normal_addr[0]);
                return -EINVAL;
        }
	udelay(1000);

	retval = i2c_add_driver(&tdamc_driver);
	if (retval) return retval;
	retval = misc_register (&tdamc_miscdev);
	if (retval) {
		i2c_del_driver(&tdamc_driver);
		return retval;
	}
	printk("I2C: TDA9885-MC44BS driver successfully loaded\n");

        if ( (!mc_power) ){
                /* Switching on demodulator only for i2c attach */
                GPCR(btweb_features.abil_dem_video) =
                        GPIO_bit(btweb_features.abil_dem_video);
                slave_address = SLAVE_ADDRESS_TDA;
        }
        else{
                /* Switching on modulator only for i2c attach */
                GPCR(btweb_features.abil_mod_video) =
                        GPIO_bit(btweb_features.abil_mod_video);
                slave_address = SLAVE_ADDRESS_MC;
        }

	slave_address=0;
	normal_addr[0] = 0;

	return 0;
}

static __exit void tdamc_exit(void)
{
	misc_deregister(&tdamc_miscdev);
	i2c_del_driver(&tdamc_driver);
	
	printk("tdamc9885_mc44bs.c: Switching off mod/demod power\n");

	/* Switching off mod and demod*/
//        if (tda_power) {
                GPCR(btweb_features.abil_dem_video) =
                        GPIO_bit(btweb_features.abil_dem_video);
//        }
//        if (mc_power) {
                GPCR(btweb_features.abil_mod_video) =
                        GPIO_bit(btweb_features.abil_mod_video);
//        }

}

module_init(tdamc_init);
module_exit(tdamc_exit);

MODULE_PARM (slave_address, "i");
MODULE_PARM_DESC (slave_address, "I2C slave address for TDA9885 and MC44BS chips.");
MODULE_PARM (tda_power, "i");
MODULE_PARM_DESC(tda_power, "TDA9885 demodulator power (default=0 off)");
MODULE_PARM (mc_power, "i");
MODULE_PARM_DESC(mc_power, "MC44BS modulator power (default=0 off)");

MODULE_AUTHOR ("Raffaele Recalcati");
MODULE_LICENSE("GPL");
