/*
 *  linux/drivers/sound/pxa-ac97.c -- AC97 interface for the Cotula chip
 *
 *  Author:	Nicolas Pitre
 *  Created:	Aug 15, 2001
 *  Copyright:	MontaVista Software Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/pci.h>
#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/poll.h>
#include <linux/sound.h>
#include <linux/soundcard.h>
#include <linux/pm.h>
#include <linux/ac97_codec.h>

#include <asm/hardware.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <asm/semaphore.h>
#include <asm/dma.h>

#include "pxa-audio.h"

#ifdef CONFIG_PM
static struct pm_dev *ac97_pm_dev=NULL;
#endif

#define CODEC_PRIMARY    1
#define CODEC_SECONDARY  1

#define ADDR_BASE_PRIM 0x40500200
#define ADDR_BASE_SEC  0x40500300

#define AC97_MODE_CTRL  0x5E
#define AC_REMAP_SLOT56 0x10

#define SNDCTL_BTGET_DELAY          _SIOWR('P',99, int)


// !!! parm --- punto di partenza quella di Advantech senza opzioni PM

DECLARE_MUTEX(CAR_mutex);
//DECLARE_MUTEX(CAR_mutex_1);
extern int time_add_start;


#ifdef CODEC_PRIMARY
static u16 pxa_ac97_read(struct ac97_codec *codec, u8 reg )
{
// !!! parm funzione sdoppiata per accedere a entrambi i codec
	u16 val = -1;
	u32 addr;
	volatile u32 *ioaddr;
	u32 addr_cod_base = ADDR_BASE_PRIM ;
	
//printk("pxa ac97 read \n");

	down(&CAR_mutex);
	if (!(CAR & CAR_CAIP)) {
 		addr = addr_cod_base + (reg << 1) ;
		ioaddr = (volatile u32 *)ioremap( addr, 4);
		CAR |= CAR_CAIP;
		val = *ioaddr;
		while(!(GSR & GSR_SDONE)) {
			udelay(1);
		}
		GSR |= GSR_SDONE;
		if(GSR & GSR_RDCS) {
			GSR |= GSR_RDCS;
		}
		val = (u16)(*ioaddr & 0x0000FFFF);
//printk (" read at add %X, val %X \n", addr, val);
		while(!(GSR & GSR_SDONE)) {
			udelay(1);
		}
		GSR |= GSR_SDONE;
		CAR &= ~CAR_CAIP;
		iounmap(ioaddr);
	} else {
		printk(KERN_CRIT __FUNCTION__": CAR_CAIP already set\n");
	}
	up(&CAR_mutex);
	//printk("%s(0x%02x) = 0x%04x\n", __FUNCTION__, reg, val);
	return val;
}


static void pxa_ac97_wait(struct ac97_codec *codec)
{
// !!! parm funzione sdoppiata per accedere a entrambi i codec

	volatile u32 *ioaddr;
	u32 addr_cod_base = ADDR_BASE_PRIM ;
	int i=1;
	u16 val=0;

//printk ("pxa_ac97_wait \n" );

	down(&CAR_mutex);
	if(!(CAR & CAR_CAIP)) {
		ioaddr = (volatile u32 *)ioremap( addr_cod_base + (0x0026 << 1), 4);
		CAR |= CAR_CAIP;
		while(i==1) {
			val = *ioaddr;
			while(!(GSR & GSR_SDONE)) {
				udelay(1);
			}
			GSR |= GSR_SDONE;
			if(GSR & GSR_RDCS) {
				GSR |= GSR_RDCS;
			}
			val = (u16)(*ioaddr & 0x0000FFFF);
			while(!(GSR & GSR_SDONE)) {
				udelay(1);
			}
			GSR |= GSR_SDONE;
			if((val & 0x000F) == 0x000F) {
				i = 0;
			}
		}
		CAR &= ~CAR_CAIP;
		iounmap(ioaddr);
	}
	else {
		printk(KERN_CRIT __FUNCTION__": CAR_CAIP already set\n");
	}
	up(&CAR_mutex);
}

static void pxa_ac97_write(struct ac97_codec *codec, u8 reg, u16 val)
{
// !!! parm funzione sdoppiata per accedere a entrambi i codec
	volatile u32 *ioaddr, addr;
	u32 addr_cod_base = ADDR_BASE_PRIM ;

	down(&CAR_mutex);
	if (!(CAR & CAR_CAIP)) {
		addr = addr_cod_base + (reg << 1 );
//printk ("pxa_ac97_write addr %X val %X \n", addr, val);
		ioaddr = (volatile u32 *)ioremap( addr , 4);
		GSR |= GSR_CDONE;
		CAR |= CAR_CAIP;
		*ioaddr = val;
		while(!(GSR & GSR_CDONE)) {
			udelay(1);
		}
		GSR |= GSR_CDONE;
		CAR &= ~CAR_CAIP;
		iounmap(ioaddr);
	} else {
		printk(KERN_CRIT __FUNCTION__": CAR_CAIP already set\n");
	}
	up(&CAR_mutex);
	//printk("%s(0x%02x, 0x%04x)\n", __FUNCTION__, reg, val);
}
#endif

#ifdef CODEC_SECONDARY
extern void pxa_ac97_wait1(struct ac97_codec *codec ); 
extern void pxa_ac97_write1(struct ac97_codec *codec, u8 reg, u16 val );
extern u16 pxa_ac97_read1(struct ac97_codec *codec, u8 reg );
/*
static void pxa_ac97_wait1(struct ac97_codec *codec )
{
// !!! parm funzione sdoppiata per accedere a entrambi i codec

	volatile u32 *ioaddr;
	int i=1;
	u16 val=0;
	u32 addr_cod_base = ADDR_BASE_SEC ;

//printk ("pxa_ac97_wait 1 \n");

	down(&CAR_mutex_1);
	if(!(CAR & CAR_CAIP)) {
		
		ioaddr = (volatile u32 *)ioremap( addr_cod_base + (0x0026 << 1), 4);
		CAR |= CAR_CAIP;
		while(i==1) {
			val = *ioaddr;
			while(!(GSR & GSR_SDONE)) {
				udelay(1);
			}
			GSR |= GSR_SDONE;
			if(GSR & GSR_RDCS) {
				GSR |= GSR_RDCS;
			}
			val = (u16)(*ioaddr & 0x0000FFFF);
			while(!(GSR & GSR_SDONE)) {
				udelay(1);
			}
			GSR |= GSR_SDONE;
			if((val & 0x000F) == 0x000F) {
				i = 0;
			}
		}
		CAR &= ~CAR_CAIP;
		iounmap(ioaddr);
	}
	else {
		printk(KERN_CRIT __FUNCTION__": CAR_CAIP already set\n");
	}
	up(&CAR_mutex_1);
}



u16 pxa_ac97_read1(struct ac97_codec *codec, u8 reg )
{
// !!! parm funzione sdoppiata per accedere a entrambi i codec
	u16 val = -1;
	u32 addr;
	volatile u32 *ioaddr;
	u32 addr_cod_base = ADDR_BASE_SEC ;

//printk("pxa ac97 read 1\n");
	
	down(&CAR_mutex_1);
	if (!(CAR & CAR_CAIP)) {
 		addr = ADDR_BASE_SEC + (reg << 1) ;
//	printk (" read 1 at add %X, val %X \n", addr, val);
		ioaddr = (volatile u32 *)ioremap( addr, 4);
		CAR |= CAR_CAIP;
		val = *ioaddr;
		while(!(GSR & GSR_SDONE)) {
			udelay(1);
		}
		GSR |= GSR_SDONE;
		if(GSR & GSR_RDCS) {
			GSR |= GSR_RDCS;
		}
		val = (u16)(*ioaddr & 0x0000FFFF);
		while(!(GSR & GSR_SDONE)) {
			udelay(1);
		}
		GSR |= GSR_SDONE;
		CAR &= ~CAR_CAIP;
		iounmap(ioaddr);
	} else {
		printk(KERN_CRIT __FUNCTION__": CAR_CAIP already set\n");
	}
	up(&CAR_mutex_1);
	//printk("%s(0x%02x) = 0x%04x\n", __FUNCTION__, reg, val);
	return val;
}



void pxa_ac97_write1(struct ac97_codec *codec, u8 reg, u16 val )
{
// !!! parm funzione sdoppiata per accedere a entrambi i codec
	volatile u32 *ioaddr, addr;
	u32 addr_cod_base = ADDR_BASE_SEC ;

	down(&CAR_mutex_1);
	if (!(CAR & CAR_CAIP)) {
		addr = addr_cod_base + (reg << 1 );
//printk ("pxa_ac97_write 1 addr %X val %X \n", addr, val);
		ioaddr = (volatile u32 *)ioremap( addr , 4);
		GSR |= GSR_CDONE;
		CAR |= CAR_CAIP;
		*ioaddr = val;
		while(!(GSR & GSR_CDONE)) {
			udelay(1);
		}
		GSR |= GSR_CDONE;
		CAR &= ~CAR_CAIP;
		iounmap(ioaddr);
	} else {
		printk(KERN_CRIT __FUNCTION__": CAR_CAIP already set\n");
	}
	up(&CAR_mutex_1);
	//printk("%s(0x%02x, 0x%04x)\n", __FUNCTION__, reg, val);
}
*/

