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

#if defined(CONFIG_BIGBUF) || defined(CONFIG_BIGBUF_MODULE)
unsigned char __attribute__ ((aligned (4096))) btweb_bigbuf[256*1024];
EXPORT_SYMBOL(btweb_bigbuf);
#endif

#if defined(CONFIG_BIGBUF1) || defined(CONFIG_BIGBUF1_MODULE)
unsigned char __attribute__ ((aligned (4096))) btweb_bigbuf1[256*1024];
EXPORT_SYMBOL(btweb_bigbuf1);
#endif

EXPORT_SYMBOL(btweb_features);
EXPORT_SYMBOL(btweb_globals);

/* The rest of the file is the conventional part */
static void __init btweb_init_irq(void)
{
	pxa_init_irq();
	kd_mksound = btweb_kd_mksound;
}

static int __init btweb_init(void)
{
	printk("btweb_init\n");
	/* For USB. Can't do this in btweb_map_io, as it's too early */
	if (btweb_features.usb_irq >= 0)
		set_GPIO_IRQ_edge(btweb_features.usb_irq, GPIO_RISING_EDGE);
	/* Similarly, we beed I/O mapped before doing this */
	if ((btweb_globals.flavor!=BTWEB_MH500) && (MDCNFG != btweb_features.mdcnfg)) {
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
	extern int btweb_find_device(int *memsize);
	int memsize, i;

	/* If this is an old bootloader, complain */
	if (machine_is_btweb_old()) {
		printk(KERN_ERR "BTWEB Warning: using old boot loader\n");
	}

	/* Detect the flavor of btweb device, from GPIO settings */
	if ( (i = btweb_find_device(&memsize))) {
		printk("BtWeb: error %i in find_device(). Stopping\n", i);
		while (1);
	}

	/* Some boards have 32MB some 64MB.  Let's use a safe default */
	SET_BANK (0, 0xa0000000, memsize);
	mi->nr_banks      = 1;
}

static struct map_desc btweb_io_desc[] __initdata = {
 /* virtual     physical    length      domain     r  w  c  b */
  { 0xf1000000, 0x0c000000, 0x00100000, DOMAIN_IO, 0, 1, 0, 0 }, /* LAN91C96 IO */
  { 0xf1100000, 0x0e000000, 0x00100000, DOMAIN_IO, 0, 1, 0, 0 }, /* LAN91C96 Attr */
  { 0xf4000000, 0x10000000, 0x00400000, DOMAIN_IO, 0, 1, 0, 0 }, /* SA1111 */
  LAST_DESC
};

static void __init btweb_map_io(void)
{
	printk("btweb_map_io: calling pxa_map_io\n");
	pxa_map_io();
        printk("btweb_map_io: calling iotable_init\n");
	iotable_init(btweb_io_desc);

	if ((btweb_globals.flavor==BTWEB_F453AV)||(btweb_globals.flavor==BTWEB_2F)|| \
		(btweb_globals.flavor==BTWEB_INTERFMM) ){
		set_GPIO_mode(btweb_features.tvia_reset | GPIO_OUT);
		GPCR(btweb_features.tvia_reset) = GPIO_bit(btweb_features.tvia_reset); /* tvia5202 HW reset */

		printk("Setting MSC2 (%p) to 0x92347FF1\n",&MSC2);
		MSC2=0x92347FF1;    /* BUS veloce su chip select Tvia5202 */
	} else {
		if ((btweb_globals.flavor==BTWEB_PE)||(btweb_globals.flavor==BTWEB_PI)) {
	                set_GPIO_mode(btweb_features.tvia_reset | GPIO_OUT);
        		GPSR(btweb_features.tvia_reset) = GPIO_bit(btweb_features.tvia_reset); /* tvia5202 HW reset */
			printk("Setting MSC2 (%p) to 0x92347FF1\n",&MSC2);
			MSC2=0x92347FF1;    /* BUS veloce su chip select Tvia5202 */

                        set_GPIO_mode(btweb_features.eth_reset | GPIO_OUT);
                        GPSR(btweb_features.eth_reset) = GPIO_bit(btweb_features.eth_reset); /* eth soft reset */
                        printk("Not resetting eth soft reset\n");
                        printk("GPLR0=%08X\n",GPLR0);
		}
	}
	

	printk("btweb_map_io: verifying some registers\n");
        printk("MSC0=%X\n",MSC0);
        printk("MSC1=%X\n",MSC1);
        printk("MSC2=%X\n",MSC2);
        printk("MDREFR=%X\n",MDREFR);


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

