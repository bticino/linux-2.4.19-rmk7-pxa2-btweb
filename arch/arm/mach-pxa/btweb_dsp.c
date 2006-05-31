/*
 * drivers/char/btweb_dsp
 *
 * Author: Alessandro Rubini
 * Contributors: Recalcati
 *
 * Moved from sa1110 to pxa255 by Raffaele Recalcati
 * This file contains communication to dsp5402 card
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/config.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/tty.h>
#include <linux/version.h>

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/errno.h>

#include <linux/serial_core.h>
#include <linux/miscdevice.h>
#include <linux/delay.h>

#include <asm/hardware.h>
#include <asm/setup.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include <asm/uaccess.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/io.h> /* for DSP operations */
#include <asm-arm/arch-pxa/pxa-regs.h> 

#include <linux/string.h>


/*#include "generic.h" */

#define DSP_BOB		  0x0101		  /* set little endian data transfer */
#define DSP_DSPINT	0x0404		/* host to DSP interrupt */
#define DSP_HINT		0x0808		/* DSP to host interrupt */

static unsigned short *dsp_iopage;

int dsp_reset(struct inode *inode, struct file *file, unsigned cmd,unsigned long arg)
{
	/* ignore any argument, just do reset here */
	/* raise reset */
	GPSR(73) |= GPIO_bit(73);
	
/*	udelay(1); DAVEDE */ /* wait 1/7.3728MHz = 135.6ns (par 11.13.4) */

	/* sleep 1/10 second */
  set_current_state(TASK_INTERRUPTIBLE);
	schedule_timeout(HZ/10);

/* Same PIOs of sa1110 */
	GPSR(19) |= GPIO_bit(19);
	GPCR(17) |= GPIO_bit(17);

	/* sleep 2/10 second */
  set_current_state(TASK_INTERRUPTIBLE);
	schedule_timeout(2*HZ/10);
	/* lower reset */
	GPCR(73) |= GPIO_bit(73);
	
  set_current_state(TASK_INTERRUPTIBLE);
	schedule_timeout(2*HZ/10);

  writew(DSP_BOB, (unsigned short *)dsp_iopage); // little endian

	return 0;
}

int dsp_open(struct inode *inode, struct file *file)
{
	if (dsp_iopage) return 0;

/*	if (GPLR & GPIO_GPIO(1)) { OLD */
	if (GPLR(0) & GPIO_bit(1)) {   
		printk("btweb: no DSP device found, continuing anyways\n");
		return -ENODEV;
	}
	
	dsp_iopage = (unsigned short *)ioremap(0x20000000, PAGE_SIZE);
	if (!dsp_iopage) {
		printk("btweb: ioremap failed\n");
		return -ENODEV;
	}

  /* first open, force a reset */
	dsp_reset(inode, file, 0, 0);
	
  return 0;

}

#define DSP_SIZE (16*1024*2)		// (16K-0x80) x 16 bit on-chip DARAM

#define DSP_STEP  2  /* This means HPIA is incremented each word//byte */
#define DSP_HPID_A  (dsp_iopage + 4)  /* Data register with auto-increment */
#define DSP_HPIA    (dsp_iopage + 8)  /* Address register */
#define DSP_HPID    (dsp_iopage + 12) /* Data register, HPIA is not affected */
#define DARAM_INIT 0x0000             /* word address */

ssize_t dsp_read(struct file *filp, char *ubuff, size_t count, loff_t *offp)
{
	unsigned long off = (unsigned long)*offp;
	unsigned short data[64],ris; /* read at most 64 words at a time */
	int i;

	if (off >= DSP_SIZE) return 0;

	if (count > 128) count = 128;
	if (off + count > DSP_SIZE) count = DSP_SIZE - off;
	if (count & 1) count &= ~1; /* must be even */

	off += DARAM_INIT * DSP_STEP;
	writew(off/DSP_STEP, DSP_HPIA);
	ris = readw(DSP_HPIA);

	for (i = 0; i < count/2; i++){
		data[i] = readw(DSP_HPID_A);
	}
	copy_to_user(ubuff, data, count);
	*offp += count;

	return count;
}

ssize_t dsp_write(struct file *filp, const char *ubuff,
		     size_t count, loff_t *offp)
{
	unsigned long off = (unsigned long)*offp;
	unsigned short data[64]; /* read at most 64 words at a time */
	unsigned short val;
	int i;

	if (off >= DSP_SIZE) return 0;

	if (count > 128) count = 128;
	if (off + count > DSP_SIZE) count = DSP_SIZE - off;
	if (count & 1) count &= ~1; /* must be even */

	copy_from_user(data, ubuff, count);

	off+=(DARAM_INIT-1)*DSP_STEP;
	writew( off/DSP_STEP, DSP_HPIA);

	/* check if DSP_BOB is lost, and if so re-set it correctly */
	val = readw(dsp_iopage);
	if ((val & DSP_BOB) !=DSP_BOB)
	    writew(DSP_BOB, (unsigned short *)dsp_iopage);

	for (i = 0; i < count/2; i++){
		writew(data[i], DSP_HPID_A);
	}
	*offp += count;

	return count;
}

struct file_operations dsp_fops = {
	.open = dsp_open,
	.read = dsp_read,
	.write = dsp_write,
	.ioctl = dsp_reset,
};

struct miscdevice dsp_misc = {
	.minor = 127 /* an unused minor, it seems */,
	.name = "dsp",
	.fops = &dsp_fops,
};

static int init_module(void)
{
/*	u32 reg; */
	int result=1;

	result = misc_register(&dsp_misc);

	printk("btpxa_dsp 2.0: dsp_init\n"); 

	if (result)
		printk("btweb: can't register /dev/dsp (error %i)\n",
		       result);
/*	reg = GPDR; */
/*	printk("btweb: dsp_init reg=%x letto\n",reg); DAVEDE */
/*	reg &= ~GPIO_GPIO(1); */ /* GPIO1 is input */
	GPDR(1) &= ~GPIO_bit(1);  /* TODO: Da mettere nel boot loader */
  
/*	reg |= GPIO_GPIO(17) | GPIO_GPIO(19); */ /* 17 and 19 are output DAVEDE */   

/* TODO: Da mettere nel boot loader */
	GPDR(19) |= GPIO_bit(19); 
	GPDR(17) |= GPIO_bit(17); 
	GPDR(73) |= GPIO_bit(73); 

/*	GPDR = reg; */
/*	printk("btweb: dsp_init reg=%x scritto\n",reg); DAVEDE */

/* TODO: Da mettere nel boot loader */
        MCIO0 = 0x00018103;
	MCIO1 = 0x00018103;
	MECR =  0x00000001;

	return result;
}

void cleanup_module(void)
{
  return;
}


