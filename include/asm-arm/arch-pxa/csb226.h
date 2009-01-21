/*
 *  linux/include/asm-arm/arch-pxa/csb226.h
 *
 *  Author:	Robert Schwebel (stolen from lubbock.h)
 *  Created:	Oct 30, 2002
 *  Copyright:	Pengutronix
 *  
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define CSB226_FPGA_PHYS	PXA_CS2_PHYS    

#define CSB226_FPGA_VIRT	(0xf0000000)	/* phys 0x08000000 */
#define CSB226_ETH_BASE		(0xf1000000)	/* phys 0x0c000000 */

#define CSB226_P2V(x)		((x) - CSB226_FPGA_PHYS + CSB226_FPGA_VIRT)
#define CSB226_V2P(x)		((x) - CSB226_FPGA_VIRT + CSB226_FPGA_PHYS)

#ifndef __ASSEMBLY__
#  define __CSB226_REG(x)	(*((volatile unsigned long *)CSB226_P2V(x)))
#else
#  define __CSB226_REG(x)	CSB226_P2V(x)
#endif


/* register physical addresses */
#define _CSB226_MISC_WR		(CSB226_FPGA_PHYS + 0x080)
#define _CSB226_MISC_RD		(CSB226_FPGA_PHYS + 0x090)
#define _CSB226_IRQ_MASK_EN	(CSB226_FPGA_PHYS + 0x0C0)
#define _CSB226_IRQ_SET_CLR	(CSB226_FPGA_PHYS + 0x0D0)
#define _CSB226_GP		(CSB226_FPGA_PHYS + 0x100)



/* register virtual addresses */

#define CSB226_MISC_WR             __CSB226_REG(_CSB226_MISC_WR) 
#define CSB226_MISC_RD             __CSB226_REG(_CSB226_MISC_RD)         
#define CSB226_IRQ_MASK_EN         __CSB226_REG(_CSB226_IRQ_MASK_EN)
#define CSB226_IRQ_SET_CLR         __CSB226_REG(_CSB226_IRQ_SET_CLR)             
#define CSB226_GP                  __CSB226_REG(_CSB226_GP)      


/* GPIOs */

#define GPIO_CSB226_IRQ		0
#define IRQ_GPIO_CSB226_IRQ	IRQ_GPIO0


/*
 * LED macros
 */

// #define LEDS_BASE LUB_DISC_BLNK_LED

// 8 discrete leds available for general use:

/*
#define D28	0x1
#define D27	0x2
#define D26	0x4
#define D25	0x8
#define D24	0x10
#define D23	0x20
#define D22	0x40
#define D21	0x80
*/

/* Note: bits [15-8] are used to enable/blank the 8 7 segment hex displays so
*  be sure to not monkey with them here.
*/

/*
#define HEARTBEAT_LED	D28
#define SYS_BUSY_LED    D27
#define HEXLEDS_BASE LUB_HEXLED

#define HEARTBEAT_LED_ON  (LEDS_BASE &= ~HEARTBEAT_LED)
#define HEARTBEAT_LED_OFF (LEDS_BASE |= HEARTBEAT_LED)
#define SYS_BUSY_LED_OFF  (LEDS_BASE |= SYS_BUSY_LED)
#define SYS_BUSY_LED_ON   (LEDS_BASE &= ~SYS_BUSY_LED)

// use x = D26-D21 for these, please...
#define DISCRETE_LED_ON(x) (LEDS_BASE &= ~(x))
#define DISCRETE_LED_OFF(x) (LEDS_BASE |= (x))
*/

#ifndef __ASSEMBLY__

//extern int hexled_val = 0;

#endif

/*
#define BUMP_COUNTER (HEXLEDS_BASE = hexled_val++)
#define DEC_COUNTER (HEXLEDS_BASE = hexled_val--)
*/
