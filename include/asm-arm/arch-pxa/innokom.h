/*
 * linux/include/asm-arm/arch-pxa/innokom.h
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

/* 
 * GPIOs 
 */
#define GPIO_INNOKOM_RESET	3
#define GPIO_INNOKOM_SW_UPDATE	11
#define GPIO_INNOKOM_ETH	59

/* 
 * ethernet chip (SMSC91C111) 
 */
#define INNOKOM_ETH_PHYS	PXA_CS5_PHYS
#define INNOKOM_ETH_BASE	(0xf0000000)	/* phys 0x14000000 */
#define INNOKOM_ETH_SIZE	(1*1024*1024)
#define INNOKOM_ETH_IRQ		IRQ_GPIO(GPIO_INNOKOM_ETH)
#define INNOKOM_ETH_IRQ_EDGE	GPIO_RISING_EDGE

/*
 * virtual to physical conversion macros
 */
#define INNOKOM_P2V(x)		((x) - INNOKOM_FPGA_PHYS + INNOKOM_FPGA_VIRT)
#define INNOKOM_V2P(x)		((x) - INNOKOM_FPGA_VIRT + INNOKOM_FPGA_PHYS)

#ifndef __ASSEMBLY__
#  define __INNOKOM_REG(x)	(*((volatile unsigned long *)INNOKOM_P2V(x)))
#else
#  define __INNOKOM_REG(x)	INNOKOM_P2V(x)
#endif
