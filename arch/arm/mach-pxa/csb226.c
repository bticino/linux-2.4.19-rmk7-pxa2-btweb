/*
 *  linux/arch/arm/mach-pxa/csb226.c
 *
 * (c) 2003 Robert Schwebel <r.schwebel@pengutronix.de>, Pengutronix
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
#include <asm/arch/irqs.h>
#include <asm/hardware/sa1111.h>

#include "generic.h"

static unsigned long csb226_irq_en_mask;

static void csb226_mask_and_ack_irq(unsigned int irq)
{
	int csb226_irq = (irq - CSB226_IRQ(0));
	csb226_irq_en_mask &= ~(1 << csb226_irq);
	CSB226_IRQ_MASK_EN &= ~(1 << csb226_irq);
	CSB226_IRQ_SET_CLR &= ~(1 << csb226_irq);
}

static void csb226_mask_irq(unsigned int irq)
{
	int csb226_irq = (irq - CSB226_IRQ(0));
	csb226_irq_en_mask &= ~(1 << csb226_irq);
	CSB226_IRQ_MASK_EN &= ~(1 << csb226_irq);
}

static void csb226_unmask_irq(unsigned int irq)
{
	int csb226_irq = (irq - CSB226_IRQ(0));
	csb226_irq_en_mask |= (1 << csb226_irq);
	CSB226_IRQ_MASK_EN |= (1 << csb226_irq);
}

void csb226_irq_demux(int irq, void *dev_id, struct pt_regs *regs)
{
	unsigned long irq_status;
	int i;

	while ((irq_status = CSB226_IRQ_SET_CLR & csb226_irq_en_mask)) {
		for (i = 0; i < 6; i++) {
			if(irq_status & (1<<i)) 
				do_IRQ(CSB226_IRQ(i), regs);
		}
	}
}

/* FIXME: this should not be necessary on csb226 */
static struct irqaction csb226_irq = {
	name:		"CSB226 FPGA",
	handler:	csb226_irq_demux,
	flags:		SA_INTERRUPT
};

static void __init csb226_init_irq(void)
{
	int irq;
	
	pxa_init_irq();

	/* setup extra csb226 irqs */
/* RS: ???
	for(irq = CSB226_IRQ(0); irq <= CSB226_IRQ(5); irq++)
	{
		irq_desc[irq].valid	= 1;
		irq_desc[irq].probe_ok	= 1;
		irq_desc[irq].mask_ack	= csb226_mask_and_ack_irq;
		irq_desc[irq].mask	= csb226_mask_irq;
		irq_desc[irq].unmask	= csb226_unmask_irq;
	}

	set_GPIO_IRQ_edge(GPIO_CSB226_IRQ, GPIO_FALLING_EDGE);
	setup_arm_irq(IRQ_GPIO_CSB226_IRQ, &csb226_irq);
*/
}

/* FIXME: not necessary on CSB226? */
static int __init csb226_init(void)
{
	int ret;

	return 0;
}

__initcall(csb226_init);

static void __init
fixup_csb226(struct machine_desc *desc, struct param_struct *params,
		char **cmdline, struct meminfo *mi)
{
	SET_BANK (0, 0xa0000000, 64*1024*1024);
	mi->nr_banks      = 1;
#if 0
	setup_ramdisk (1, 0, 0, 8192);
	setup_initrd (__phys_to_virt(0xa1000000), 4*1024*1024);
	ROOT_DEV = MKDEV(RAMDISK_MAJOR,0);
#endif
}

/* FIXME: shouldn't this be moved to arch/arm/mach-pxa/mm.c? [RS] */
static struct map_desc csb226_io_desc[] __initdata = {
	/* virtual    physical    length      domain     r  w  c  b */
//	{ 0xf4000000, 0x04000000, 0x00ffffff, DOMAIN_IO, 1, 1, 0, 0 }, /* HT4562B PS/2 controller */
	{ 0xf8000000, 0x08000000, 1024*1024,  DOMAIN_IO, 0, 1, 0, 0 }, /* CS8900 LAN controller   */
//	{ 0xe0000000, 0x20000000, 0x0fffffff, DOMAIN_IO, 1, 1, 0, 0 }, /* CompactFlash            */
#if 0
	{ 0xf0000000, 0x08000000, 0x00100000, DOMAIN_IO, 0, 1, 0, 0 }, /* CPLD                    */
	{ 0xf1000000, 0x0c000000, 0x00100000, DOMAIN_IO, 0, 1, 0, 0 }, /* LAN91C96 IO             */
	{ 0xf1100000, 0x0e000000, 0x00100000, DOMAIN_IO, 0, 1, 0, 0 }, /* LAN91C96 Attr           */
	{ 0xf4000000, 0x10000000, 0x00400000, DOMAIN_IO, 0, 1, 0, 0 }, /* SA1111                  */
#endif
  LAST_DESC
};

static void __init csb226_map_io(void)
{
	pxa_map_io();
	iotable_init(csb226_io_desc);

	/* This enables the BTUART */
	CKEN |= CKEN7_BTUART;
	set_GPIO_mode(GPIO42_BTRXD_MD);
	set_GPIO_mode(GPIO43_BTTXD_MD);
	set_GPIO_mode(GPIO44_BTCTS_MD);
	set_GPIO_mode(GPIO45_BTRTS_MD);

	/* This is for the CS8900 chip select */
	set_GPIO_mode(GPIO78_nCS_2_MD);

	/* setup sleep mode values */
	PWER  = 0x00000002;
	PFER  = 0x00000000;
	PRER  = 0x00000002;
	PGSR0 = 0x00008000;
	PGSR1 = 0x003F0202;
	PGSR2 = 0x0001C000;
	PCFR |= PCFR_OPDE;
}

MACHINE_START(CSB226, "Cogent CSB226 Development Platform")
	MAINTAINER("Robert Schwebel, Pengutronix")
	BOOT_MEM(0xa0000000, 0x40000000, io_p2v(0x40000000))
	BOOT_PARAMS(0xa0000100)
	FIXUP(fixup_csb226)
	MAPIO(csb226_map_io)
	INITIRQ(csb226_init_irq)
MACHINE_END
