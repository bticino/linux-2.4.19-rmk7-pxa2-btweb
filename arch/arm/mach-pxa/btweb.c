/*
 *  linux/arch/arm/mach-pxa/btweb.c
 *
 *  Support for the Intel DBPXA250 Development Platform.
 *  Adapted to the BTicino device family by Alessandro Rubini in 2004,2005
 *  
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */
#include <linux/config.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/major.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/bitops.h>

#include <linux/vt_kern.h> /* kd_mksound */

#include <linux/delay.h> 

#include <asm/types.h>
#include <asm/setup.h>
#include <asm/memory.h>
#include <asm/mach-types.h>
#include <asm/hardware.h>
#include <asm/irq.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/irq.h>

#include <asm/arch/irq.h>

#include "generic.h"

/* This part of the file deals with autodetection of the different flavors */
struct btweb_features btweb_features;
struct btweb_globals btweb_globals;

unsigned char __attribute__ ((aligned (4096))) btweb_bigbuf[256*1024];
unsigned char __attribute__ ((aligned (4096))) btweb_bigbuf1[256*1024];
EXPORT_SYMBOL(btweb_bigbuf);
EXPORT_SYMBOL(btweb_bigbuf1);
EXPORT_SYMBOL(btweb_features);
EXPORT_SYMBOL(btweb_globals);

static int __init is_f452(void) // and MH200
{
	struct btweb_features feat = {
		.eth_reset = 63, 
		.eth_irq = 23,
		.e2_wp = 70,
		.rtc_irq = 8,
		.led = 71, /* currently unused */
		.usb_irq = -1,
		.backlight = -1,
		.pic_reset = 62,
		.buzzer = 0,
		.mdcnfg = 0x19C9,
		.ctrl_hifi = -1,
		.ctrl_video = -1,
		.virt_conf = -1,
		.abil_mod_video = -1,
		.abil_mod_hifi = -1,
		.abil_fon = -1,
	};
	/* GPIO6 is low */
	set_GPIO_mode( 6 | GPIO_IN );
	if (GPLR(6) & GPIO_bit(6)) return -ENODEV;

	/* GPIO7 is high */
	set_GPIO_mode( 7 | GPIO_IN );
	if (!(GPLR(7) & GPIO_bit(7))) return -ENODEV;

	btweb_features = feat;
	return 0;
}
static int __init is_ts(void) // H4684 product
{
	struct btweb_features feat = {
		.eth_reset = 3, 
		.eth_irq = 22,
		.e2_wp = 10,
		.rtc_irq = 8,
		.led = -1,
		.usb_irq = 26,
		.backlight = 12,
		.pic_reset = 44,
		.buzzer = 1,
		.mdcnfg = 0x19C9,
		.ctrl_hifi = -1,
		.ctrl_video = -1,
		.virt_conf = -1,
		.abil_mod_video = -1,
		.abil_mod_hifi = -1,
		.abil_fon = -1,
	};
	/* GPIO6 is high (it's already in IN mode)  */
	if (!(GPLR(6) & GPIO_bit(6))) return -ENODEV;

	/* GPIO7 is low (it's already in IN mode) */
	if (GPLR(7) & GPIO_bit(7)) return -ENODEV;

	/* GPIO82 is low */
	set_GPIO_mode( 82 | GPIO_IN );
	if (GPLR(82) & GPIO_bit(82)) return -ENODEV;

	/* GPIO84 is low */
	set_GPIO_mode( 84 | GPIO_IN );
	if (GPLR(84) & GPIO_bit(84)) return -ENODEV;

	btweb_features = feat;
	return 0;
}
static int __init is_fpga(void)   // First master of F453AV/Interf2filitcp/ip
{
	struct btweb_features feat = {
		.eth_reset = 3,
		.eth_irq = 22,
		.e2_wp = 10,
		.rtc_irq = 8,
		.led = 40,
		.usb_irq = 21,
		.backlight = -1,
		.pic_reset = 44,
		.buzzer = -1,
		.mdcnfg = 0x19C9,
		.ctrl_hifi = 0,
		.ctrl_video = 1,
		.virt_conf = 2,
		.abil_mod_video = 35,
		.abil_mod_hifi = 41,
		.abil_fon = 54,
	};
	/* GPIO6 is high (it's already in IN mode)  */
	if (!(GPLR(6) & GPIO_bit(6))) return -ENODEV;

	/* GPIO7 is high (it's already in IN mode)  */
	if (!(GPLR(7) & GPIO_bit(7))) return -ENODEV;

	/* GPIO82 is high (it's already in IN mode)  */
	if (!(GPLR(82) & GPIO_bit(82))) return -ENODEV;

	/* GPIO84 is high (it's already in IN mode)  */
	if (!(GPLR(84) & GPIO_bit(84))) return -ENODEV;
	
	btweb_features = feat;
	return 0;
}

