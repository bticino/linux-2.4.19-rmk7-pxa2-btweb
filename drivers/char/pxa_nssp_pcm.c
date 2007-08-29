
/*
 *  PCM audio driver SSP ctrl driver  
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

#include <asm/mach-types.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <asm/hardware.h>
#ifdef CONFIG_ARCH_PXA
#include <asm/arch/pxa-regs.h>
#endif

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

static ssize_t read_ssp(struct file *filp, char *buffer,
                       size_t count, loff_t *ppos)
{
	unsigned char ssdr;
	int len=1;

	if(count != 1)
		return -EINVAL;

//	GPCR(0)=0x800|0x1000000;
//	GPCR(32)=0x8|0x200|0x400000;
	
//	CSON(filp->private_data);
	CSON(voicedev);
	printk("read_ssp: CSON=%d\n",voicedev);

	mdelay(1);
	SSDR=0xff;
	printk("read_ssp:reading\n");
	ssdr=0x00ff&SSDR;	
        printk("read_ssp:read %x\n",ssdr);

	mdelay(2);
	CSOFF;
//	GPSR(0)=0x800|0x1000000;
//	GPSR(32)=0x8|0x200|0x400000;
	mdelay(1);

        if(copy_to_user(buffer, &ssdr, len)) {
                return -EFAULT;
        }

	return 1;
}


static ssize_t write_ssp(struct file *filp, const char *buffer,size_t count, loff_t *ppos)
{
	unsigned char val=0;
	int len=1;

        if(count != 1)
                return -EINVAL;

	if (copy_from_user(&val, buffer, len)) {
                return -EFAULT;
        }

        printk("write_ssp:writing %x\n",val);

        
//	GPCR(0)=0x800|0x1000000;
//      GPCR(32)=0x8|0x200|0x400000;
	CSON(voicedev);//filp->private_data);
	mdelay(1);

        SSDR = val;

	mdelay(1);
//	GPSR(0)=0x800|0x1000000;
//	GPSR(32)=0x8|0x200|0x400000;
	CSOFF;
	mdelay(1);

//	GPCR(0)=0x800|0x1000000;
//	GPCR(32)=0x8|0x200|0x400000;
//	CSON(voicedev);//filp->private_data);
//	mdelay(1);
	val=SSDR;
//	mdelay(1);
	printk("write_ssp:trailing read %x\n",val);

//	GPSR(0)=0x800|0x1000000;
//	GPSR(32)=0x8|0x200|0x400000;
//	CSOFF;
//	mdelay(1);


	return 1;
}


static int open_ssp(struct inode *inode, struct file *filp)
{
	int minor = MINOR(inode->i_rdev);
        printk(KERN_INFO "CTRLSSP opening: minor=%d\n",minor);
	voicedev=minor;

	if(ctrlssp_active){
		printk(KERN_INFO "NO\n");
		return -EBUSY;
	}
	if ((minor<0)||(minor>9)){
		printk(KERN_INFO "NO1\n");
		return -EINVAL;
	}
	
	ctrlssp_active++;
	printk(KERN_INFO "YES\n");
	
	return 0;
}

static int release_ssp(struct inode *inode, struct file *filp)
{
	ctrlssp_active--;
	return 0;
}

static struct file_operations ctrlssp_fops = {
        owner:    THIS_MODULE,
        read:     read_ssp,
	write:	  write_ssp,
        open:     open_ssp,
        release:  release_ssp, 
};

static struct miscdevice ctrlssp_tpanel = {
	0, "voice0", &ctrlssp_fops
};

int __init ctrlssp_init(void)
{
	unsigned int sscr;

	ctrlssp_set_gpio();


        GPSR(btweb_features.pbx_rst_d)=GPIO_bit(btweb_features.pbx_rst_d);
        mdelay(1000);
	/* out of reset state TODO*/
	GPCR(btweb_features.pbx_rst_d)=GPIO_bit(btweb_features.pbx_rst_d);
        mdelay(1000);
        /* out of reset state TODO*/
        GPSR(btweb_features.pbx_rst_d)=GPIO_bit(btweb_features.pbx_rst_d);
        mdelay(1000);
        GPCR(btweb_features.pbx_rst_d)=GPIO_bit(btweb_features.pbx_rst_d);
        mdelay(1000);



	SSSR = 0x0080;
	SSCR0 = 0x1107;  /* 0x11yy=100000 Hz */
	SSCR1 = 0x0018;  /* 0x8=sclk idle is hi */
	CKEN |= CKEN3_SSP;
	sscr = SSCR0;
	sscr |= (1<<7);
	SSCR0 = sscr;

        if (register_chrdev(100, "voice", &ctrlssp_fops))
        {
                printk(KERN_NOTICE "Can't allocate major number %d for Legerity Codecs.\n",
                       100);
                return -EAGAIN;
        }

	printk(KERN_INFO "CTRLSSP ssp Driver\n");
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
