/*
 *  linux/arch/arm/mach-pxa/trizeps2.c
 *
 *  Support for the Keith&Koep MT6N Development Platform.
 *  
 *  Author:	Luc De Cock
 *  Created:	Jan 13, 2003
 *  Copyright:	Teradyne DS, Ltd.
 *  
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */
#include <linux/init.h>
#include <linux/major.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/sched.h>

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

static unsigned long trizeps2_irq_en_mask;
unsigned short trizeps2_bcr_shadow = 0x50; // 0x70


static void __init trizeps2_init_irq(void)
{
	int irq;
	
	pxa_init_irq();

	set_GPIO_IRQ_edge(GPIO_ETHERNET_IRQ, GPIO_RISING_EDGE);
}

static int __init trizeps2_init(void)
{
	/* Configure the BCR register */
	unsigned short *bcr = (unsigned short *) TRIZEPS2_BCR_BASE;

	*bcr = trizeps2_bcr_shadow;
	return 0;
}

__initcall(trizeps2_init);

static void __init
fixup_trizeps2(struct machine_desc *desc, struct param_struct *params,
		char **cmdline, struct meminfo *mi)
{
#ifdef TRIZEPS2_MEM_64MB
	SET_BANK (0, 0xa0000000, 64*1024*1024);
#else
	SET_BANK (0, 0xa0000000, 32*1024*1024);
#endif
	mi->nr_banks      = 1;
}

static struct map_desc trizeps2_io_desc[] __initdata = {
 /* virtual     physical    length      domain     r  w  c  b */
  { 0xf0000000, 0x0e000000, 0x00100000, DOMAIN_IO, 0, 1, 0, 0 }, /* BCR */
  { 0xf0100000, 0x0c000000, 0x00100000, DOMAIN_IO, 0, 1, 0, 0 }, /* PCMCIA STATUS */
  { 0xf1000000, 0x0c800000, 0x00100000, DOMAIN_IO, 0, 1, 0, 0 }, /* LAN91C96 IO */
  { 0xf1100000, 0x0e000000, 0x00100000, DOMAIN_IO, 0, 1, 0, 0 }, /* LAN91C96 Attr */
  { 0xf2000000, 0x0d800000, 0x00100000, DOMAIN_IO, 0, 1, 0, 0 }, /* TTL-IO */
  LAST_DESC
};

static void __init trizeps2_map_io(void)
{
	pxa_map_io();
	iotable_init(trizeps2_io_desc);

	/* This is for the SMC chip select */
	set_GPIO_mode(GPIO79_nCS_3_MD);

	/* setup sleep mode values */
	PWER  = 0x00000002;
	PFER  = 0x00000000;
	PRER  = 0x00000002;
	PGSR0 = 0x00008000;
	PGSR1 = 0x003F0202;
	PGSR2 = 0x0001C000;
	PCFR |= PCFR_OPDE;
}

MACHINE_START(TRIZEPS2, "Keith-n-Koep MT6N Development Platform")
	MAINTAINER("Luc De Cock")
	BOOT_MEM(0xa0000000, 0x40000000, io_p2v(0x40000000))
	FIXUP(fixup_trizeps2)
	MAPIO(trizeps2_map_io)
	INITIRQ(trizeps2_init_irq)
MACHINE_END
