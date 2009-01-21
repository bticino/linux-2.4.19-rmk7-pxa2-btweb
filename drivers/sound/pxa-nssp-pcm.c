/*
 * linux/drivers/sound/pxa-nssp-pcm.c
 *
 * Author: Raffaele Recalcati 
 * Created: 2007
 * Copyright: BTicino S.p.A.
 *
 * Driver initializing tdm 32 channels (pcm telephon audio bus)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
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

#include <asm/mach-types.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <asm/hardware.h>
#ifdef CONFIG_ARCH_PXA
#include <asm/arch/pxa-regs.h>
#endif

static void ctrlnssp_set_gpio(void)
{
	set_GPIO_mode(GPIO81_NSSP_CLK_IN); 
	set_GPIO_mode(GPIO82_NSSP_FRM_OUT); 
	set_GPIO_mode(GPIO83_NSSP_RX);
	set_GPIO_mode(GPIO84_NSSP_TX);
}


int ctrlnssp_active=0;

static int open_ssp(struct inode *inode, struct file *filp)
{
	if(ctrlnssp_active){
		printk(KERN_INFO "NO\n");
		return -EBUSY;
	}
	ctrlnssp_active++;
	printk(KERN_INFO "YES\n");
	
	return 0;
}

static int release_nssp(struct inode *inode, struct file *filp)
{
	ctrlnssp_active--;
	return 0;
}

static struct file_operations ctrlnssp_fops = {
        owner:    THIS_MODULE,
/*        read:     read_ssp, */
/*	write:	  write_ssp, */
/*        open:     open_ssp, */
        release:  release_nssp, 
};

static struct miscdevice ctrlssp_tpanel = {
	0, "pcm0", &ctrlnssp_fops
};

int __init ctrlnssp_init(void)
{
	unsigned int sscr;

	ctrlnssp_set_gpio();

	/* Enabling FRM divisor shift register */
        GPCR(btweb_features.pbx_fs_res)= GPIO_bit(btweb_features.pbx_fs_res);

	SSCR0_P2 = 0x3F;  /* 0x11yy=100000 Hz */
	SSCR1_P2 = 0x22800018;  /* RWOT set, LBM loopback */
        SSPSP_P2 = 0x207; /*  */
	CKEN |= CKEN9_NSSP;
	sscr = SSCR0_P2;
	sscr |= (1<<7);
	SSCR0_P2 = sscr;

#if 0 
        if (register_chrdev(100, "voice", &ctrlssp_fops))
        {
                printk(KERN_NOTICE "Can't allocate major number %d for Codecs.\n",
                       100);
                return -EAGAIN;
        }
#endif

	printk(KERN_INFO "pxa-nssp-pcm driver initialized\n");
	return 0;
}
 
void __exit ctrlnssp_exit(void)
{
	unsigned int sscr;
#if 0 
	unregister_chrdev(100, "voice");
#endif


	sscr = SSCR0_P2;
	sscr &= ~(1<<7);
	SSCR0_P2 = sscr;
	CKEN &= ~(CKEN9_NSSP);
}

module_init(ctrlnssp_init);
module_exit(ctrlnssp_exit);

MODULE_AUTHOR("Raffaele Recalcati");
MODULE_DESCRIPTION("pxa-nssp-pcm data driver");

EXPORT_NO_SYMBOLS;