#endif


#ifdef CODEC_PRIMARY
static struct ac97_codec pxa_ac97_codec = {
	codec_wait: pxa_ac97_wait,
	codec_read:	pxa_ac97_read,
	codec_write: pxa_ac97_write,
};

#endif

extern struct ac97_codec pxa_ac97_codec1;
/*
#ifdef CODEC_SECONDARY
struct ac97_codec pxa_ac97_codec1 = {
	codec_wait: pxa_ac97_wait1,
	codec_read:	pxa_ac97_read1,
	codec_write: pxa_ac97_write1,
};
#endif
*/

/*
#ifdef CONFIG_PM
static int ac97_pm_callback(struct pm_dev *pm_dev, pm_request_t req, void *data)
{
	switch (req) {
	case PM_SUSPEND:
		GCR = GCR_ACLINK_OFF;
		CKEN &= ~CKEN2_AC97; 
		break;
	case PM_RESUME:
		CKEN |= CKEN2_AC97; 
		GCR = 0;
		udelay(10);
		GCR = GCR_COLD_RST;
		while (!(GSR & GSR_PCR)) {
			udelay(1);
		}
		ac97_probe_codec(&pxa_ac97_codec);

		pxa_ac97_write(&pxa_ac97_codec, 0x1A, 0x0404); // ORIG last parameter 0x0505

		break;
	}
	return 0;
}
#endif
*/

