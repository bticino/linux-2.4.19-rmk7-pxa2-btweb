/*
 *  linux/drivers/char/pxa-ssp-legerity.c
 *
 *  Author: Raffaele Recalcati 
 *  Created: 2007,2008
 *  Copyright: BTicino S.p.A.
 *
 *  Driver for interfacing with Legerity codec based on pxa255 ssp port
 *
 *  PERMISSION IS HEREBY GRANTED TO USE, COPY AND MODIFY THIS SOFTWARE
 *  ONLY FOR THE PURPOSE OF DEVELOPING LEGERITY RELATED PRODUCTS.
 *
 *  THIS SOFTWARE MAY NOT BE DISTRIBUTED TO ANY PARTY WHICH IS NOT COVERED
 *  BY LEGERITY NON-DISCLOSURE AGREEMENT (NDA). UNAUTHORIZED DISCLOSURE
 *  IS AGAINST LAW AND STRICTLY PROHIBITED.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
 *  EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 *  WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
*/

#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/config.h>
#include <linux/version.h>
#include <asm/hardware.h> /* for btweb_globals when ARCH_BTWEB is set */
#include <asm/mach-types.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#ifdef CONFIG_ARCH_PXA
#include <asm/arch/pxa-regs.h>
#endif
 
/* Mode */
#undef CODEC_INTERRUPT_MODE
#undef TX_DELAY    /* not using register fifo status to wait, but only wait */
#undef RX_DELAY



/* DEBUG */
#undef DEBUG  

#ifdef DEBUG
        #define deb(format, arg...) printk(KERN_DEBUG __FILE__ ": " format "\n" , ## arg)
#else
        #define deb(format, arg...) do {} while (0)
#endif

#define trace(format, arg...) printk(KERN_INFO __FILE__ ": " format "\n" , ## arg)

/* DEFINES */
#define CODEC_INT1 59
#define CODEC_INT2 60
#define CODEC_INT3 65
#define CODEC_EXT_D 9


