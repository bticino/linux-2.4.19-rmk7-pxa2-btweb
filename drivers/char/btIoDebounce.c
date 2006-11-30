#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/signal.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/miscdevice.h>
#include <linux/ptrace.h>
#include <linux/poll.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/smp_lock.h>
#include <linux/init.h>
                                                                          
#include <asm/signal.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/semaphore.h>
#include <linux/spinlock.h>
#include <asm/uaccess.h>


#define GPIO_AUX_NUMBER 2
/* #define BTIODEBOUNCE_MINOR from miscdevice */

static void notify_pad_up_down(void);
static int raw_data;
static int button_pending;


struct btIoDebounce {
/*        enum pc110pad_mode mode; */
        int     bounce_interval;
        int     irq;
        int     io;
};


static struct btIoDebounce default_params = {
/*        mode:                   0, */
        bounce_interval:        10,
        io:			GPIO_AUX_NUMBER
};
                                                                                                        
static struct btIoDebounce current_params;
                                                                                                        

/* set just after an up/down transition */
                                                                             
/*
 * Timer goes off a short while after an up/down transition and copies
 * the value of raw_down to debounced_down.
 */
                                                                             
static void bounce_timeout(unsigned long data);
static struct timer_list bounce_timer = { function: bounce_timeout };

/**
 * bounce_timeout:
 * @data: Unused
 *
 * No further up/down transitions happened within the
 * bounce period, so treat this as a genuine transition.
 */
                                                                             
static void bounce_timeout(unsigned long data)
{
	if (!(GPLR(GPIO_AUX_NUMBER) & GPIO_bit(GPIO_AUX_NUMBER))==raw_data) {
		printk("rd\n");
		notify_pad_up_down();
	}
	else
		printk("nord\n");
}




/* driver/filesystem interface management */
static wait_queue_head_t queue;
static struct fasync_struct *asyncptr;
static struct semaphore reader_lock;
                                                                             
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


/**
 * notify_pad_up_down:
 *
 * Called by the raw pad read routines when a (debounced) up/down
 * transition is detected.
 */
                                                                             
static void notify_pad_up_down(void)
{
	button_pending = 1;
	wake_readers();
}


/**
 *      open_Io:
 *      @inode: inode of pad
 *      @file: file handle to pad
 *
 *      Open access to the pad. We turn the pad off first (we turned it off
 *      on close but if this is the first open after a crash the state is
 *      indeterminate). The device has a small fifo so we empty that before
 *      we kick it back into action.
 */
                                                                                                                   
static int open_Io(struct inode * inode, struct file * file)
{
        current_params = default_params;
	del_timer(&bounce_timer);
        button_pending=0;
        GPDR(GPIO_AUX_NUMBER) &= ~GPIO_bit(GPIO_AUX_NUMBER); /* input */

        return 0;
}

/**
 * read_Io:
 * @file: File handle to pad
 * @buffer: Target for the mouse data
 * @count: Buffer length
 * @ppos: Offset (unused)
 *
 * Read data from the pad. We use the reader_lock to avoid mess when there are
 * two readers. This shouldnt be happening anyway but we play safe.
 */
                                                                             
static ssize_t read_Io(struct file * file, char * buffer, size_t count, loff_t *ppos)
{
        down(&reader_lock);
        disable_irq(current_params.irq);

	if(count != 1)
                return -EINVAL;

        if(copy_to_user(buffer, &raw_data, sizeof(int))) {
                return -EFAULT;
        }

        enable_irq(current_params.irq);
        up(&reader_lock);

	button_pending=0;

        return 1;
}



/**
 *      write_Io:
 *      @file: File handle to the pad
 *      @buffer: Unused
 *      @count: Unused
 *      @ppos: Unused
 *
 *      Writes are disallowed. 
 */
                                                                             
static ssize_t write_Io(struct file * file, const char * buffer, size_t count, loff_t *ppos)
{
        return -EINVAL;
}