static DECLARE_MUTEX(pxa_ac97_mutex);
static DECLARE_MUTEX(pxa_ac97_mutex_1);
static int pxa_ac97_refcount;

#ifdef CODEC_PRIMARY
int pxa_ac97_get(struct ac97_codec **codec)
{
	int ret;

printk ("pxa ac97 get, refcount %d \n", pxa_ac97_refcount);

	*codec = NULL;
	down(&pxa_ac97_mutex);

	if (!pxa_ac97_refcount) {
		CKEN |= CKEN2_AC97; 
		set_GPIO_mode(GPIO31_SYNC_AC97_MD);
		set_GPIO_mode(GPIO30_SDATA_OUT_AC97_MD);
		set_GPIO_mode(GPIO28_BITCLK_AC97_MD);
		set_GPIO_mode(GPIO29_SDATA_IN_AC97_MD);
//		set_GPIO_mode(GPIO32_SDATA_IN1_AC97_MD);	// toccherebbe il secondary

		GCR = 0;
		udelay(10);
		GCR = GCR_COLD_RST;
		while (!(GSR & GSR_PCR)) {
			schedule();
		}

		ret = ac97_probe_codec(&pxa_ac97_codec);
		if (ret != 1) {
			GCR = GCR_ACLINK_OFF;
			CKEN &= ~CKEN2_AC97; 
			return ret;
		}

	// need little hack for UCB1400 (should be moved elsewhere)
//		pxa_ac97_write(&pxa_ac97_codec,AC97_EXTENDED_STATUS,1);
//pxa_ac97_write(&pxa_ac97_codec, 0x6a, 0x1ff7);
//		pxa_ac97_write(&pxa_ac97_codec, 0x6a, 0x0050);
//		pxa_ac97_write(&pxa_ac97_codec, 0x6c, 0x0030);
		pxa_ac97_write(&pxa_ac97_codec, 0x1A, 0x0404 ); // CARLOS.... ORIG last parameter 0x0505

	}

	pxa_ac97_refcount++;
	up(&pxa_ac97_mutex);

	*codec = &pxa_ac97_codec;


	return 0;
}
#endif

