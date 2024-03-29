/*  da drivers/pcmcia/bt_cerf.h
 * preso da drivers/pcmcia/cerf.h
 *
 * PCMCIA implementation routines for CerfBoard
 * Based off the Assabet.
 *
 */
#ifndef _LINUX_PCMCIA_CERF_H
#define _LINUX_PCMCIA_CERF_H

#include <linux/config.h>
#include <asm/hardware.h>
#include <asm/irq.h>
#include <asm/arch/pcmcia.h>

#ifdef CONFIG_MACH_BTWEB 

#define PCMCIA_GPCR GPCR0
#define PCMCIA_GPSR GPSR0

/*  originale
#define PCMCIA_GPIO_CF_CD    14 
#define CONFIG_PXA_CERF_PDA
#define PCMCIA_GPIO_CF_IRQ   13
#define PCMCIA_GPIO_CF_RESET 12
*/
#define PCMCIA_GPIO_CF_CD    4		/* to be used for hotplug */
#define PCMCIA_GPIO_CF_IRQ   11         /* FIXED but dependent on HW GPIO used */
#define PCMCIA_GPIO_CF_RESET 3 		/* no BVD */

/*
#ifdef CONFIG_PXA_CERF_PDA
# define PCMCIA_GPIO_CF_BVD1  11
# define PCMCIA_GPIO_CF_BVD2  10
#elif defined( CONFIG_PXA_CERF_BOARD)
# define PCMCIA_GPIO_CF_BVD1  32
# define PCMCIA_GPIO_CF_BVD2  10
#endif
*/

#define PCMCIA_GPIO_CF_CD_MASK    (GPIO_bit(PCMCIA_GPIO_CF_CD))
#define PCMCIA_GPIO_CF_IRQ_MASK   (GPIO_bit(PCMCIA_GPIO_CF_IRQ))
#define PCMCIA_GPIO_CF_RESET_MASK (GPIO_bit(PCMCIA_GPIO_CF_RESET))
#define PCMCIA_GPIO_CF_BVD1_MASK  (GPIO_bit(PCMCIA_GPIO_CF_BVD1))
#define PCMCIA_GPIO_CF_BVD2_MASK  (GPIO_bit(PCMCIA_GPIO_CF_BVD2))

#define PCMCIA_GPIO_CF_CD_EDGE    PCMCIA_GPIO_CF_CD
#define PCMCIA_GPIO_CF_IRQ_EDGE   PCMCIA_GPIO_CF_IRQ
#define PCMCIA_GPIO_CF_RESET_EDGE PCMCIA_GPIO_CF_RESET
#define PCMCIA_GPIO_CF_BVD1_EDGE  PCMCIA_GPIO_CF_BVD1
#define PCMCIA_GPIO_CF_BVD2_EDGE  PCMCIA_GPIO_CF_BVD2

#define PCMCIA_IRQ_CF_CD   IRQ_GPIO(PCMCIA_GPIO_CF_CD)
#define PCMCIA_IRQ_CF_IRQ  IRQ_GPIO(PCMCIA_GPIO_CF_IRQ)
#define PCMCIA_IRQ_CF_BVD1 IRQ_GPIO(PCMCIA_GPIO_CF_BVD1)
#define PCMCIA_IRQ_CF_BVD2 IRQ_GPIO(PCMCIA_GPIO_CF_BVD2)

#define PCMCIA_PWR_SHUTDOWN 0 /* not needed */
#define CERF_SOCKET 0

inline void cerf_pcmcia_set_gpio_direction(void)
{
	GPDR(PCMCIA_GPIO_CF_CD)   &= ~(PCMCIA_GPIO_CF_CD_MASK);
/*	GPDR(PCMCIA_GPIO_CF_BVD1) &= ~(PCMCIA_GPIO_CF_BVD1_MASK);
	GPDR(PCMCIA_GPIO_CF_BVD2) &= ~(PCMCIA_GPIO_CF_BVD2_MASK);
*/	GPDR(PCMCIA_GPIO_CF_IRQ)  &= ~(PCMCIA_GPIO_CF_IRQ_MASK);

	GPDR(PCMCIA_GPIO_CF_RESET)|=  (PCMCIA_GPIO_CF_RESET_MASK);
}