static int __init is_f453av(void)
{
	struct btweb_features feat = {
		.eth_reset = 3,
		.eth_irq = 22,
		.e2_wp = 10,
		.rtc_irq = 8,
		.led = 40,
		.usb_irq = 21,
		.backlight = -1,
		.pic_reset = 44,
		.buzzer = -1,
		.mdcnfg = 0x19C9,
		.ctrl_hifi = -1,
		.ctrl_video = 1,
		.virt_conf = 2,
		.abil_mod_video = 35,
		.abil_mod_hifi = 41,
		.abil_fon = 54,
	};

	/* GPIO6 is low (it's already in IN mode)  */
	if (GPLR(6) & GPIO_bit(6)) return -ENODEV;

	/* GPIO7 is low (it's already in IN mode)  */
	if (GPLR(7) & GPIO_bit(7)) return -ENODEV;

	/* GPIO82 is low (it's already in IN mode)  */
	if (GPLR(82) & GPIO_bit(82)) return -ENODEV;

	/* GPIO84 is high (it's already in IN mode)  */
	if (!(GPLR(84) & GPIO_bit(84))) return -ENODEV;
	

	btweb_features = feat;
	return 0;
}

static int memsize;

static struct btweb_flavor {
	int (*f)(void);
	int id;
	char *name;
	int memsize;
} fltab[] __initdata = {
	/* note: the order of detection is important */
	{is_f452,   BTWEB_F452, "F452X-MH200 web server",64<<20},
	{is_ts,     BTWEB_TS,   "Touch-screen engine",   64<<20},
	{is_fpga,   BTWEB_FPGA, "FPGA-enabled engine",   64<<20},
	{is_f453av, BTWEB_F453AV, "F453AV web server",64<<20},
	{NULL,}
};

static int __init btweb_find_device(void)
{
	
	
	struct btweb_flavor *fptr;
	for (fptr = fltab; fptr->f; fptr++)
		if (!fptr->f())
			break;
	if (!fptr->f) return -ENODEV;
	/* set up globals */
	strcpy(btweb_globals.name, fptr->name);
	btweb_globals.flavor = fptr->id;
	btweb_globals.last_touch = jiffies;
	memsize = fptr->memsize;

	/* Recover hwversion, too -- it may not be meaningul on some flavor */
	btweb_globals.hw_version =
		(GPLR(58) & GPIO_bit(58)) >> (58&0x1f) << 3 |
		(GPLR(63) & GPIO_bit(63)) >> (63&0x1f) << 2 |
		(GPLR(64) & GPIO_bit(64)) >> (64&0x1f) << 1 |
		(GPLR(69) & GPIO_bit(69)) >> (69&0x1f) << 0 ;

	printk("BtWeb: Detected \"%s\" device (hw version %i)\n",
	       btweb_globals.name, btweb_globals.hw_version);
	return 0;
}

/* The rest of the file is the conventional part */
static void __init btweb_init_irq(void)
{
	pxa_init_irq();
	kd_mksound = btweb_kd_mksound;
}

static int __init btweb_init(void)
{
	/* For USB. Can't do this in btweb_map_io, as it's too early */
	if (btweb_features.usb_irq >= 0){
		set_GPIO_IRQ_edge(btweb_features.usb_irq, GPIO_RISING_EDGE);
		printk("Setted usb wake up irq rising edge: %d\n",btweb_features.usb_irq);
	}
	/* Similarly, we beed I/O mapped before doing this */
	if (MDCNFG != btweb_features.mdcnfg) {
		printk("Warning: MDCNFG is 0x%08x, setting it to 0x%08x\n",
		       MDCNFG, btweb_features.mdcnfg);
		MDCNFG = btweb_features.mdcnfg;
	}
	return 0;
}

__initcall(btweb_init);


static void __init
fixup_btweb(struct machine_desc *desc, struct param_struct *params,
		char **cmdline, struct meminfo *mi)
{
	/* If this is an old bootloader, complain */
	if (machine_is_btweb_old()) {
		printk(KERN_ERR "BTWEB Warning: using old boot loader\n");
	}

	/* Detect the flavor of btweb device, from GPIO settings */
	if (btweb_find_device()) {
		printk("BtWeb: unknown device. Stopping\n");
		while (1);
	}

	/* Some boards have 32MB some 64MB.  Let's use a safe default */
	printk("btweb:memsize=%x\n",memsize); /* For debugging purpose */
	SET_BANK (0, 0xa0000000, memsize);
	mi->nr_banks      = 1;
}

static struct map_desc btweb_io_desc[] __initdata = {
 /* virtual     physical    length      domain     r  w  c  b */
  { 0xf1000000, 0x0c000000, 0x00100000, DOMAIN_IO, 0, 1, 0, 0 }, /* LAN91C96 and SMC9118 IO */
  { 0xf1100000, 0x0e000000, 0x00100000, DOMAIN_IO, 0, 1, 0, 0 }, /* LAN91C96 Attr */
  { 0xf4000000, 0x10000000, 0x00400000, DOMAIN_IO, 0, 1, 0, 0 }, /* SA1111 */
  LAST_DESC
};