#ifdef CODEC_SECONDARY
int pxa_ac97_get1(struct ac97_codec **codec)
{
	int ret;

printk ("pxa ac97 get1, refcount %d \n", pxa_ac97_refcount);

	*codec = NULL;
	down(&pxa_ac97_mutex_1);


//	if (!pxa_ac97_refcount) {	// lasciato nel caso ci sia solo il secondario

		CKEN |= CKEN2_AC97; 
		set_GPIO_mode(GPIO31_SYNC_AC97_MD);
		set_GPIO_mode(GPIO30_SDATA_OUT_AC97_MD);
		set_GPIO_mode(GPIO28_BITCLK_AC97_MD);
		set_GPIO_mode(GPIO32_SDATA_IN1_AC97_MD);

//		GCR = 0;
//		udelay(10);
//		GCR = GCR_COLD_RST;
//		while (!(GSR & GSR_PCR)) {
//			schedule();
//	}
		// !!! parm, commentato sopra
//		}

		pxa_ac97_codec1.id = pxa_ac97_refcount; // !!! metto l'id per riconoscerlo
		ret = ac97_probe_codec(&pxa_ac97_codec1);
		if (ret != 1) 
			return ret;
//			GCR = GCR_ACLINK_OFF;
//			CKEN &= ~CKEN2_AC97; 
		pxa_ac97_write1(&pxa_ac97_codec1,AC97_MODE_CTRL,AC_REMAP_SLOT56);

		// need little hack for UCB1400 (should be moved elsewhere)
//		pxa_ac97_write(&pxa_ac97_codec,AC97_EXTENDED_STATUS,1);
		//pxa_ac97_write(&pxa_ac97_codec, 0x6a, 0x1ff7);
//		pxa_ac97_write(&pxa_ac97_codec, 0x6a, 0x0050);
//		pxa_ac97_write(&pxa_ac97_codec, 0x6c, 0x0030);

		pxa_ac97_write1(&pxa_ac97_codec1, 0x1A, 0x0 ); // mono input al secondary codec

	pxa_ac97_refcount++;
	up(&pxa_ac97_mutex_1);
// 	*codec = &pxa_ac97_codec; era solo cosi'qua sotto !!! parm
	*codec = &pxa_ac97_codec1;

	return 0;
}
# endif

#ifdef CODEC_PRIMARY
void pxa_ac97_put(void)
{
printk ("pxa_ac97_put \n");
	down(&pxa_ac97_mutex);
	pxa_ac97_refcount--;
	if (!pxa_ac97_refcount) {
		GCR = GCR_ACLINK_OFF;
		CKEN &= ~CKEN2_AC97; 
	}
	up(&pxa_ac97_mutex);
}
#endif

#ifdef CODEC_SECONDARY
void pxa_ac97_put1(void)
{
	down(&pxa_ac97_mutex_1);
	pxa_ac97_refcount--;
	if (!pxa_ac97_refcount) {
		GCR = GCR_ACLINK_OFF;
		CKEN &= ~CKEN2_AC97; 
	}
	up(&pxa_ac97_mutex_1);
}
#endif

#ifdef CODEC_PRIMARY
EXPORT_SYMBOL(pxa_ac97_get);
EXPORT_SYMBOL(pxa_ac97_put);
#endif

