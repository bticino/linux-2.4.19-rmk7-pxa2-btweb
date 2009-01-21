/*
 * Glue audio driver for the HP iPAQ H3900 & Philips UDA1380 codec.
 *
 * Copyright (c) 2000 Nicolas Pitre <nico@cam.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License.
 *
 * This is the machine specific part of the HP iPAQ (aka Bitsy) support.
 * This driver makes use of the UDA1380 and the pxa-audio modules.
 *
 * History:
 *
 * 2002-12-18   Jamey Hicks     Adapted from h3600-uda1341.c
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/sound.h>
#include <linux/soundcard.h>
#include <linux/i2c.h>
#include <linux/kmod.h>

#include <asm/semaphore.h>
#include <asm/uaccess.h>
#include <asm/hardware.h>
#include <asm/dma.h>
#include <asm/mach-types.h>

#include "pxa-i2s.h"

#undef DEBUG
#ifdef DEBUG
#define DPRINTK( x... )  printk( ##x )
#else
#define DPRINTK( x... )
#endif


#define AUDIO_NAME		"Btweb_CS4334"

#define AUDIO_RATE_DEFAULT	44100


/*
 * Audio interface
 */

static long audio_samplerate = AUDIO_RATE_DEFAULT;

static void btweb_set_samplerate(long val)
{
	pxa_i2s_set_samplerate (val);
	audio_samplerate = val;
}

static void btweb_audio_init(void *data)
{
	pxa_i2s_init ();

	/* Should enable the audio power */

}

static void btweb_audio_shutdown(void *data)
{
	/* should disable the audio power and all signals to the audio chip */
	pxa_i2s_shutdown ();
}

static int btweb_audio_ioctl(struct inode *inode, struct file *file,
			     uint cmd, ulong arg)
{
	long val;
	int ret = 0;

	/*
	 * These are platform dependent ioctls which are not handled by the
	 * generic pxa-audio module.
	 */
	switch (cmd) {
	case SNDCTL_DSP_STEREO:
		ret = get_user(val, (int *) arg);
		if (ret)
			return ret;
		/* the UDA1380 is stereo only */
		ret = (val == 0) ? -EINVAL : 1;
		return put_user(ret, (int *) arg);

	case SNDCTL_DSP_CHANNELS:
	case SOUND_PCM_READ_CHANNELS:
		/* the UDA1380 is stereo only */
		return put_user(2, (long *) arg);

	case SNDCTL_DSP_SPEED:
		ret = get_user(val, (long *) arg);
		if (ret) break;
		btweb_set_samplerate(val);
		/* fall through */

	case SOUND_PCM_READ_RATE:
		return put_user(audio_samplerate, (long *) arg);

	case SNDCTL_DSP_SETFMT:
	case SNDCTL_DSP_GETFMTS:
		/* we can do 16-bit only */
		return put_user(AFMT_S16_LE, (long *) arg);

	default:
		/* Maybe this is meant for the mixer (but we have no mixer) */
		return -ENOTTY;
	}

	return ret;
}



static audio_state_t audio_state = {
	output_stream:		&pxa_i2s_output_stream,
	input_stream:		&pxa_i2s_input_stream,
#if 0 /* there were for power management, but we don't have one */
	hw_init:		btweb_audio_init,
	hw_shutdown:		btweb_audio_shutdown,
#endif
	client_ioctl:		btweb_audio_ioctl,
	sem:			__MUTEX_INITIALIZER(audio_state.sem),
	rd_ref: 0,
	wr_ref: 0,
};

static int btweb_audio_release(struct inode *inode, struct file *file)
{
	btweb_audio_shutdown(NULL /* unused */);
	return pxa_audio_release(inode, file);
}

static int btweb_audio_open(struct inode *inode, struct file *file)
{
	int err;

	/* we haven't any specific data */
	//audio_state.data = NULL;

	/* call the hw_init stuff, as pxa_audio_attach won't do itself */
	btweb_audio_init(NULL /* unused */);
	err = pxa_audio_attach(inode, file, &audio_state);

	return err;
}

/*
 * Missing fields of this structure will be patched with the call
 * to pxa_audio_attach().
 */
static struct file_operations btweb_audio_fops = {
	open:		btweb_audio_open,
	release:	btweb_audio_release,
	owner:		THIS_MODULE
};


static int audio_dev_id;

static int __init btweb_sound_init(void)
{
	if (!machine_is_btweb())
		return -ENODEV;

	/* register devices */
	audio_dev_id = register_sound_dsp(&btweb_audio_fops, -1);

	printk( KERN_INFO "BtWeb I2S sound initialized\n" );
	return 0;
}

static void __exit btweb_sound_exit(void)
{
	unregister_sound_dsp(audio_dev_id);
}

module_init(btweb_sound_init);
module_exit(btweb_sound_exit);

MODULE_AUTHOR("Nicolas Pitre, George France, Jamey Hicks");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Glue audio driver for the HP iPAQ H3900 & Philips UDA1380 codec.");

EXPORT_NO_SYMBOLS;
