/*
 *  linux/drivers/char/pxa-ssp-oki.c
 *
 *  Author: Raffaele Recalcati 
 *  Created: 2007
 *  Copyright: BTicino S.p.A.
 *
 *  Driver for interfacing with Zarlink acustic canceller
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 * 
 */

#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/timer.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/config.h>
#include <linux/version.h>
#include <linux/rtc.h>
#include <linux/string.h>
#include <linux/proc_fs.h>
#include <asm/hardware.h> /* for btweb_globals when ARCH_BTWEB is set */

#include <asm/mach-types.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <asm/hardware.h>
#ifdef CONFIG_ARCH_PXA
#include <asm/arch/pxa-regs.h>
#endif
 
/* DEBUG */
#define DEBUG 

#define OKI_MAJOR 55

#ifdef DEBUG
        #define deb(format, arg...) printk(KERN_DEBUG __FILE__ ": " format "\n" , ## arg)
#else
        #define deb(format, arg...) do {} while (0)
#endif

#define trace(format, arg...) printk(KERN_INFO __FILE__ ": " format "\n" , ## arg)

/* DEFINES */
#define PXA_SSP_WRITE 5500
#define PXA_SSP_READ  5501
#define CSON    GPCR(5)= GPIO_bit(5);
#define CSOFF   GPSR(5)= GPIO_bit(5);

static int ctrlssp_oki_active=0;

static void ctrlssp_set_gpio(void)
{
	set_GPIO_mode(GPIO23_SCLK_md); /* lowercase: buglet in pxa-regs.h */
/*	set_GPIO_mode(GPIO24_SFRM_MD); */
	set_GPIO_mode(GPIO25_STXD_MD);
	set_GPIO_mode(GPIO26_SRXD_MD);
}



static int
ioctl_ssp_oki( struct inode *inode, struct file *file,
                unsigned int cmd, unsigned long arg)
{
        unsigned short int val=0; /* per wr su SSP */
        unsigned char  ssdr;      /* per rd da SSP */


/*	if (copy_from_user(&val, (unsigned short int*)arg, sizeof(val))){
                deb("copy_from_user error \n");
		return -EFAULT;
	}*/
	if (cmd & 0x80000000) /* reset */
	{
        	GPCR(4) = GPIO_bit(4); /* reset basso */
	        udelay(10);
        	GPSR(4) = GPIO_bit(4); /* rilascio reset */
	        udelay(10); 
		deb("ioctl_ssp: RST done\n");
		return 0;
	}

	CSON;
	udelay(1);
	deb("ioctl_ssp:wr %x to CSON\n",cmd); /*val);*/
	SSDR = 0xFFFF & cmd; /*val;*/
	udelay(7);
			
	ssdr=0x00ff&SSDR;
	udelay(7);
	CSOFF;
	udelay(7);

	if ( (val & 0x8000) != 0 ) /* read */
	{
		deb("ioctl_ssp:rd %x from CSON\n",ssdr);

		if(copy_to_user((void *)arg, &ssdr, sizeof(ssdr))) {
			deb("ioctl_ssp: no copytouser\n");
			return -EFAULT;
		}
	}
	return 0;
}


static int open_ssp_oki(struct inode *inode, struct file *filp)
{
	int minor = MINOR(inode->i_rdev);
        deb(KERN_INFO "CTRLSSP opening: minor=%d\n",minor);

	if(ctrlssp_oki_active>0){
		printk(KERN_INFO "open_ssp not done - already opened\n");
		return -EBUSY;
	}
	ctrlssp_oki_active++;
	deb(KERN_INFO "open_ssp done\n");
	return 0;
}

static int release_ssp_oki(struct inode *inode, struct file *filp)
{
	ctrlssp_oki_active--;
	return 0;
}

static struct file_operations ctrlssp_oki_fops = {
        owner:    THIS_MODULE,
	ioctl:	  ioctl_ssp_oki,
        open:     open_ssp_oki,
        release:  release_ssp_oki, 
};

int __init ctrlssp_oki_init(void)
{
	unsigned int sscr;

	ctrlssp_set_gpio();
        CSOFF;

        GPCR(4) = GPIO_bit(4); /* reset basso */
        udelay(10);
        GPSR(4) = GPIO_bit(4); /* rilascio reset */
        udelay(10); 

	SSSR = 0x0080;   /* ROR bit reset -- Receiver Overrun Status */
        SSCR0 = 0x000F;  /* 1.84MHz, int CK, Motorola SPI, 16 bit */
	SSCR1 = 0x0000;  /* sclk idle low, sample on r edge, drive on f edge */
        CKEN |= CKEN3_SSP;
	sscr = SSCR0;
	sscr |= (1<<7);  /* SSE bit set -- SSP enabled */
	SSCR0 = sscr;

        if (register_chrdev(OKI_MAJOR, "oki", &ctrlssp_oki_fops))
        {
                deb(KERN_NOTICE "Can't allocate major number %d for OKI AEC.\n",
                       OKI_MAJOR);
                return -EAGAIN;
        }

	deb(KERN_INFO "ctrlssp_oki ssp Driver\n");
	return 0;
}
 
void __exit ctrlssp_oki_exit(void)
{
	unsigned int sscr;

	unregister_chrdev(OKI_MAJOR, "oki");

	/* return to RESET state TODO*/
        CSOFF;
	GPCR(4)= GPIO_bit(4);

	sscr = SSCR0;
	sscr &= ~(1<<7);
	SSCR0 = sscr;
	CKEN &= ~(CKEN3_SSP);
}

module_init(ctrlssp_oki_init);
module_exit(ctrlssp_oki_exit);

MODULE_AUTHOR("Raffaele Recalcati e Roberto Cerati");
MODULE_DESCRIPTION("SSP for oki control driver");

EXPORT_NO_SYMBOLS;