#ifdef CODEC_SECONDARY
EXPORT_SYMBOL(pxa_ac97_get1);
EXPORT_SYMBOL(pxa_ac97_put1);

#endif

/*
 * Audio Mixer stuff
 */

#ifdef CODEC_PRIMARY
static audio_state_t ac97_audio_state;
static audio_stream_t ac97_audio_in;
#endif

#ifdef CODEC_SECONDARY
static audio_state_t ac97_audio1_state;
static audio_stream_t ac97_audio1_in;
#endif

#ifdef CODEC_PRIMARY
static int mixer_ioctl( struct inode *inode, struct file *file,
			unsigned int cmd, unsigned long arg)
{
	int ret, val;
//printk ("mixer_ioctl cmd %d \n", cmd);

	ret = pxa_ac97_codec.mixer_ioctl(&pxa_ac97_codec, cmd, arg);
	if (ret)
		return ret;

	/* We must snoop for some commands to provide our own extra processing */
	switch (cmd) {
	case SOUND_MIXER_WRITE_RECSRC:
		/*
		 * According to the PXA250 spec, mic-in should use different
		 * DRCMR and different AC97 FIFO.
		 * Unfortunately current UCB1400 versions (up to ver 2A) don't
		 * produce slot 6 for the audio input frame, therefore the PXA
		 * AC97 mic-in FIFO is always starved.
		 */
#if 0
		ret = get_user(val, (int *)arg);
		if (ret)
			return ret;
		pxa_audio_clear_buf(&ac97_audio_in);
		*ac97_audio_in.drcmr = 0;
		if (val & (1 << SOUND_MIXER_MIC)) {
			ac97_audio_in.dcmd = DCMD_RXMCDR;
			ac97_audio_in.drcmr = &DRCMRRXMCDR;
			ac97_audio_in.dev_addr = __PREG(MCDR);
		} else {
			ac97_audio_in.dcmd = DCMD_RXPCDR;
			ac97_audio_in.drcmr = &DRCMRRXPCDR;
			ac97_audio_in.dev_addr = __PREG(PCDR);
		}
		if (ac97_audio_state.rd_ref)
			*ac97_audio_in.drcmr =
				ac97_audio_in.dma_ch | DRCMR_MAPVLD;
#endif
		break;
	}
	return 0;
}
#endif


#ifdef CODEC_SECONDARY
static int mixer1_ioctl( struct inode *inode, struct file *file,
			unsigned int cmd, unsigned long arg)
{
	int ret, val;
//printk ("mixer_ioctl 1 cmd %d \n", cmd);

	ret = pxa_ac97_codec1.mixer_ioctl(&pxa_ac97_codec1, cmd, arg);
	if (ret)
		return ret;

	/* We must snoop for some commands to provide our own extra processing */
	switch (cmd) {
	case SOUND_MIXER_WRITE_RECSRC:
		/*
		 * According to the PXA250 spec, mic-in should use different
		 * DRCMR and different AC97 FIFO.
		 * Unfortunately current UCB1400 versions (up to ver 2A) don't
		 * produce slot 6 for the audio input frame, therefore the PXA
		 * AC97 mic-in FIFO is always starved.
		 */
#if 0
		ret = get_user(val, (int *)arg);
		if (ret)
			return ret;
		pxa_audio_clear_buf(&ac97_audio1_in);
		*ac97_audio1_in.drcmr = 0;
		if (val & (1 << SOUND_MIXER_MIC)) {
			ac97_audio1_in.dcmd = DCMD_RXMCDR;
			ac97_audio1_in.drcmr = &DRCMRRXMCDR;
			ac97_audio1_in.dev_addr = __PREG(MCDR);
		} else {
			ac97_audio1_in.dcmd = DCMD_RXPCDR;
			ac97_audio1_in.drcmr = &DRCMRRXPCDR;
			ac97_audio1_in.dev_addr = __PREG(PCDR);
		}
		if (ac97_audio1_state.rd_ref)
			*ac97_audio1_in.drcmr =
				ac97_audio1_in.dma_ch | DRCMR_MAPVLD;
#endif
		break;
	}
	return 0;
}
#endif