#define PXA_SSP_WRITE 5500
#define PXA_SSP_READ  5501
#define CSON(x) switch (x) {\
		/* FXS Codec number 1 from the rigth : minor 0 */ \
		case 0:\
		GPCR(btweb_features.pbx_cs1_d)= GPIO_bit(btweb_features.pbx_cs1_d); \
		GPCR(btweb_features.pbx_cs2_d)= GPIO_bit(btweb_features.pbx_cs2_d); \
		GPCR(btweb_features.pbx_cs3_d)= GPIO_bit(btweb_features.pbx_cs3_d); \
		GPCR(btweb_features.pbx_cs4_d)= GPIO_bit(btweb_features.pbx_cs4_d); \
		GPCR(btweb_features.pbx_cs5_d)= GPIO_bit(btweb_features.pbx_cs5_d); \
		break;\
		/* FXS Codec number 2 from the rigth : minor 1 */ \
		case 1:\
		GPSR(btweb_features.pbx_cs1_d)= GPIO_bit(btweb_features.pbx_cs1_d); \
		GPCR(btweb_features.pbx_cs2_d)= GPIO_bit(btweb_features.pbx_cs2_d); \
		GPCR(btweb_features.pbx_cs3_d)= GPIO_bit(btweb_features.pbx_cs3_d); \
		GPCR(btweb_features.pbx_cs4_d)= GPIO_bit(btweb_features.pbx_cs4_d); \
		GPCR(btweb_features.pbx_cs5_d)= GPIO_bit(btweb_features.pbx_cs5_d); \
		break;\
		/* FXS Codec number 3 from the rigth : minor 2 */ \
		case 2:\
		GPCR(btweb_features.pbx_cs1_d)= GPIO_bit(btweb_features.pbx_cs1_d); \
		GPSR(btweb_features.pbx_cs2_d)= GPIO_bit(btweb_features.pbx_cs2_d); \
		GPCR(btweb_features.pbx_cs3_d)= GPIO_bit(btweb_features.pbx_cs3_d); \
		GPCR(btweb_features.pbx_cs4_d)= GPIO_bit(btweb_features.pbx_cs4_d); \
		GPCR(btweb_features.pbx_cs5_d)= GPIO_bit(btweb_features.pbx_cs5_d); \
		break;\
		/* FXS Codec number 4 from the rigth : minor 3 */ \
		case 3:\
		GPSR(btweb_features.pbx_cs1_d)= GPIO_bit(btweb_features.pbx_cs1_d); \
		GPSR(btweb_features.pbx_cs2_d)= GPIO_bit(btweb_features.pbx_cs2_d); \
		GPCR(btweb_features.pbx_cs3_d)= GPIO_bit(btweb_features.pbx_cs3_d); \
		GPCR(btweb_features.pbx_cs4_d)= GPIO_bit(btweb_features.pbx_cs4_d); \
		GPCR(btweb_features.pbx_cs5_d)= GPIO_bit(btweb_features.pbx_cs5_d); \
		break;\
		/* FXO Codec number 1 from the rigth : minor 4 */ \
		case 4:\
		GPCR(btweb_features.pbx_cs1_d)= GPIO_bit(btweb_features.pbx_cs1_d); \
		GPCR(btweb_features.pbx_cs2_d)= GPIO_bit(btweb_features.pbx_cs2_d); \
		GPSR(btweb_features.pbx_cs3_d)= GPIO_bit(btweb_features.pbx_cs3_d); \
		GPCR(btweb_features.pbx_cs4_d)= GPIO_bit(btweb_features.pbx_cs4_d); \
		GPCR(btweb_features.pbx_cs5_d)= GPIO_bit(btweb_features.pbx_cs5_d); \
		break;\
		/* FXO Codec number 2 from the rigth : minor 5 */ \
		case 5:\
		GPSR(btweb_features.pbx_cs1_d)= GPIO_bit(btweb_features.pbx_cs1_d); \
		GPCR(btweb_features.pbx_cs2_d)= GPIO_bit(btweb_features.pbx_cs2_d); \
		GPSR(btweb_features.pbx_cs3_d)= GPIO_bit(btweb_features.pbx_cs3_d); \
		GPCR(btweb_features.pbx_cs4_d)= GPIO_bit(btweb_features.pbx_cs4_d); \
		GPCR(btweb_features.pbx_cs5_d)= GPIO_bit(btweb_features.pbx_cs5_d); \
		break;\
		/* FXO Codec number 3 - SCS          : minor 6 */ \
		case 6:\
		GPCR(btweb_features.pbx_cs1_d)= GPIO_bit(btweb_features.pbx_cs1_d); \
		GPSR(btweb_features.pbx_cs2_d)= GPIO_bit(btweb_features.pbx_cs2_d); \
		GPSR(btweb_features.pbx_cs3_d)= GPIO_bit(btweb_features.pbx_cs3_d); \
		GPCR(btweb_features.pbx_cs4_d)= GPIO_bit(btweb_features.pbx_cs4_d); \
		GPCR(btweb_features.pbx_cs5_d)= GPIO_bit(btweb_features.pbx_cs5_d); \
		break;\
		/* FXO Codec number 4 - MUSIC        : minor 7 */ \
		case 7:\
		GPSR(btweb_features.pbx_cs1_d)= GPIO_bit(btweb_features.pbx_cs1_d); \
		GPSR(btweb_features.pbx_cs2_d)= GPIO_bit(btweb_features.pbx_cs2_d); \
		GPSR(btweb_features.pbx_cs3_d)= GPIO_bit(btweb_features.pbx_cs3_d); \
		GPCR(btweb_features.pbx_cs4_d)= GPIO_bit(btweb_features.pbx_cs4_d); \
		GPCR(btweb_features.pbx_cs5_d)= GPIO_bit(btweb_features.pbx_cs5_d); \
		break;\
		/* LE78D11 Codec number 1 - AUX      : minor 8 */ \
		case 8:\
		GPCR(btweb_features.pbx_cssa_d)=GPIO_bit(btweb_features.pbx_cssa_d);\
		break;\
		/* LE78D11 Codec number 2 - AUX      : minor 9 */ \
		case 9:\
		GPCR(btweb_features.pbx_cssb_d)=GPIO_bit(btweb_features.pbx_cssb_d);\
		break;\
		case 10:\
		GPCR(btweb_features.pbx_cs_clid1_d)=GPIO_bit(btweb_features.pbx_cs_clid1_d);\
		break;\
		case 11:\
		GPCR(btweb_features.pbx_cs_clid2_d)=GPIO_bit(btweb_features.pbx_cs_clid2_d);\
		break;\
		/* DISABLE ALL CHIP-SELECT */ \
		default:\
		GPSR(btweb_features.pbx_cs1_d)= GPIO_bit(btweb_features.pbx_cs1_d); \
		GPSR(btweb_features.pbx_cs2_d)= GPIO_bit(btweb_features.pbx_cs2_d); \
		GPSR(btweb_features.pbx_cs3_d)= GPIO_bit(btweb_features.pbx_cs3_d); \
		GPSR(btweb_features.pbx_cs4_d)= GPIO_bit(btweb_features.pbx_cs4_d); \
		GPSR(btweb_features.pbx_cs5_d)= GPIO_bit(btweb_features.pbx_cs5_d); \
		GPSR(btweb_features.pbx_cssa_d)=GPIO_bit(btweb_features.pbx_cssa_d);\
		GPSR(btweb_features.pbx_cssb_d)=GPIO_bit(btweb_features.pbx_cssb_d);\
		GPSR(btweb_features.pbx_cs_clid1_d)=GPIO_bit(btweb_features.pbx_cs_clid1_d);\
		GPSR(btweb_features.pbx_cs_clid2_d)=GPIO_bit(btweb_features.pbx_cs_clid2_d);\
	}


