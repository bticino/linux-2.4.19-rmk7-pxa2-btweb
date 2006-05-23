
/*
 *  ADS7846 touchscreen driver
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

#define PS5MX_TPANEL_MINOR 11

#define PEN_UP_TIMEOUT  (HZ/10)

#define GPIO_REFRESH (5*HZ) /* refresh gpio state every that much time */

#define AD_QUEUE_SIZE	32
#define PEN_IGNORE		0
#define PEN_DOWN		1
#define PEN_UP			2
#define MAX_ERROR		100 /* 20 original */

#define INCBUF(x,mod) (((x)+1) & ((mod) - 1))

struct ts_info {
	unsigned short pressure;
	unsigned short x;
	unsigned short y;
	unsigned short pad;
};

struct ad_queue {
	int head;
	int tail;
	wait_queue_head_t proc_list;
	struct fasync_struct *fasync;
	struct semaphore lock;
	struct ts_info buf[AD_QUEUE_SIZE];
};

static struct ad_queue g_queue;
static struct ad_queue *queue = &g_queue;
static struct timer_list adtimer;
static int ad_active=0, last_x=0, last_y=0;
static int nextstate=PEN_IGNORE, prevstate=PEN_IGNORE;
spinlock_t ads_controller_lock = SPIN_LOCK_UNLOCKED;

static void ads_set_gpio(void)
{
	set_GPIO_mode(GPIO23_SCLK_md); /* lowercase: buglet in pxa-regs.h */
	set_GPIO_mode(GPIO24_SFRM_MD);
	set_GPIO_mode(GPIO25_STXD_MD);
	set_GPIO_mode(GPIO26_SRXD_MD);
}

static unsigned int get_sample(unsigned int *px,unsigned int *py)
{
	unsigned int ssdr, x[5], y[5];
	int i, diff0, diff1, diff2;

	for(i=0; i<5; i++) {
		SSDR = 0x00d3;
		udelay(250);
		ssdr = SSDR;
		x[i] = ssdr & 0x0FFF;
	}
	for(i=0; i<5; i++) {
		SSDR = 0x0093;
		udelay(250);
		ssdr = SSDR;
		y[i] = ssdr & 0x0FFF;
	}

	if((x[2]<4095) && (x[3]<4095) && (x[4]<4095) && (x[2]>0) && (x[3]>0) && (x[4]>0)) {
		diff0 = x[2] - x[3];
		diff1 = x[2] - x[4];
		diff2 = x[4] - x[2];
		diff0 = diff0 > 0  ? diff0 : -diff0;
		diff1 = diff1 > 0  ? diff1 : -diff1;
		diff2 = diff2 > 0  ? diff2 : -diff2;

		if((diff0 > MAX_ERROR) || (diff1 > MAX_ERROR) || (diff2 > MAX_ERROR)) {
			return 1;
		}
		else {
			if(diff0 < diff1) {
				*px= (x[2] + ((diff2 < diff0) ? x[4] : x[3]));
			} else { 
				*px= (x[4] + ((diff2 < diff1) ? x[2] : x[3]));
			}
			*px >>= 1;
		}
	}
	else {
		return 2;
	}

	if((y[2]<4095) && (y[3]<4095) && (y[4]<4095) && (y[2]>0) && (y[3]>0) && (y[4]>0)) {
		diff0 = y[2] - y[3];
		diff1 = y[2] - y[4];
		diff2 = y[4] - y[2];
		diff0 = diff0 > 0  ? diff0 : -diff0;
		diff1 = diff1 > 0  ? diff1 : -diff1;
		diff2 = diff2 > 0  ? diff2 : -diff2;

		if((diff0 > MAX_ERROR) || (diff1 > MAX_ERROR) || (diff2 > MAX_ERROR)) {
			return 1;
		}
		else {
			if(diff0 < diff1) {
				*py = (y[2] + ((diff2 < diff0) ? y[4] : y[3]));
			} else { 
				*py = (y[4] + ((diff2 < diff1) ? y[2] : y[3]));
			}
			*py >>= 1;
		}
	}
	else {
		return 2;
	}
	if((abs(last_x - *px) <= MAX_ERROR) && (abs(last_y - *py) <= MAX_ERROR)) {
		*px = last_x; *py = last_y;
	}
	else {
		last_x = *px; last_y = *py;
	}

	return 0;
}

static void add_adqueue(unsigned short x, unsigned short y, int down)
{
	struct ts_info *info = &queue->buf[queue->head];
	unsigned int nhead = INCBUF(queue->head, AD_QUEUE_SIZE);
	
	if(nhead != queue->tail) {
		info->x = x & 0x0FFF;
		info->y = y & 0x0FFF;
		info->pressure = down;
		info->pad = 0;

		queue->head = nhead;

		if(queue->fasync)
			kill_fasync(&queue->fasync, SIGIO, POLL_IN);

		wake_up_interruptible(&queue->proc_list);
	}
}