#ifdef CODEC_PRIMARY
static struct file_operations mixer_fops = {
	ioctl:		mixer_ioctl,
	llseek:		no_llseek,
	owner:		THIS_MODULE
};
#endif

#ifdef CODEC_SECONDARY
static struct file_operations mixer1_fops = {
	ioctl:		mixer1_ioctl,
	llseek:		no_llseek,
	owner:		THIS_MODULE
};
#endif

/*
 * AC97 codec ioctls
 */

static int codec_adc_rate = 48000;
static int codec_dac_rate = 48000;

#ifdef CODEC_PRIMARY
static int ac97_ioctl(struct inode *inode, struct file *file,
		      unsigned int cmd, unsigned long arg)
{
	int ret;
	long val;
	u16 valVRA;

printk( "access to ac97 ioctl ");
	switch(cmd) {

	case SNDCTL_DSP_STEREO:
printk( " DSP stereo \n");

		ret = get_user(val, (int *) arg);
		if (ret)
			return ret;
		/* FIXME: do we support mono? */
		ret = (val == 0) ? -EINVAL : 1;
		return put_user(ret, (int *) arg);

	case SNDCTL_DSP_CHANNELS:
	case SOUND_PCM_READ_CHANNELS:
		/* FIXME: do we support mono? */
//printk( " PCM read channels \n");

		return put_user(2, (long *) arg);

	case SNDCTL_DSP_SPEED:
printk( " DSP speed \n");

		ret = get_user(val, (long *) arg);
		if (ret)
			return ret;

		/* Variable Sample Rate Enabling Included by Carlos otherwise Fs always 48KHz */
		valVRA = pxa_ac97_read(&pxa_ac97_codec,AC97_EXTENDED_STATUS );
		pxa_ac97_write(&pxa_ac97_codec,AC97_EXTENDED_STATUS, valVRA | 0x0001  );
//printk(" Variable Sample Rate Enabling Included by Carlos otherwise Fs always 48KHz\n");

		if (file->f_mode & FMODE_READ)
			codec_adc_rate = ac97_set_adc_rate(&pxa_ac97_codec, val);
		if (file->f_mode & FMODE_WRITE)
			codec_dac_rate = ac97_set_dac_rate(&pxa_ac97_codec, val);
		/* fall through */


/*              valVRA = pxa_ac97_read(&pxa_ac97_codec,AC97_RECORD_GAIN);
                printk("Get RECLEV %x\n",valVRA);
                pxa_ac97_write(&pxa_ac97_codec,AC97_RECORD_GAIN, 0x0c0c);
                printk("Set RECLEV to 80%%\n") ;
                valVRA = pxa_ac97_read(&pxa_ac97_codec,AC97_RECORD_GAIN);
                printk("reGet RECLEV %x\n",valVRA); */


	case SOUND_PCM_READ_RATE:
//printk( " PCM read rate \n");

		if (file->f_mode & FMODE_READ)
			val = codec_adc_rate;
		if (file->f_mode & FMODE_WRITE)
			val = codec_dac_rate;
		return put_user(val, (long *) arg);

	case SNDCTL_DSP_SETFMT:
	case SNDCTL_DSP_GETFMTS:
		/* FIXME: can we do other fmts? */
//printk( " DSP_GETFMT/GETFMTS \n");

		return put_user(AFMT_S16_LE, (long *) arg);

	default:
		/* Maybe this is meant for the mixer (As per OSS Docs) */
printk( " unknown \n");
		return mixer_ioctl(inode, file, cmd, arg);
	}
	return 0;
}
#endif