/**
 * Io_poll:
 * @file: File of the Io device
 * @wait: Poll table
 *
 * The pad is ready to read if there is a button or any position change
 * pending in the queue. The reading and interrupt routines maintain the
 * required state for us and do needed wakeups.
 */
                                                                                                                                                             
static unsigned int Io_poll(struct file *file, poll_table * wait)
{
        poll_wait(file, &queue, wait);
        if (button_pending)
                return POLLIN | POLLRDNORM;
        return 0;
}


/**
 * btIo_irq:
 * @irq: Interrupt number
 * @ptr: Unused
 * @regs: Unused
 *
 * Callback when pad's irq goes off; copies values in to raw_* globals;  * initiates debounce processing. This isn't SMP safe however there are
 * no SMP machines with a PC110 touchpad on them.  */
static void btIo_irq(int irq, void *ptr, struct pt_regs *regs)
{
	int value=0;
        {
		if (!(GPLR(GPIO_AUX_NUMBER) & GPIO_bit(GPIO_AUX_NUMBER)))
			value=1;

                raw_data=value;
        }

	if (raw_data)
	        mod_timer(&bounce_timer,jiffies+current_params.bounce_interval);

}

static struct file_operations btIo_fops = {
        owner:          THIS_MODULE,
        read:           read_Io,
        write:          write_Io,
        poll:           Io_poll,
        open:           open_Io,
};


static struct miscdevice btIoDebounce_pad = {
        minor:          BTIODEBOUNCE_MINOR,
        name:           "btIoDebounce pad",
        fops:           &btIo_fops,
};


/**
 *      btIoDebounce:
 *
 *      We claim the needed I/O and interrupt resources.
 */
                                                                          
static char banner[] __initdata = KERN_INFO "btIoDebounce Input at 0x%X, irq %d.\n";
                                                                          
static int __init btIoDebounce_init_driver(void)
{
        init_MUTEX(&reader_lock);
        current_params = default_params;
                
        printk(KERN_INFO "btIoDebounce: started\n");

        GPDR(current_params.io) &= ~GPIO_bit(current_params.io); /* input FIXME */

        printk(KERN_INFO __FILE__ "GPIO interrupt: falling edge. Gpio %d\n",current_params.io);
        set_GPIO_IRQ_edge(current_params.io, GPIO_FALLING_EDGE);

        /* gpio2 -> irq 25*/
	current_params.irq=GPIO_2_80_TO_IRQ(current_params.io);
        if (request_irq(current_params.irq, btIo_irq, 0, "btIoDebounce", 0)) {
                printk(KERN_ERR "btIoDebounce: Unable to get IRQ.\n");
                return -EBUSY;
        }
#if 0
        if (!request_region(current_params.io, 4, "btIoDebounce"))  {
                printk(KERN_ERR "btIoDebounce: I/O area in use.\n");
                free_irq(current_params.irq,0);
                return -EBUSY;
        }
#endif
        init_waitqueue_head(&queue);
        printk(banner, current_params.io, current_params.irq);
        misc_register(&btIoDebounce_pad);
        printk(KERN_INFO "btIoDebounce: running\n");
	return 0;

}

/*
 *      btIoDebounce_exit_driver:
 *
 *      Free the resources we acquired when the module was loaded. We also
 *      turn the pad off to be sure we don't leave it using power.
 */
                                                                             
static void __exit btIoDebounce_exit_driver(void)
{
	if (current_params.irq) {
		printk(KERN_INFO "btIoDebounce: free_irq %d\n",current_params.irq);
		free_irq(current_params.irq, &btIoDebounce_pad);
	}
//	release_region(current_params.io, 4);
	misc_deregister(&btIoDebounce_pad);
}


module_init(btIoDebounce_init_driver);
module_exit(btIoDebounce_exit_driver);
                                                                             
MODULE_AUTHOR("Raffaele Recalcati");
MODULE_DESCRIPTION("Driver for reading from gpio using interrupt");
MODULE_LICENSE("GPL");
                                                                             
EXPORT_NO_SYMBOLS;