static void ad_sendreq(unsigned long ptr)
{
	unsigned long flags;
	unsigned int i=0, x, y;
	static unsigned long uptime;
	static unsigned long last_set_gpio;

	if (!last_set_gpio) last_set_gpio = jiffies; /* just inited */

	spin_lock_irqsave(&ads_controller_lock, flags);
	i = get_sample(&x, &y);
	if(i==0) {
		add_adqueue(x, y, 1);
		nextstate = PEN_UP;
		prevstate = PEN_DOWN;
		i = 0;
		uptime = jiffies + PEN_UP_TIMEOUT;
	}
	else {
		/* here we had an i==2 case, but it didn't always work */
		if (prevstate == PEN_DOWN && time_after_eq(jiffies, uptime)) {
			add_adqueue(0, 0, 0);
			btweb_globals.last_touch = jiffies;
			nextstate = PEN_DOWN;
			prevstate = PEN_UP;
		}
	}
	adtimer.expires = jiffies + HZ/25;
	add_timer(&adtimer);
	spin_unlock_irqrestore(&ads_controller_lock, flags);
	if (time_after(jiffies, last_set_gpio + GPIO_REFRESH)) {
		last_set_gpio = jiffies;
		ads_set_gpio();
	}
}

static ssize_t read_ad(struct file *filp, char *buffer,
                       size_t count, loff_t *ppos)
{
	if(count < sizeof(struct ts_info))
		return -EINVAL;

	if(down_interruptible(&queue->lock))
		return -ERESTARTSYS;
	while(queue->head == queue->tail) {
		up(&queue->lock);
		if(filp->f_flags & O_NONBLOCK)
			return -EAGAIN;
		if(wait_event_interruptible(queue->proc_list, (queue->head != queue->tail)))
			return -ERESTARTSYS;
		if(down_interruptible(&queue->lock))
			return -ERESTARTSYS;
	}
	if(copy_to_user(buffer, &queue->buf[queue->tail], sizeof(struct ts_info))) {
		up(&queue->lock);
		return -EFAULT;
	}

	queue->tail = INCBUF(queue->tail, AD_QUEUE_SIZE);

	up(&queue->lock);
	return sizeof(struct ts_info);
}

static unsigned int poll_ad(struct file *filp, poll_table *wait)
{
	poll_wait(filp, &queue->proc_list, wait);
	return (queue->head == queue->tail ? 0 : (POLLIN | POLLRDNORM));
}

static int open_ad(struct inode *inode, struct file *filp)
{
	if(ad_active)
		return -EBUSY;
	ad_active++;
	init_timer(&adtimer);
	adtimer.function = ad_sendreq;
	adtimer.data = 0;
	adtimer.expires = jiffies + HZ/25;
	add_timer(&adtimer);
	return 0;
}

static int fasync_ad(int fd, struct file *filp, int on)
{
	int retval;

	retval = fasync_helper(fd, filp, on, &queue->fasync);
	if (retval < 0)
		return retval;
	return 0;
}

static int release_ad(struct inode *inode, struct file *filp)
{
	del_timer(&adtimer);
	if(ad_active) {
		ad_active = 0;
		fasync_ad(-1, filp, 0);
	}
	return 0;
}

static struct file_operations ads7846_fops = {
        owner:    THIS_MODULE,
        poll:     poll_ad,
        read:     read_ad,
        open:     open_ad,
        release:  release_ad,
        fasync:   fasync_ad,
};

static struct miscdevice ads7846_tpanel = {
	PS5MX_TPANEL_MINOR, "ads7846", &ads7846_fops
};

int __init ads7846_init(void)
{
	unsigned int sscr;

	ads_set_gpio();

	SSSR = 0x0080;
	SSCR0 = 0x112B;
	SSCR1 = 0x00C0;
	CKEN |= CKEN3_SSP;
	sscr = SSCR0;
	sscr |= (1<<7);
	SSCR0 = sscr;

	queue->head = queue->tail = 0;
	init_waitqueue_head(&queue->proc_list);
	init_MUTEX(&queue->lock);
	queue->fasync = NULL;

	misc_register(&ads7846_tpanel);
	printk(KERN_INFO "ADS7846 Touchscreen Driver\n");
	return 0;
}
 
void __exit ads7846_exit(void)
{
	unsigned int sscr;

	misc_deregister(&ads7846_tpanel);

	sscr = SSCR0;
	sscr &= ~(1<<7);
	SSCR0 = sscr;
	CKEN &= ~(CKEN3_SSP);
}

module_init(ads7846_init);
module_exit(ads7846_exit);

MODULE_AUTHOR("EdwardKuo");
MODULE_DESCRIPTION("ADS7846 Touchscreen Driver");

EXPORT_NO_SYMBOLS;