#ifdef CODEC_SECONDARY
static int ac97_ioctl_1(struct inode *inode, struct file *file,
		      unsigned int cmd, unsigned long arg)
{
	int ret;
	long val;
	u16 valVRA;

//printk( "access to ioctl 1");

	switch(cmd) {

	case SNDCTL_DSP_STEREO:
printk( " DSP stereo \n");

		ret = get_user(val, (int *) arg);
		if (ret)
			return ret;
		/* FIXME: do we support mono? */
		ret = (val == 0) ? -EINVAL : 1;
		return put_user(ret, (int *) arg);

	case SNDCTL_DSP_CHANNELS:
	case SOUND_PCM_READ_CHANNELS:
		/* FIXME: do we support mono? */
//printk( " PCM read channels \n");

		return put_user(2, (long *) arg);
//		return put_user(1, (long *) arg);	// forzato al mono da noi, non funziona molto bene pero'

	case SNDCTL_DSP_SPEED:
//printk( "access to ioctl DSP speed \n");

		ret = get_user(val, (long *) arg);
		if (ret)
			return ret;

		/* Variable Sample Rate Enabling Included by Carlos otherwise Fs always 48KHz */
		valVRA = pxa_ac97_read1(&pxa_ac97_codec1,AC97_EXTENDED_STATUS );
		pxa_ac97_write1(&pxa_ac97_codec1,AC97_EXTENDED_STATUS, valVRA | 0x0001 );
//printk("Variable Sample Rate Enabling Included by Carlos otherwise Fs always 48KHz\n");

		if (file->f_mode & FMODE_READ)
			codec_adc_rate = ac97_set_adc_rate(&pxa_ac97_codec1, val);
		if (file->f_mode & FMODE_WRITE)
			codec_dac_rate = ac97_set_dac_rate(&pxa_ac97_codec1, val);
		/* fall through */

/*              valVRA = pxa_ac97_read(&pxa_ac97_codec,AC97_RECORD_GAIN);
                printk("Get RECLEV %x\n",valVRA);
                pxa_ac97_write(&pxa_ac97_codec,AC97_RECORD_GAIN, 0x0c0c);
                printk("Set RECLEV to 80%%\n") ;
                valVRA = pxa_ac97_read(&pxa_ac97_codec,AC97_RECORD_GAIN);
                printk("reGet RECLEV %x\n",valVRA); */


	case SOUND_PCM_READ_RATE:
//printk( " PCM read rate \n");

		if (file->f_mode & FMODE_READ)
			val = codec_adc_rate;
		if (file->f_mode & FMODE_WRITE)
			val = codec_dac_rate;
		return put_user(val, (long *) arg);

	case SNDCTL_DSP_SETFMT:
	case SNDCTL_DSP_GETFMTS:
		/* FIXME: can we do other fmts? */
//printk( " DSP_GETFMT/GETFMTS \n");

		return put_user(AFMT_S16_LE, (long *) arg);

	default:
		/* Maybe this is meant for the mixer (As per OSS Docs) */
//printk( " unknown \n");
		return mixer_ioctl(inode, file, cmd, arg);
	}
	return 0;
}
#endif

/*
 * Audio stuff
 */

#ifdef CODEC_PRIMARY
static audio_stream_t ac97_audio_out = {
	name:			"AC97 audio out",
	dcmd:			DCMD_TXPCDR,
	drcmr:			&DRCMRTXPCDR,
	dev_addr:		__PREG(PCDR),
};

static audio_stream_t ac97_audio_in = {
	name:			"AC97 audio in",
	dcmd:			DCMD_RXPCDR,
	drcmr:			&DRCMRRXPCDR,
	dev_addr:		__PREG(PCDR),
};

static audio_state_t ac97_audio_state = {
	output_stream:		&ac97_audio_out,
	input_stream:		&ac97_audio_in,
	client_ioctl:		ac97_ioctl,
	sem:			__MUTEX_INITIALIZER(ac97_audio_state.sem),
};

/*
 * Missing fields of this structure will be patched with the call
 * to pxa_audio_attach().
 */


static int ac97_audio_open(struct inode *inode, struct file *file)
{
printk ("audio open\n");
	return pxa_audio_attach(inode, file, &ac97_audio_state);
}

static struct file_operations ac97_audio_fops = {
	open:		ac97_audio_open,
	owner:		THIS_MODULE
};
#endif


#ifdef CODEC_SECONDARY

				// ---- configurazione input mic, output modem