#define CSOFF CSON(0xFF);

static int ctrlssp_active=0;
static int voicedev=-1;

#ifdef CODEC_INTERRUPT_MODE
static int interrupt_pending;
/* driver/filesystem interface management */
static wait_queue_head_t queue;
static struct fasync_struct *asyncptr;
static struct semaphore reader_lock;

struct codec_interrupt_type {
	int     bounce_interval;
	int     irq[MAX_CODEC_IRQ];
	int     io[MAX_CODEC_IRQ];
};

static struct codec_interrupt_type codec_interrupt  = {
        bounce_interval:        1,
        io:                     {CODEC_INT1,CODEC_INT2},
};

static char banner[] __initdata = KERN_INFO "pxa-ssp-legerity at 0x%X, irq %d.\n";
#endif

#define SSP_TIMEOUT 100000
int ssp_timeout = SSP_TIMEOUT;

#define MAX_CODEC_IRQ 4




static void ctrlssp_set_gpio(void)
{
	set_GPIO_mode(GPIO23_SCLK_md); /* lowercase: buglet in pxa-regs.h */
/*	set_GPIO_mode(GPIO24_SFRM_MD); */
	set_GPIO_mode(GPIO25_STXD_MD);
	set_GPIO_mode(GPIO26_SRXD_MD);
}

#define small_delay(n) (\
	{unsigned long nloop=(n); volatile static int i; while (nloop--){ i++; } })

#ifdef CODEC_INTERRUPT_MODE
/**
 * poll_ssp:
 *
 * @file: File of the Io device
 * @wait: Poll table
 *  
 * The pad is ready to read if there is a button or any position change
 * pending in the queue. The reading and interrupt routines maintain the
 * required state for us and do needed wakeups.
 *
*/

static unsigned int poll_ssp(struct file *file, poll_table * wait)
{
	poll_wait(file, &queue, wait);
	if (interrupt_pending)
		return POLLIN | POLLRDNORM;
	return 0;
}

/*
 *  * Timer goes off a short while after an up/down transition and copies
 *   * the value of raw_down to debounced_down.
 *    */

static void bounce_timeout(unsigned long data);
static struct timer_list bounce_timer = { function: bounce_timeout };

/**
 *      wake_readers:
 *
 *      Take care of letting any waiting processes know that
 *      now would be a good time to do a read().  Called
 *      whenever a state transition occurs, real or synthetic. Also
 *      issue any SIGIO's to programs that use SIGIO on mice (eg
 *      Executor)
*/

static void wake_readers(void)
{
	wake_up_interruptible(&queue);
	kill_fasync(&asyncptr, SIGIO, POLL_IN);
}

/*
 * notify_interrupt:
 *
 * Called by the raw pad read routines when a (debounced) up/down
 * transition is detected.
*/

static void notify_interrupt(void)
{
	wake_readers();
}



