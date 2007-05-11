
/*
 *  Driver for interfacing with Legerity codec based on pxa ssp port
 */
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/timer.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/config.h>
#include <linux/version.h>
#include <linux/rtc.h>
#include <linux/string.h>
#include <linux/proc_fs.h>
#include <asm/hardware.h> /* for btweb_globals when ARCH_BTWEB is set */


#include <asm/mach-types.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <asm/hardware.h>
#ifdef CONFIG_ARCH_PXA
#include <asm/arch/pxa-regs.h>
#endif
 
/* DEBUG */
#undef DEBUG 

#ifdef DEBUG
        #define deb(format, arg...) printk(KERN_DEBUG __FILE__ ": " format "\n" , ## arg)
#else
        #define deb(format, arg...) do {} while (0)
#endif

#define trace(format, arg...) printk(KERN_INFO __FILE__ ": " format "\n" , ## arg)

/* DEFINES */
#define PXA_SSP_WRITE 5500
#define PXA_SSP_READ  5501
#define CSON(x) switch (x) {\
		case 0:\
		GPCR(btweb_features.pbx_cs1_d)= GPIO_bit(btweb_features.pbx_cs1_d); \
		GPCR(btweb_features.pbx_cs2_d)= GPIO_bit(btweb_features.pbx_cs2_d); \
		GPCR(btweb_features.pbx_cs3_d)= GPIO_bit(btweb_features.pbx_cs3_d); \
		GPCR(btweb_features.pbx_cs4_d)= GPIO_bit(btweb_features.pbx_cs4_d); \
		GPCR(btweb_features.pbx_cs5_d)= GPIO_bit(btweb_features.pbx_cs5_d); \
		break;\
		case 1:\
		GPSR(btweb_features.pbx_cs1_d)= GPIO_bit(btweb_features.pbx_cs1_d); \
		GPCR(btweb_features.pbx_cs2_d)= GPIO_bit(btweb_features.pbx_cs2_d); \
		GPCR(btweb_features.pbx_cs3_d)= GPIO_bit(btweb_features.pbx_cs3_d); \
		GPCR(btweb_features.pbx_cs4_d)= GPIO_bit(btweb_features.pbx_cs4_d); \
		GPCR(btweb_features.pbx_cs5_d)= GPIO_bit(btweb_features.pbx_cs5_d); \
		break;\
		case 2:\
		GPCR(btweb_features.pbx_cs1_d)= GPIO_bit(btweb_features.pbx_cs1_d); \
		GPSR(btweb_features.pbx_cs2_d)= GPIO_bit(btweb_features.pbx_cs2_d); \
		GPCR(btweb_features.pbx_cs3_d)= GPIO_bit(btweb_features.pbx_cs3_d); \
		GPCR(btweb_features.pbx_cs4_d)= GPIO_bit(btweb_features.pbx_cs4_d); \
		GPCR(btweb_features.pbx_cs5_d)= GPIO_bit(btweb_features.pbx_cs5_d); \
		break;\
		case 3:\
		GPSR(btweb_features.pbx_cs1_d)= GPIO_bit(btweb_features.pbx_cs1_d); \
		GPSR(btweb_features.pbx_cs2_d)= GPIO_bit(btweb_features.pbx_cs2_d); \
		GPCR(btweb_features.pbx_cs3_d)= GPIO_bit(btweb_features.pbx_cs3_d); \
		GPCR(btweb_features.pbx_cs4_d)= GPIO_bit(btweb_features.pbx_cs4_d); \
		GPCR(btweb_features.pbx_cs5_d)= GPIO_bit(btweb_features.pbx_cs5_d); \
		break;\
		case 4:\
		GPCR(btweb_features.pbx_cs1_d)= GPIO_bit(btweb_features.pbx_cs1_d); \
		GPCR(btweb_features.pbx_cs2_d)= GPIO_bit(btweb_features.pbx_cs2_d); \
		GPSR(btweb_features.pbx_cs3_d)= GPIO_bit(btweb_features.pbx_cs3_d); \
		GPCR(btweb_features.pbx_cs4_d)= GPIO_bit(btweb_features.pbx_cs4_d); \
		GPCR(btweb_features.pbx_cs5_d)= GPIO_bit(btweb_features.pbx_cs5_d); \
		break;\
		case 5:\
		GPSR(btweb_features.pbx_cs1_d)= GPIO_bit(btweb_features.pbx_cs1_d); \
		GPCR(btweb_features.pbx_cs2_d)= GPIO_bit(btweb_features.pbx_cs2_d); \
		GPSR(btweb_features.pbx_cs3_d)= GPIO_bit(btweb_features.pbx_cs3_d); \
		GPCR(btweb_features.pbx_cs4_d)= GPIO_bit(btweb_features.pbx_cs4_d); \
		GPCR(btweb_features.pbx_cs5_d)= GPIO_bit(btweb_features.pbx_cs5_d); \
		break;\
		case 6:\
		GPCR(btweb_features.pbx_cs1_d)= GPIO_bit(btweb_features.pbx_cs1_d); \
		GPSR(btweb_features.pbx_cs2_d)= GPIO_bit(btweb_features.pbx_cs2_d); \
		GPSR(btweb_features.pbx_cs3_d)= GPIO_bit(btweb_features.pbx_cs3_d); \
		GPCR(btweb_features.pbx_cs4_d)= GPIO_bit(btweb_features.pbx_cs4_d); \
		GPCR(btweb_features.pbx_cs5_d)= GPIO_bit(btweb_features.pbx_cs5_d); \
		break;\
		case 7:\
		GPSR(btweb_features.pbx_cs1_d)= GPIO_bit(btweb_features.pbx_cs1_d); \
		GPSR(btweb_features.pbx_cs2_d)= GPIO_bit(btweb_features.pbx_cs2_d); \
		GPSR(btweb_features.pbx_cs3_d)= GPIO_bit(btweb_features.pbx_cs3_d); \
		GPCR(btweb_features.pbx_cs4_d)= GPIO_bit(btweb_features.pbx_cs4_d); \
		GPCR(btweb_features.pbx_cs5_d)= GPIO_bit(btweb_features.pbx_cs5_d); \
		break;\
		case 8:\
		GPCR(btweb_features.pbx_cssa_d)=GPIO_bit(btweb_features.pbx_cssa_d);\
		break;\
		case 9:\
		GPCR(btweb_features.pbx_cssb_d)=GPIO_bit(btweb_features.pbx_cssb_d);\
		break;\
		default:\
		GPSR(btweb_features.pbx_cs1_d)= GPIO_bit(btweb_features.pbx_cs1_d); \
		GPSR(btweb_features.pbx_cs2_d)= GPIO_bit(btweb_features.pbx_cs2_d); \
		GPSR(btweb_features.pbx_cs3_d)= GPIO_bit(btweb_features.pbx_cs3_d); \
		GPSR(btweb_features.pbx_cs4_d)= GPIO_bit(btweb_features.pbx_cs4_d); \
		GPSR(btweb_features.pbx_cs5_d)= GPIO_bit(btweb_features.pbx_cs5_d); \
		GPSR(btweb_features.pbx_cssa_d)=GPIO_bit(btweb_features.pbx_cssa_d);\
		GPSR(btweb_features.pbx_cssb_d)=GPIO_bit(btweb_features.pbx_cssb_d);\
	}