#define DCMD_RXMCDR_mono (DCMD_INCTRGADDR|DCMD_FLOWSRC|DCMD_BURST32|DCMD_WIDTH4)
#define DCMD_TXPCDR_mono (DCMD_INCSRCADDR|DCMD_BURST32|DCMD_WIDTH4)


static audio_stream_t ac97_audio1_out = {
	name:			"AC97 audio1 out",
	dcmd:			DCMD_TXPCDR,// modem codec
	drcmr:			&DRCMRTXMODR,
	dev_addr:		__PREG(MODR), // buffer del modem
};


static audio_stream_t ac97_audio1_in = {
	name:			"AC97 audio1 in",
	dcmd:			DCMD_RXMCDR,	// flag di settaggi per il DMA 
	drcmr:			&DRCMRRXMCDR,	// add.  0x40000120, ch.register for ac97 DRCMR8
	dev_addr:		__PREG(MCDR), // MIC FIFO data register
};

/*	--- confuigurazione per il secondary in audio pure esso
static audio_stream_t ac97_audio1_in = {
	name:			"AC97 audio1 in",
	dcmd:			DCMD_RXPCDR,
	drcmr:			&DRCMRRXPCDR,
	dev_addr:		__PREG(PCDR),	// questi erano al primary
};

static audio_stream_t ac97_audio1_out = {
	name:			"AC97 audio1 out",
	dcmd:			DCMD_TXPCDR,	// buffer audio, lo stesso del primary
	drcmr:			&DRCMRTXPCDR,
	dev_addr:		__PREG(PCDR),
};
*/


static audio_state_t ac97_audio1_state = {
	output_stream:		&ac97_audio1_out,
	input_stream:		&ac97_audio1_in,
	client_ioctl:		ac97_ioctl_1,
	sem:			__MUTEX_INITIALIZER(ac97_audio1_state.sem),
};

static int ac97_audio1_open(struct inode *inode, struct file *file)
{
//printk ("audio1 open\n");
	return pxa_audio_attach(inode, file, &ac97_audio1_state);
}

/*
 * Missing fields of this structure will be patched with the call
 * to pxa_audio_attach().
 */


static struct file_operations ac97_audio1_fops = {
	open:		ac97_audio1_open,
	owner:		THIS_MODULE
};
#endif


static int __init pxa_ac97_init(void)
{
	int ret;
	struct ac97_codec *dummy;

#ifdef CODEC_PRIMARY
printk("primary codec pxa ac97 init \n");
	ret = pxa_ac97_get(&dummy);
	if (ret)
		return ret;
	ac97_audio_state.dev_dsp = register_sound_dsp(&ac97_audio_fops, -1);
	pxa_ac97_codec.dev_mixer = register_sound_mixer(&mixer_fops, -1);
#endif

#ifdef CODEC_SECONDARY
//printk("secondo codec pxa ac97 init \n");
	ret = pxa_ac97_get1(&dummy);
	if (ret)
		return ret;
	ac97_audio1_state.dev_dsp = register_sound_dsp(&ac97_audio1_fops, -1);
	pxa_ac97_codec1.dev_mixer = register_sound_mixer(&mixer1_fops, -1);
#endif

#ifdef CONFIG_PM
        ac97_pm_dev = pm_register(PM_SYS_DEV, 0, ac97_pm_callback);
#endif

	return 0;
}


static void __exit pxa_ac97_exit(void)
{
#ifdef CONFIG_PM
	if(ac97_pm_dev)
		pm_unregister(ac97_pm_dev);
#endif

#ifdef CODEC_SECONDARY
	unregister_sound_dsp(ac97_audio1_state.dev_dsp);
	unregister_sound_mixer(pxa_ac97_codec1.dev_mixer);
	pxa_ac97_put1();

#endif

#ifdef CODEC_PRIMARY
	unregister_sound_dsp(ac97_audio_state.dev_dsp);
	unregister_sound_mixer(pxa_ac97_codec.dev_mixer);
	pxa_ac97_put();
#endif

}

module_init(pxa_ac97_init);
module_exit(pxa_ac97_exit);