/*
 * bounce_timeout:
 * @data: Unused
 *
 * No further up/down transitions happened within the
 * bounce period, so treat this as a genuine transition.
*/
static void bounce_timeout(unsigned long data)
{
	if ( (!(GPLR(codec_interrupt.io[0]) & GPIO_bit(codec_interrupt.io[0]))) || \
		(!(GPLR(codec_interrupt.io[1]) & GPIO_bit(codec_interrupt.io[1]))) ) {
		printk(".\n");
               notify_interrupt();
        }
        else
		printk(":\n");
}




/**
 * ssp_irq:
 * @irq: Interrupt number
 * @ptr: Unused
 * @regs: Unused
 *
 * Callback when pad's irq goes off; copies values in to raw_* globals;
*/
static void ssp_irq(int irq, void *ptr, struct pt_regs *regs)
{
	if ( (!(GPLR(codec_interrupt.io[0]) & GPIO_bit(codec_interrupt.io[0]))) || \
		(!(GPLR(codec_interrupt.io[1]) & GPIO_bit(codec_interrupt.io[1]))) ) {
#if DEBOUNCE
		mod_timer(&bounce_timer,jiffies+codec_interrupt.bounce_interval);
#else
		printk(".\n");
		notify_interrupt();
#endif
	}

}
#endif


static int
ioctl_ssp( struct inode *inode, struct file *file,
                unsigned int cmd, unsigned long arg)
{
        unsigned char val=0;
        unsigned char ssdr,tmp;

	voicedev = MINOR(inode->i_rdev);

        switch (cmd) {
                case PXA_SSP_WRITE: 
			if (copy_from_user(&val, (unsigned char*)arg, sizeof(val))){
	                        deb("copy_from_user error \n");
				return -EFAULT;
			}
			CSON(voicedev);
			//small_delay(1);
			deb("ioctl_ssp:wr %x to CSON=%d\n",val,voicedev);
#if TX_DELAY
			SSDR = 0xff&val;
			udelay(7);
			CSOFF;
			udelay(7);
#else

			ssp_timeout=SSP_TIMEOUT;
			while ( (SSSR & SSSR_TFL0) || (SSSR & SSSR_BSY) ) {
				if (!--ssp_timeout)
					return -ETIMEDOUT;
			}

			SSDR = 0xff&val;

			ssp_timeout=SSP_TIMEOUT;
			while ( (SSSR & SSSR_TFL0) || (SSSR & SSSR_BSY) ) {
				if (!--ssp_timeout)
					return -ETIMEDOUT;
			}

			// Hold time
			small_delay(1);

			CSOFF;

			// Off time
			udelay(3);
#endif

			/* trailing read */
#if TX_DELAY
			tmp=SSDR;
#else
                        ssp_timeout=SSP_TIMEOUT;
                        while (!(SSSR & SSSR_RNE)) {
                                if (!--ssp_timeout)
                                        return -ETIMEDOUT;
                        }
                        tmp=0x00ff&SSDR;

#endif

                break;
                case PXA_SSP_READ:
			CSON(voicedev);
			//small_delay(1);

#if RX_DELAY
			SSDR=0xff;
			udelay(7);
			ssdr=0x00ff&SSDR;
			deb("ioctl_ssp:rd %x from CSON=%d\n",ssdr,voicedev);
			udelay(7);
			CSOFF;
			udelay(7);
#else
			/* 0xff write: I write in order to read */
			ssp_timeout=SSP_TIMEOUT;
			while (!(SSSR & SSSR_TNF)) {
				if (!--ssp_timeout)
					return -ETIMEDOUT;
			}
			SSDR = 0xff;
			ssp_timeout=SSP_TIMEOUT;
			while (!(SSSR & SSSR_TNF)) {
				if (!--ssp_timeout)
					return -ETIMEDOUT;
			}

			/* read */
			ssp_timeout=SSP_TIMEOUT;
			while (!(SSSR & SSSR_RNE)) {
				if (!--ssp_timeout)
					return -ETIMEDOUT;
			}
			ssdr=0x00ff&SSDR;
			CSOFF;
			small_delay(1);
#endif
			if(copy_to_user((void *)arg, &ssdr, sizeof(ssdr))) {
				deb("ioctl_ssp: no copytouser\n");
				return -EFAULT;
			}
		break;
	}
	return 0;
}


#define MINOR_MIN 0
#define MINOR_MAX 9
#define MAX_OPENED 10