#define CSOFF CSON(0xFF);

static int ctrlssp_active=0;
static int voicedev=-1;
	

static void ctrlssp_set_gpio(void)
{
	set_GPIO_mode(GPIO23_SCLK_md); /* lowercase: buglet in pxa-regs.h */
/*	set_GPIO_mode(GPIO24_SFRM_MD); */
	set_GPIO_mode(GPIO25_STXD_MD);
	set_GPIO_mode(GPIO26_SRXD_MD);
}



static int
ioctl_ssp( struct inode *inode, struct file *file,
                unsigned int cmd, unsigned long arg)
{
        unsigned char val=0;
        unsigned char ssdr,tmp;

	voicedev = MINOR(inode->i_rdev);

        switch (cmd) {
                case PXA_SSP_WRITE: /* write */
			if (copy_from_user(&val, (unsigned char*)arg, sizeof(val))){
	                        deb("copy_from_user error \n");
				return -EFAULT;
			}
			CSON(voicedev);
			udelay(1);
			deb("ioctl_ssp:wr %x to CSON=%d\n",val,voicedev);
			SSDR = 0xff&val;
			udelay(7);
			CSOFF;
			udelay(7);

			/* trailing read */
			tmp=SSDR;

                break;
                case PXA_SSP_READ: /* read */
			CSON(voicedev);
			udelay(1);
			SSDR=0xff;
			udelay(7);
			ssdr=0x00ff&SSDR;
			deb("ioctl_ssp:rd %x from CSON=%d\n",ssdr,voicedev);
			udelay(7);
			CSOFF;
			udelay(7);
			if(copy_to_user((void *)arg, &ssdr, sizeof(ssdr))) {
				deb("ioctl_ssp: no copytouser\n");
				return -EFAULT;
			}
		break;
	}
	return 0;
}




static int open_ssp(struct inode *inode, struct file *filp)
{
	int minor = MINOR(inode->i_rdev);
        deb(KERN_INFO "CTRLSSP opening: minor=%d\n",minor);
	voicedev=minor;

	if(ctrlssp_active>10){
		printk(KERN_INFO "NO\n");
		return -EBUSY;
	}
	if ((minor<0)||(minor>9)){
		deb(KERN_INFO "NO1\n");
		return -EINVAL;
	}
	
	ctrlssp_active++;
	deb(KERN_INFO "YES\n");

	
	
	return 0;
}

static int release_ssp(struct inode *inode, struct file *filp)
{
	ctrlssp_active--;
	return 0;
}

static struct file_operations ctrlssp_fops = {
        owner:    THIS_MODULE,
/*        read:     read_ssp,
	write:	  write_ssp,
*/
	ioctl:	  ioctl_ssp,
        open:     open_ssp,
        release:  release_ssp, 
};

#if 0 
static struct miscdevice ctrlssp_tpanel = {
	0, "voice0", &ctrlssp_fops
};
#endif

int __init ctrlssp_init(void)
{
	unsigned int sscr;

	ctrlssp_set_gpio();

        GPSR(btweb_features.pbx_rst_d)=GPIO_bit(btweb_features.pbx_rst_d);
        mdelay(100);
	/* out of reset state TODO*/
	GPCR(btweb_features.pbx_rst_d)=GPIO_bit(btweb_features.pbx_rst_d);
        mdelay(100);

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
