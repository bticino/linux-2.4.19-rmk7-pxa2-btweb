/*
 *  linux/arch/arm/mach-pxa/innokom.c
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


static void __init innokom_init_irq(void)
{
	pxa_init_irq();
}


void sw_update_handler( int irq, void* dev_id,struct pt_regs* regs)
{
}


void reset_handler( int irq, void* dev_id,struct pt_regs* regs)
{
}


static int __init innokom_init(void)
{
	int sw_irq = GPIO_2_80_TO_IRQ(11);	/* software update button */
	int reset_irq = GPIO_2_80_TO_IRQ(3);	/* reset button           */

	set_GPIO_IRQ_edge(11,GPIO_FALLING_EDGE);
	if (request_irq(sw_irq,sw_update_handler,SA_INTERRUPT,"software update button",NULL))
		printk(KERN_INFO "innokom: can't get assigned irq %i\n",sw_irq);

	set_GPIO_IRQ_edge(3,GPIO_FALLING_EDGE);
	if (request_irq(reset_irq,reset_handler,SA_INTERRUPT,"reset button",NULL))
		printk(KERN_INFO "innokom: can't get assigned irq %i\n",reset_irq);

	return 0;
}


__initcall(innokom_init);


static void __init
fixup_innokom(struct machine_desc *desc, struct param_struct *params,
		char **cmdline, struct meminfo *mi)
{
	/* we probably want to get this information from the bootloader later */
	SET_BANK (0, 0xa0000000, 64*1024*1024); 
	mi->nr_banks      = 1;
}


/* memory mapping */
static struct map_desc innokom_io_desc[] __initdata = {
/*  virtual           physical          length            domain     r  w  c  b                      */
  { INNOKOM_ETH_BASE, INNOKOM_ETH_PHYS, INNOKOM_ETH_SIZE, DOMAIN_IO, 0, 1, 0, 0 }, /* ETH SMSC 91111 */
  LAST_DESC
};

static void __init innokom_map_io(void)
{
	pxa_map_io();
	iotable_init(innokom_io_desc);

	/* Enable the BTUART */
	CKEN |= CKEN7_BTUART;
	set_GPIO_mode(GPIO42_BTRXD_MD);
	set_GPIO_mode(GPIO43_BTTXD_MD);
	set_GPIO_mode(GPIO44_BTCTS_MD);
	set_GPIO_mode(GPIO45_BTRTS_MD);

	set_GPIO_mode(GPIO33_nCS_5_MD);	/* SMSC network chip */

	/* setup sleep mode values */
	PWER  = 0x00000002;
	PFER  = 0x00000000;
	PRER  = 0x00000002;
	PGSR0 = 0x00008000;
	PGSR1 = 0x003F0202;
	PGSR2 = 0x0001C000;
	PCFR |= PCFR_OPDE;
}

MACHINE_START(INNOKOM, "Auerswald Innokom")
	MAINTAINER("Robert Schwebel, Pengutronix")
	BOOT_MEM(0xa0000000, 0x40000000, io_p2v(0x40000000))
	BOOT_PARAMS(0xa0000100)
	FIXUP(fixup_innokom)
	MAPIO(innokom_map_io)
	INITIRQ(innokom_init_irq)
MACHINE_END