static int open_ssp(struct inode *inode, struct file *filp)
{
	int minor = MINOR(inode->i_rdev);
        deb(KERN_INFO "CTRLSSP opening: minor=%d\n",minor);
	voicedev=minor;

	if(ctrlssp_active>MAX_OPENED){
		printk(KERN_INFO "open_ssp not done - exceeded MAX_OPENED\n");
		return -EBUSY;
	}
	if ((minor<MINOR_MIN)||(minor>MINOR_MAX)){
		deb(KERN_INFO "open_ssp not done - wrong minor\n");
		return -EINVAL;
	}
	
	ctrlssp_active++;
	deb(KERN_INFO "open_ssp done\n");

	
	
	return 0;
}

static int release_ssp(struct inode *inode, struct file *filp)
{
	ctrlssp_active--;
	return 0;
}

static struct file_operations ctrlssp_fops = {
        owner:    THIS_MODULE,
	ioctl:	  ioctl_ssp,
/*	poll:	  poll_ssp, */
        open:     open_ssp,
        release:  release_ssp, 
};


int __init ctrlssp_init(void)
{
	unsigned int sscr;

	ctrlssp_set_gpio();

        GPSR(btweb_features.pbx_rst_d)=GPIO_bit(btweb_features.pbx_rst_d);
        mdelay(100);
	/* out of reset state TODO*/
	GPCR(btweb_features.pbx_rst_d)=GPIO_bit(btweb_features.pbx_rst_d);
        mdelay(100);

#ifdef CODEC_INTERRUPT_MODE
	init_MUTEX(&reader_lock);
	set_GPIO_IRQ_edge(codec_interrupt.io[0], GPIO_FALLING_EDGE);
	set_GPIO_IRQ_edge(codec_interrupt.io[1], GPIO_FALLING_EDGE);
        codec_interrupt.irq[0]=GPIO_2_80_TO_IRQ(codec_interrupt.io[0]);
        codec_interrupt.irq[1]=GPIO_2_80_TO_IRQ(codec_interrupt.io[1]);
	if (request_irq(codec_interrupt.irq[0], ssp_irq, 0, "codec_irq", 0)) {
		printk(KERN_ERR "pxa-ssp-legerity: Unable to get IRQ: io=%d.\n",codec_interrupt.io[0]);
		return -EBUSY;
	}
	if (request_irq(codec_interrupt.irq[1], ssp_irq, 0, "codec_irq", 0)) {
		printk(KERN_ERR "pxa-ssp-legerity: Unable to get IRQ. io=%d\n",codec_interrupt.io[1]);
		return -EBUSY;
	}
	init_waitqueue_head(&queue);
	printk(banner, codec_interrupt.io[0], codec_interrupt.irq[0]);
	printk(banner, codec_interrupt.io[1], codec_interrupt.irq[1]);
#endif


	SSSR = 0x0080;
//	SSCR0 = 0x1107;  /* 0x11yy=100000 Hz */
	SSCR0 = 0x0007;  /* 0x00yy=1228800 Hz */

	SSCR1 = 0x0018;  /* 0x8=sclk idle is hi */
	CKEN |= CKEN3_SSP;
	sscr = SSCR0;
	sscr |= (1<<7);
	SSCR0 = sscr;

        if (register_chrdev(100, "voice", &ctrlssp_fops))
        {
                deb(KERN_NOTICE "Can't allocate major number %d for Legerity Codecs.\n",
                       100);
                return -EAGAIN;
        }

	deb(KERN_INFO "CTRLSSP ssp Driver\n");
	return 0;
}
 
void __exit ctrlssp_exit(void)
{
	unsigned int sscr;

	unregister_chrdev(100, "voice");

	/* return to RESET state TODO*/
	GPSR(btweb_features.pbx_rst_d)= GPIO_bit(btweb_features.pbx_rst_d);

	sscr = SSCR0;
	sscr &= ~(1<<7);
	SSCR0 = sscr;
	CKEN &= ~(CKEN3_SSP);
}

module_init(ctrlssp_init);
module_exit(ctrlssp_exit);

MODULE_AUTHOR("Raffaele Recalcati");
MODULE_DESCRIPTION("SSP control driver");

EXPORT_NO_SYMBOLS;
