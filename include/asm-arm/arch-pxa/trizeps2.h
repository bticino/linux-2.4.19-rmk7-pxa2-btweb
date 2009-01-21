/*
 *  linux/include/asm-arm/arch-pxa/trizeps2.h
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Copyright (c) 2002 Luc De Cock, Teradyne DS, Ltd.
 *
 * 2002-10-10: Initial code started from idp.h
 */


/*
 * Note: this file must be safe to include in assembly files
 */

/* comment out following if you have a board  with 32MB RAM */
//#define PXA_TRIZEPS2_64MB	1
#undef PXA_TRIZEPS2_64MB

#define TRIZEPS2_FLASH_PHYS		(PXA_CS0_PHYS)
#define TRIZEPS2_ALT_FLASH_PHYS		(PXA_CS1_PHYS)
#define TRIZEPS2_MEDIAQ_PHYS		(PXA_CS3_PHYS)
#define TRIZEPS2_IDE_PHYS		(PXA_CS5_PHYS + 0x03000000)
#define TRIZEPS2_ETH_PHYS		(0x0C800000)
#define TRIZEPS2_COREVOLT_PHYS		(PXA_CS5_PHYS + 0x03800000)
#define TRIZEPS2_BCR_PHYS		(0x0E000000)
#define TRIZEPS2_CPLD_PHYS		(0x0C000000)

/*
 * virtual memory map
 */

#define TRIZEPS2_IDE_BASE		(0xf0000000)
#define TRIZEPS2_IDE_SIZE		(1*1024*1024)

#define TRIZEPS2_ETH_BASE		(0xf1000000)
#define TRIZEPS2_ETH_SIZE		(1*1024*1024)
#define ETH_BASE		TRIZEPS2_ETH_BASE //smc9194 driver compatibility issue

#define TRIZEPS2_COREVOLT_BASE	(TRIZEPS2_ETH_BASE + TRIZEPS2_ETH_SIZE)
#define TRIZEPS2_COREVOLT_SIZE	(1*1024*1024)

#define TRIZEPS2_BCR_BASE	(0xf0000000)
#define TRIZEPS2_BCR_SIZE	(1*1024*1024)

#define BCR_P2V(x)		((x) - TRIZEPS2_BCR_PHYS + TRIZEPS2_BCR_BASE)
#define BCR_V2P(x)		((x) - TRIZEPS2_BCR_BASE + TRIZEPS2_BCR_PHYS)

#ifndef __ASSEMBLY__
#  define __BCR_REG(x)		(*((volatile unsigned short *)BCR_P2V(x)))
#else
#  define __BCR_REG(x)		BCR_P2V(x)
#endif

/* board level registers  */
#define TRIZEPS2_CPLD_BASE	(0xf0100000)
#define CPLD_P2V(x)             ((x) - TRIZEPS2_CPLD_PHYS + TRIZEPS2_CPLD_BASE)
#define CPLD_V2P(x)             ((x) - TRIZEPS2_CPLD_BASE + TRIZEPS2_CPLD_PHYS)

#ifndef __ASSEMBLY__
#  define __CPLD_REG(x)         (*((volatile unsigned short *)CPLD_P2V(x)))
#else
#  define __CPLD_REG(x)         CPLD_P2V(x)
#endif

#define _TRIZEPS2_PCCARD_STATUS	(0x0c000000)
#define TRIZEPS2_PCCARD_STATUS         __CPLD_REG(_TRIZEPS2_PCCARD_STATUS)

/*
 * CS memory timing via Static Memory Control Register (MSC0-2)
 */

#define MSC_CS(cs,val) ((val)<<((cs&1)<<4))

#define MSC_RBUFF_SHIFT		15 
#define MSC_RBUFF(x)		((x)<<MSC_RBUFF_SHIFT)
#define MSC_RBUFF_SLOW		MSC_RBUFF(0)
#define MSC_RBUFF_FAST		MSC_RBUFF(1)

#define MSC_RRR_SHIFT		12
#define MSC_RRR(x)		((x)<<MSC_RRR_SHIFT)

#define MSC_RDN_SHIFT		8
#define MSC_RDN(x)		((x)<<MSC_RDN_SHIFT)

#define MSC_RDF_SHIFT		4
#define MSC_RDF(x)		((x)<<MSC_RDF_SHIFT)

#define MSC_RBW_SHIFT		3
#define MSC_RBW(x)		((x)<<MSC_RBW_SHIFT)
#define MSC_RBW_16		MSC_RBW(1)
#define MSC_RBW_32		MSC_RBW(0)

#define MSC_RT_SHIFT		0
#define MSC_RT(x)		((x)<<MSC_RT_SHIFT)