inline int cerf_pcmcia_level_detect( void)
{
	return ((GPLR(PCMCIA_GPIO_CF_CD)&PCMCIA_GPIO_CF_CD_MASK)==0)?1:0;
}
inline int cerf_pcmcia_level_ready( void)
{
//	return 0;
	return (GPLR(PCMCIA_GPIO_CF_IRQ)&PCMCIA_GPIO_CF_IRQ_MASK)?1:0;
}
inline int cerf_pcmcia_level_bvd1( void)
{
	return 0;
//	return (GPLR(PCMCIA_GPIO_CF_BVD1)&PCMCIA_GPIO_CF_BVD1_MASK)?1:0;
}
inline int cerf_pcmcia_level_bvd2( void)
{
	return 0;
//	return (GPLR(PCMCIA_GPIO_CF_BVD2)&PCMCIA_GPIO_CF_BVD2_MASK)?1:0;
}

/*
#elif defined(CONFIG_SA1100_CERF) // SA1100 

#define PCMCIA_GPDR GPDR
#define PCMCIA_GPCR GPCR
#define PCMCIA_GPSR GPSR
#define PCMCIA_GPLR GPLR

#define PCMCIA_GPIO_CF_CD_MASK    GPIO_CF_CD
#define PCMCIA_GPIO_CF_IRQ_MASK   GPIO_CF_IRQ
#define PCMCIA_GPIO_CF_RESET_MASK GPIO_CF_RESET
#define PCMCIA_GPIO_CF_BVD1_MASK  GPIO_CF_BVD1
#define PCMCIA_GPIO_CF_BVD2_MASK  GPIO_CF_BVD2

#define PCMCIA_GPIO_CF_CD_EDGE    PCMCIA_GPIO_CF_CD_MASK
#define PCMCIA_GPIO_CF_IRQ_EDGE   PCMCIA_GPIO_CF_IRQ_MASK
#define PCMCIA_GPIO_CF_RESET_EDGE PCMCIA_GPIO_CF_RESET_MASK
#define PCMCIA_GPIO_CF_BVD1_EDGE  PCMCIA_GPIO_CF_BVD1_MASK
#define PCMCIA_GPIO_CF_BVD2_EDGE  PCMCIA_GPIO_CF_BVD2_MASK

#define PCMCIA_IRQ_CF_CD   IRQ_GPIO_CF_CD
#define PCMCIA_IRQ_CF_IRQ  IRQ_GPIO_CF_IRQ
#define PCMCIA_IRQ_CF_BVD1 IRQ_GPIO_CF_BVD1
#define PCMCIA_IRQ_CF_BVD2 IRQ_GPIO_CF_BVD2

#define PCMCIA_PWR_SHUTDOWN GPIO_PWR_SHUTDOWN

#ifdef CONFIG_SA1100_CERF_CPLD
#define CERF_SOCKET 0
#else
#define CERF_SOCKET 1
#endif

inline void cerf_pcmcia_set_gpio_direction(void)
{
	PCMCIA_GPDR &= ~(PCMCIA_GPIO_CF_CD_MASK |
			 PCMCIA_GPIO_CF_BVD1_MASK |
			 PCMCIA_GPIO_CF_BVD2_MASK |
			 PCMCIA_GPIO_CF_IRQ_MASK);
	PCMCIA_GPDR |=   PCMCIA_GPIO_CF_RESET_MASK;
}

inline int cerf_pcmcia_level_detect( void)
{
	return ((PCMCIA_GPLR & PCMCIA_GPIO_CF_CD_MASK)==0)?1:0;
}
inline int cerf_pcmcia_level_ready( void)
{
	return (PCMCIA_GPLR & PCMCIA_GPIO_CF_IRQ_MASK)?1:0;
}
inline int cerf_pcmcia_level_bvd1( void)
{
	return (PCMCIA_GPLR & PCMCIA_GPIO_CF_BVD1_MASK)?1:0;
}
inline int cerf_pcmcia_level_bvd2( void)
{
	return (PCMCIA_GPLR & PCMCIA_GPIO_CF_BVD2_MASK)?1:0;
}
*/
#endif


#endif