static void __init btweb_map_io(void)
{
	pxa_map_io();
	iotable_init(btweb_io_desc);

	/* This enables the BTUART for Pic Multimedia*/
	CKEN |= CKEN7_BTUART;
	set_GPIO_mode(GPIO42_BTRXD_MD);
	set_GPIO_mode(GPIO43_BTTXD_MD);

	/* This enables the STUART for Pic Antintrusione*/
	CKEN |= CKEN5_STUART;
	set_GPIO_mode(GPIO46_STRXD_MD);
	set_GPIO_mode(GPIO47_STTXD_MD);

	/* This is for the SMC chip select */
	set_GPIO_mode(GPIO79_nCS_3_MD);

	/* To be sure that ethernet chip is not in reset state */
	set_GPIO_mode(btweb_features.eth_reset | GPIO_OUT);
	GPCR(btweb_features.eth_reset) = GPIO_bit(btweb_features.eth_reset);

	/* NVRAM write protect */
	set_GPIO_mode(btweb_features.e2_wp | GPIO_OUT);
	GPSR(btweb_features.e2_wp) = GPIO_bit(btweb_features.e2_wp);

	/* If we have a backlight, we have gpio54 as output, too */
	set_GPIO_mode(54 | GPIO_OUT);
	GPCR(54) = GPIO_bit(54);

	if ((BTWEB_FPGA==btweb_globals.flavor) || (BTWEB_F453AV==btweb_globals.flavor)) {

		printk("NOW SOME GPIO FIX FOR F453AV (TO BE PORTED TO UBOOT IN A near FUTURE\n");

		if (BTWEB_FPGA==btweb_globals.flavor)
			printk("First Master of F453AV named FPGA: keep only for compatibility\n");


		/* To be sure that pic is not in reset state */
		set_GPIO_mode(btweb_features.pic_reset | GPIO_OUT);
		GPSR(btweb_features.pic_reset) = GPIO_bit(btweb_features.pic_reset);

		/* Enabling video modulator/demodulator  */
		set_GPIO_mode(35 | GPIO_OUT);
		GPSR(35) = GPIO_bit(35);

		/* gpio0,1 hifi and video source selection*/
		set_GPIO_mode(0 | GPIO_OUT);
		GPSR(0) = GPIO_bit(0); /* hifi internal source */
		set_GPIO_mode(1 | GPIO_OUT);
		GPCR(1) = GPIO_bit(1); /* video internal source from tvia */

		/* some fix for Tvia */
		set_GPIO_mode(19 | GPIO_IN);
		/* mfill -b 0x40E0000C -l 0x4 -p 0xD381BE0B -4 */
		/*  GPCR1=0x2000; */
		/* mfill -b 0x40E00028 -l 0x4 -p 0x2000 -4 */
		/* sleep 1/10 second */
		/* set_current_state(TASK_INTERRUPTIBLE); */
		/* schedule_timeout(HZ/10); */
		/* GPSR1=0x2000; */
		/* set_current_state(TASK_INTERRUPTIBLE); */
		/* schedule_timeout(HZ/10); */

		/* mfill -b 0x40E0001C -l 0x4 -p 0x2000 -4 */
		/* bus */
		MSC2=0x92347FF1;    /* BUS veloce su chip select Tvia5202 */
		/* MSC2=0x23447FF1;    BUS medio */
		/* MSC2=0x7ff47FF1;    BUS lento */

		/* Reset Tvia5202 - gpio45 output */
		set_GPIO_mode(45 | GPIO_OUT);
		GPCR(45) = GPIO_bit(45);  /* tvia5202 HW reset */

		/* mfill -b 0x48000010 -l 0x04 -p 0x7ff47FF1 -4 */

		/* clock for Tvia5202 */
		/* MDREFR=0x000DC018; */
		/* mfill -b 0x48000004 -l 0x04 -p 0x000DC018 -4 */
	
	}

	/* setup sleep mode values */
	PWER  = 0x00000002;
	PFER  = 0x00000000;
	PRER  = 0x00000002;
	PGSR0 = 0x00008000;
	PGSR1 = 0x003F0202;
	PGSR2 = 0x0001C000;
	PCFR |= PCFR_OPDE;
}

MACHINE_START(BTWEB, "Bticino BtWeb, PXA version")
	MAINTAINER("Alessandro Rubini")
	BOOT_MEM(0xa0000000, 0x40000000, io_p2v(0x40000000))
	BOOT_PARAMS(0xa0000100)
	FIXUP(fixup_btweb)
	MAPIO(btweb_map_io)
	INITIRQ(btweb_init_irq)
MACHINE_END

/* This old thing is to be compliant with an older redboot */
MACHINE_START(BTWEB_O, "Bticino BtWeb, PXA version")
	MAINTAINER("Alessandro Rubini")
	BOOT_MEM(0xa0000000, 0x40000000, io_p2v(0x40000000))
	BOOT_PARAMS(0xa0000100)
	FIXUP(fixup_btweb)
	MAPIO(btweb_map_io)
	INITIRQ(btweb_init_irq)
MACHINE_END