/*
 * Bit masks for various registers
 */
// TRIZEPS2_BCR_PCCARD_PWR
#define PCC_3V		(1 << 0)
#define PCC_5V		(1 << 1)
#define PCC_EN1		(1 << 2)
#define PCC_EN0		(1 << 3)

// TRIZEPS2_BCR_PCCARD_EN
#define PCC_RESET	(1 << 6)
#define PCC_ENABLE	(1 << 0)

// TRIZEPS2_BSR_PCCARDx_STATUS
#define _PCC_WRPROT	(1 << 7) // 7-4 read as low true
#define _PCC_RESET	(1 << 6)
#define _PCC_IRQ	(1 << 5)
#define _PCC_INPACK	(1 << 4)
#define PCC_BVD1	(1 << 0)
#define PCC_BVD2	(1 << 1)
#define PCC_VS1		(1 << 2)
#define PCC_VS2		(1 << 3)

// TRIZEPS2_BCR_CONTROL bits
#define BCR_LCD_ON	(1 << 4)
#define BCR_LCD_OFF	(0)
#define BCR_LCD_MASK	(1 << 4)
#define BCR_PCMCIA_RESET (1 << 7)
#define BCR_PCMCIA_NORMAL (0)

#define PCC_DETECT	(GPLR(24) & GPIO_bit(24))
#define PCC_READY	(GPLR(1) & GPIO_bit(1))

// Board Control Register
#define _TRIZEPS2_BCR_CONTROL	(TRIZEPS2_BCR_PHYS)
#define TRIZEPS2_BCR_CONTROL	__BCR_REG(_TRIZEPS2_BCR_CONTROL)

// Board TTL-IO register
#define TRIZEPS2_TTLIO_PHYS	(0x0d800000)
#define TRIZEPS2_TTLIO_BASE	(0xf2000000)
// various ioctl cmds
#define TTLIO_RESET		0
#define TTLIO_GET		1
#define TTLIO_SET		2
#define TTLIO_UNSET		3

/*
 * Macros for LCD Driver
 */

#ifdef CONFIG_FB_PXA

#define FB_BACKLIGHT_ON()
#define FB_BACKLIGHT_OFF()

#define FB_PWR_ON()
#define FB_PWR_OFF()

#define FB_VLCD_ON()		WRITE_TRIZEPS2_BCR(BCR_LCD_ON,BCR_LCD_MASK);
#define FB_VLCD_OFF()		WRITE_TRIZEPS2_BCR(BCR_LCD_OFF,BCR_LCD_MASK);

#endif

/* A listing of interrupts used by external hardware devices */

#define GPIO_TOUCH_PANEL_IRQ		2
#define TOUCH_PANEL_IRQ			IRQ_GPIO(GPIO_TOUCH_PANEL_IRQ)
#define GPIO_ETHERNET_IRQ		19
#define ETHERNET_IRQ			IRQ_GPIO(GPIO_ETHERNET_IRQ)
#define GPIO_TTLIO_IRQ			23
#define TTLIO_IRQ			IRQ_GPIO(GPIO_TTLIO_IRQ)

#define TOUCH_PANEL_IRQ_EDGE		GPIO_FALLING_EDGE
#define IDE_IRQ_EDGE			GPIO_RISING_EDGE
#define ETHERNET_IRQ_EDGE		GPIO_RISING_EDGE

#define PCMCIA_S_CD_VALID		IRQ_GPIO(24)
#define PCMCIA_S_CD_VALID_EDGE		GPIO_BOTH_EDGES

#define PCMCIA_S_RDYINT			IRQ_GPIO(1)
#define PCMCIA_S_RDYINT_EDGE		GPIO_FALLING_EDGE

/*
 * macros for MTD driver
 */

#define FLASH_WRITE_PROTECT_DISABLE()	// ((TRIZEPS2_CPLD_FLASH_WE) &= ~(0x1))
#define FLASH_WRITE_PROTECT_ENABLE()	// ((TRIZEPS2_CPLD_FLASH_WE) |= (0x1))

/* shadow registers for write only registers */
#ifndef __ASSEMBLY__
extern unsigned short trizeps2_bcr_shadow;
#endif

/* 
 * macros to write to write only register
 *
 * none of these macros are protected from 
 * multiple drivers using them in interrupt context.
 */

#define WRITE_TRIZEPS2_BCR(value, mask) \
{\
	trizeps2_bcr_shadow = ((value & mask) | (trizeps2_bcr_shadow & ~mask));\
	TRIZEPS2_BCR_CONTROL = trizeps2_bcr_shadow;\
}

