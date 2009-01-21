/*
 *  Generic I2S code for pxa250
 *
 *  Phil Blundell <pb@nexus.co.uk>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/poll.h>
#include <linux/sound.h>
#include <linux/soundcard.h>

#include <asm/semaphore.h>
#include <asm/uaccess.h>
#include <asm/hardware.h>
#include <asm/dma.h>

#include "pxa-i2s.h"

/* Some pxa registers are missing or def'd differently here, fix this - ARub */
#define GPIO28_BITCLK_OUT_I2S_MD (28 | GPIO_ALT_FN_1_OUT)
#define GPIO32_SYSCLK_I2S_MD    (32 | GPIO_ALT_FN_1_OUT)
#define PXA_SACR0  SACR0 /* we have it renamed in our pxa-regs.h */
#define PXA_SACR1  SACR1 /* we have it renamed in our pxa-regs.h */
#define PXA_SADIV  SADIV /* we have it renamed in our pxa-regs.h */
#define PXA_SADR   SADR  /* we have it renamed in our pxa-regs.h */

#define SACR0_TFTH(n)                   (((n)&0xf) << 8)
#define SACR0_RFTH(n)                   (((n)&0xf) << 12)
#define SACR0_RST                       0x8
#define SACR0_BCKD_OUTPUT               0x4
#define SACR0_ENB                       0x1

#define SADIV_BITCLK_3_072_MHZ  0x0c /* 48.000 KHz sampling rate */
#define SADIV_BITCLK_2_836_MHZ  0x0d /* 44.308 KHz sampling rate */
#define SADIV_BITCLK_1_418_MHZ  0x1a /* 22.154 KHz sampling rate */
#define SADIV_BITCLK_1_024_MHZ  0x24 /* 16.000 KHz sampling rate */
#define SADIV_BITCLK_708_92_KHZ 0x34 /* 11.077 KHz sampling rate */
#define SADIV_BITCLK_512_00_KHZ 0x48 /*  8.000 KHz sampling rate */



void
pxa_i2s_init (void)
{
	unsigned long flags;

	/* Setup the uarts */
	local_irq_save(flags);

	set_GPIO_mode(GPIO28_BITCLK_OUT_I2S_MD);
	set_GPIO_mode(GPIO29_SDATA_IN_I2S_MD);
	set_GPIO_mode(GPIO30_SDATA_OUT_I2S_MD);
	set_GPIO_mode(GPIO31_SYNC_I2S_MD);
	set_GPIO_mode(GPIO32_SYSCLK_I2S_MD);

	/* enable the clock to I2S unit */
	CKEN |= CKEN8_I2S;
	PXA_SACR0 = SACR0_RST;
	PXA_SACR0 = SACR0_BCKD_OUTPUT | SACR0_RFTH(8) | SACR0_TFTH(8);
	PXA_SACR1 = 0;
	PXA_SADIV = SADIV_BITCLK_2_836_MHZ; /* 44.1 KHz */
	PXA_SACR0 |= SACR0_ENB;

 	local_irq_restore(flags);
}

void
pxa_i2s_shutdown (void)
{
	PXA_SACR0 = SACR0_RST;
	PXA_SACR1 = 0;
}

int
pxa_i2s_set_samplerate (long val)
{
	int clk_div = 0, fs = 0;

	/* wait for any frame to complete */
	udelay(125);

	if (val >= 48000)
		val = 48000;
	else if (val >= 44100)
		val = 44100;
	else if (val >= 22050)
		val = 22050;
	else if (val >= 16000)
		val = 16000;
	else
		val = 8000;

	/* Select the clock divisor */
	switch (val) {
	case 8000:
		fs = 256;
		clk_div = SADIV_BITCLK_512_00_KHZ;
		break;
	case 16000:
		fs = 256;
		clk_div = SADIV_BITCLK_1_024_MHZ;
		break;
	case 22050:
		fs = 256;
		clk_div = SADIV_BITCLK_1_418_MHZ;
		break;
	case 44100:
		fs = 256;
		clk_div = SADIV_BITCLK_2_836_MHZ;
		break;
	case 48000:
		fs = 256;
		clk_div = SADIV_BITCLK_3_072_MHZ;
		break;
	}

	PXA_SADIV = clk_div;

	return fs;
}

audio_stream_t pxa_i2s_output_stream = {
	name:			"I2S audio out",
	dcmd:			DCMD_TXPCDR,
	drcmr:			&DRCMRTXSADR,
	dev_addr:		__PREG(PXA_SADR),
};

audio_stream_t pxa_i2s_input_stream = {
	name:			"I2S audio in",
	dcmd:			DCMD_RXPCDR,
	drcmr:			&DRCMRRXSADR,
	dev_addr:		__PREG(PXA_SADR),
};

EXPORT_SYMBOL (pxa_i2s_init);
EXPORT_SYMBOL (pxa_i2s_shutdown);
EXPORT_SYMBOL (pxa_i2s_set_samplerate);
EXPORT_SYMBOL (pxa_i2s_output_stream);
EXPORT_SYMBOL (pxa_i2s_input_stream);
