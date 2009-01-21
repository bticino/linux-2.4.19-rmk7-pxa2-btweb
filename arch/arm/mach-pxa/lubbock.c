/*
 *  linux/arch/arm/mach-pxa/lubbock.c
 *
 *  Support for the Intel DBPXA250 Development Platform.
 *  
 *  Author:	Nicolas Pitre
 *  Created:	Jun 15, 2001
 *  Copyright:	MontaVista Software Inc.
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
#include <linux/bitops.h>

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
#include <asm/hardware/sa1111.h>

#include "generic.h"

#ifdef CONFIG_SA1111
 #include "sa1111.h"
#endif


static unsigned long lubbock_irq_enabled;

static void lubbock_mask_irq(unsigned int irq)
{
	int lubbock_irq = (irq - LUBBOCK_IRQ(0));
	LUB_IRQ_MASK_EN = (lubbock_irq_enabled &= ~(1 << lubbock_irq));
}

static void lubbock_unmask_irq(unsigned int irq)
{
	int lubbock_irq = (irq - LUBBOCK_IRQ(0));
	/* the irq can be acknowledged only if deasserted, so it's done here */
	LUB_IRQ_SET_CLR &= ~(1 << lubbock_irq);
	LUB_IRQ_MASK_EN = (lubbock_irq_enabled |= (1 << lubbock_irq));
}

void lubbock_irq_demux(int irq, void *dev_id, struct pt_regs *regs)
{
	unsigned long pending = LUB_IRQ_SET_CLR & lubbock_irq_enabled;
	do {
		GEDR(0) = GPIO_bit(0);  /* clear useless edge notification */
		if (likely(pending))
			do_IRQ( LUBBOCK_IRQ(0) + __ffs(pending), regs );
		pending = LUB_IRQ_SET_CLR & lubbock_irq_enabled;
	} while (pending);
}

static struct irqaction lubbock_irq = {
	name:		"Lubbock FPGA",
	handler:	lubbock_irq_demux,
	flags:		SA_INTERRUPT
};

static void __init lubbock_init_irq(void)
{
	int irq;
	
	pxa_init_irq();

	/* setup extra lubbock irqs */
	for(irq = LUBBOCK_IRQ(0); irq <= LUBBOCK_IRQ(5); irq++) {
		irq_desc[irq].valid	= 1;
		irq_desc[irq].probe_ok	= 1;
		irq_desc[irq].mask_ack	= lubbock_mask_irq;
		irq_desc[irq].mask	= lubbock_mask_irq;
		irq_desc[irq].unmask	= lubbock_unmask_irq;
	}

	set_GPIO_IRQ_edge(GPIO_LUBBOCK_IRQ, GPIO_FALLING_EDGE);
	setup_arm_irq(IRQ_GPIO_LUBBOCK_IRQ, &lubbock_irq);
}

static int __init lubbock_init(void)
{
	int ret;

	ret = sa1111_probe(LUBBOCK_SA1111_BASE);
	if (ret)
		return ret;
	sa1111_wake();
	sa1111_init_irq(LUBBOCK_SA1111_IRQ);
	return 0;
}

__initcall(lubbock_init);

static void __init
fixup_lubbock(struct machine_desc *desc, struct param_struct *params,
		char **cmdline, struct meminfo *mi)
{
	/* Some boards have 32MB some 64MB.  Let's use a safe default */
	SET_BANK (0, 0xa0000000, 32*1024*1024);
	mi->nr_banks      = 1;
}

static struct map_desc lubbock_io_desc[] __initdata = {
 /* virtual     physical    length      domain     r  w  c  b */
  { 0xf0000000, 0x08000000, 0x00100000, DOMAIN_IO, 0, 1, 0, 0 }, /* CPLD */
  { 0xf1000000, 0x0c000000, 0x00100000, DOMAIN_IO, 0, 1, 0, 0 }, /* LAN91C96 IO */
  { 0xf1100000, 0x0e000000, 0x00100000, DOMAIN_IO, 0, 1, 0, 0 }, /* LAN91C96 Attr */
  { 0xf4000000, 0x10000000, 0x00400000, DOMAIN_IO, 0, 1, 0, 0 }, /* SA1111 */
  LAST_DESC
};

static void __init lubbock_map_io(void)
{
	pxa_map_io();
	iotable_init(lubbock_io_desc);

	/* This enables the BTUART */
	CKEN |= CKEN7_BTUART;
	set_GPIO_mode(GPIO42_BTRXD_MD);
	set_GPIO_mode(GPIO43_BTTXD_MD);
	set_GPIO_mode(GPIO44_BTCTS_MD);
	set_GPIO_mode(GPIO45_BTRTS_MD);

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

MACHINE_START(LUBBOCK, "Intel DBPXA250 Development Platform")
	MAINTAINER("MontaVista Software Inc.")
	BOOT_MEM(0xa0000000, 0x40000000, io_p2v(0x40000000))
	FIXUP(fixup_lubbock)
	MAPIO(lubbock_map_io)
	INITIRQ(lubbock_init_irq)
MACHINE_END
