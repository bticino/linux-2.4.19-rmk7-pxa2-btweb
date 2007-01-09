/*
 * linux/drivers/video/oledfb.c -- STV8102 OLED frame buffer device
 *
 * Copyright 2006 Develer S.r.l. (http://www.develer.com/)
 * Author: Bernardo Innocenti <bernie@develer.com>
 * Author: Stefano Fedrigo <aleph@develer.com>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/tty.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/fb.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <asm/semaphore.h>
#include <asm/atomic.h>

#include <video/fbcon.h>

/* Define to either 0 or 1 */
#define OLED_DEBUG  1

/* Driver name used in several places */
#define OLED_NAME "oledfb"

/* Fully qualified driver name for users */
#define OLED_FRIENDLY_NAME "STV8102 OLED"

/* Dimensions in pixels */
#define OLED_WIDTH  128
#define OLED_HEIGHT 33

/* Dimensions in millimeters */
#define OLED_WIDTH_MM  55
#define OLED_HEIGHT_MM 14

/* Size of an horizontal line in bytes */
#define OLED_WIDTH_BYTES  ((OLED_WIDTH + 7) / 8)

/* Framebuffer size in bytes */
#define OLED_MEMSIZE (OLED_WIDTH_BYTES * OLED_HEIGHT)

#define OLED_MEMORDER (get_order(PAGE_ALIGN(OLED_MEMSIZE)))

/* OLED refresh delay in milliseconds */
#define OLED_REFRESH_DELAY 300
#define OLED_REFRESH_JIFFIES ((OLED_REFRESH_DELAY * HZ)/1000)

/* Set to 1 to enable an X11-like backfilling pattern */
#define OLED_BACKFILL_PATTERN 0

/* I2C address of the OLED command/data registers */
#define I2C_DRIVERID_STV8102_CMD  0x3C
#define I2C_DRIVERID_STV8102_DATA 0x3D

/* Use a kernel thread to refresh the OLED periodically */
#define CONFIG_OLED_REFRESH_THREAD 1

/* BROKEN: i2c code sleeps in timer context */
#define CONFIG_OLED_REFRESH_TIMER 0


#define OLED_CMD_XSTART   0x00 /* address in lower 4 bits */
#define OLED_CMD_YSTART   0x40 /* address in lower 4 bits */
#define OLED_CMD_DISPON   0xAF
#define OLED_CMD_DISPOFF  0xAE
#define OLED_CMD_MOVE     0x80 /* effects in lower 4 bits */
#define OLED_CMD_HSPEED   0x90 /* speed in lower 3 bits */
#define OLED_CMD_VSPEED   0x98 /* speed in lower 3 bits */
#define OLED_CMD_HMIN     0xC0
#define OLED_CMD_HMAX     0xC2
#define OLED_CMD_VMIN     0xC6
#define OLED_CMD_VMAX     0xC8

/* Missing utility macros */
#define countof(x) (sizeof(x) / sizeof(x[0]))
#ifndef bool
#	define bool  int
#	define false 0
#	define true  1
#endif

#if OLED_DEBUG == 1
	#define OLED_TRACE printk(KERN_DEBUG "%s:%s()\n", OLED_NAME, __FUNCTION__)
#elif OLED_DEBUG == 0
	#define OLED_TRACE do {} while (0)
#else
	#error Define OLED_DEBUG to either 0 or 1
#endif


struct oledfb_info {
	struct fb_info_gen gen;

	/* Shadow buffer for the memory mapped framebuffer */
	uint8_t *shadow;

	/* Second copy of shadow buffer for optimized refesh */
	uint8_t *shadow2;

	/* Physical address of shadow buffer as required by fbmem */
	unsigned long shadow_phys;

	/* I2C client we talk to for OLED command register read/write */
	struct i2c_client i2c_cmd;

	/* I2C client we talk to for OLED data register write */
	struct i2c_client i2c_data;

	bool screensaver_running;

	atomic_t open_cnt;

	#if CONFIG_OLED_REFRESH_THREAD
		int thread_pid;

		/* Used to tell our refresh thread to quit asap */
		/*bool*/ int quitting;

		struct semaphore thread_sem;
	#endif

	#if CONFIG_OLED_REFRESH_TIMER
		/* Timer for pushing shadow buffer through I2C bus */
		struct timer_list refresh_timer;
	#endif
};

struct oledfb_par {
	/*
	 *  The hardware specific data in this structure uniquely defines a video
	 *  mode.
	 *
	 *  If your hardware supports only one video mode, you can leave it empty.
	 */
	char dummy;
};

/*
 *  If your driver supports multiple boards, you should make these arrays,
 *  or allocate them dynamically (using kmalloc()).
 */
static struct oledfb_info fb_info;
static struct oledfb_par current_par;
static int current_par_valid = 0;
static struct display disp;

static struct i2c_driver oled_i2c_driver; /* fwd decl */

/* ------------------- OLED specific functions -------------------------- */

inline void
oledhw_write_cmd(struct oledfb_info *info, uint8_t val)
{
	i2c_smbus_write_byte(&info->i2c_cmd, val);
}

inline void
oledhw_write_data(struct oledfb_info *info, uint8_t val)
{
	i2c_smbus_write_byte(&info->i2c_data, val);
}

static void
oledhw_gotoxy(struct oledfb_info *info, int x, int y)
{
	oledhw_write_cmd(info, (uint8_t)(OLED_CMD_XSTART | (x & 0x0F)));
	oledhw_write_cmd(info, (uint8_t)(OLED_CMD_YSTART | (y & 0x0F)));
}

/* Send initialization sequence to display */
static void
oledhw_init(struct oledfb_info *info)
{
	OLED_TRACE;
	static const uint8_t init_sequence[] = {
		0xce, 0xff, 0x2a, 0xc2, 0x00, 0xc0, 0x00, 0xc4, 0x00,
		0x90, 0xa0, 0xa2, 0x80, 0x13,
		0xd6, 0xd8, 0xb2, 0xd0, 0x00, 0x26, 0x2c, 0xb8, 0xcc,
		0x1f, 0x28, 0x38, 0x2e, 0xba, 0x18, 0xc8, 0xc6, 0xa6, 0x08, 0xca, 0x00, 0x98, 0x00,
		0xaf
	};

	int i;
	for (i = 0; i < countof(init_sequence); ++i)
		oledhw_write_cmd(info, init_sequence[i]);

	/* Clear all display RAM, including invisible parts */
	oledhw_gotoxy(info, 0, 0);
	for (i = 0; i < OLED_WIDTH_BYTES * 64; ++i)
		oledhw_write_data(info, 0);
}

/* Send initialization sequence to display */
static void
oledhw_cleanup(struct oledfb_info *info)
{
	oledhw_write_cmd(info, OLED_CMD_DISPOFF);
}

/* FIXME */
#include "logo_bticino.c"

static void
oledhw_bounce(struct oledfb_info *info)
{
	//uint8_t *bitmap = (uint8_t *)info->shadow_phys;
	uint8_t *bitmap = info->shadow;
	unsigned int y;
	for (y = 0; y < LOGO_HEIGHT; ++y) {
		unsigned int x;
		for (x = 0; x < LOGO_WIDTH / 8; ++x) {
			bitmap[y * OLED_WIDTH_BYTES + x] = logo_bticino[y * (LOGO_WIDTH / 8) + x];
		}
	}
}

/* Fill the display with a background pattern */
static void
oledhw_backfill(struct oledfb_info *info, int frame)
{
#if OLED_BACKFILL_PATTERN
	/* X11-like background pattern */
	static const uint8_t pattern[4] = {0x88, 0x22, 0x44, 0x11};

	uint8_t *bitmap = info->shadow;
	unsigned int y;
	for (y = 0; y < OLED_HEIGHT; ++y) {
		unsigned int x;
		unsigned int index = (y + frame) % countof(pattern);
		for (x = 0; x < OLED_WIDTH_BYTES; ++x) {
			bitmap[y * OLED_WIDTH_BYTES + x] = pattern[index];
		}
	}
#else
	//uint8_t *bitmap = (uint8_t *)info->shadow_phys;
	uint8_t *bitmap = info->shadow;
	memset(bitmap, 0, OLED_HEIGHT * OLED_WIDTH_BYTES);
#endif
}

static void
oledhw_screensaver_start(struct oledfb_info *info)
{
	oledhw_write_cmd(info, OLED_CMD_HSPEED | 1);
	oledhw_write_cmd(info, OLED_CMD_VSPEED | 1);
	oledhw_write_cmd(info, OLED_CMD_MOVE   | 0x04 | 0x03); /* horiz. bounce only, vert. bounce and wrap */
	oledhw_write_cmd(info, OLED_CMD_VMIN); oledhw_write_cmd(info, 0x62);
	oledhw_write_cmd(info, OLED_CMD_VMAX); oledhw_write_cmd(info, 0x1F);
	oledhw_write_cmd(info, OLED_CMD_HMIN); oledhw_write_cmd(info, 0xC0);
	oledhw_write_cmd(info, OLED_CMD_HMAX); oledhw_write_cmd(info, 0x40);
}

static void
oledhw_screensaver_stop(struct oledfb_info *info)
{
	oledhw_write_cmd(info, OLED_CMD_HSPEED | 0);
	oledhw_write_cmd(info, OLED_CMD_VSPEED | 0);
}

/* Copy the shadow buffer to the physical display */
static void
oledhw_refresh(struct oledfb_info *info)
{
	//OLED_TRACE; (too verbose)

	oledhw_gotoxy(info, 0, 0);

	//printk("shadow=%p, shadow_phys=%lx\n", info->shadow, info->shadow_phys);

	//uint8_t *bitmap = (uint8_t *)info->shadow_phys;
	uint8_t *bitmap = info->shadow;

	int i;
	for (i = 0; i < OLED_MEMSIZE; ++i) {
		//info->shadow[i] = i;
		//TODO: optimize to a block transfer
		oledhw_write_data(info, bitmap[i]);
	}
}

#if CONFIG_OLED_REFRESH_THREAD

/* Process to handle background OLED refresh from the shadow buffer. */
static int
oledhw_thread(void *arg)
{
	OLED_TRACE;
	struct oledfb_info *info = (struct oledfb_info *)arg;

	/* Do some setup to look like a real daemon */
	daemonize();
	exit_files(current);
	sprintf(current->comm, OLED_NAME);

	spin_lock_irq(&current->sigmask_lock);
	sigfillset(&current->blocked);
	flush_signals(current);
	spin_unlock_irq(&current->sigmask_lock);

	current->policy = SCHED_OTHER;
	current->nice = 5;
	current->flags |= PF_NOIO;

	int i = 0;
	while (!info->quitting) {
		if (atomic_read(&info->open_cnt) == 0) {
			/* draw waking checkboard pattern when idle */
			static int frame = 0;
			oledhw_backfill(info, frame++);
			oledhw_bounce(info);
			if (!info->screensaver_running) {
				oledhw_screensaver_start(info);
				info->screensaver_running = true;
			}
		} else if (info->screensaver_running) {
			oledhw_screensaver_stop(info);
			info->screensaver_running = false;
		}

		/* Optimize refresh: transfer image only when it has changed */
		if (memcmp(info->shadow, info->shadow2, OLED_MEMSIZE)) {
			memcpy(info->shadow2, info->shadow, OLED_MEMSIZE);
			oledhw_refresh(info);
		}

		//msleep(OLED_REFRESH_DELAY);
		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(OLED_REFRESH_JIFFIES);
	}

	/* Tell module we're done */
	//printk(KERN_ERR "thread quitting\n");
	up(&info->thread_sem);
	//printk(KERN_ERR "thread had quit\n");
	return 0;
}

#endif /* CONFIG_OLED_REFRESH_THREAD */

#if CONFIG_OLED_REFRESH_TIMER
#include <linux/timer.h>

/* Callback invoked by timer to perform refresh periodically */
static void
oledhw_timer(unsigned long arg)
{
	OLED_TRACE;
	struct oledfb_info *info = (struct oledfb_info *)arg;

	oledhw_refresh(info);

	/* Retrigger timer */
	info->refresh_timer.expires +=  OLED_REFRESH_DELAY_JIFFIES;
	add_timer(&info->refresh_timer);
}
#endif /* CONFIG_OLED_REFRESH_TIMER */


/* ------------------- Chipset specific functions -------------------------- */

static int
oled_encode_fix(struct fb_fix_screeninfo *fix, const void *par,
		struct fb_info_gen *info)
{
	OLED_TRACE;
	/*
	 *  This function should fill in the 'fix' structure based on the values
	 *  in the `par' structure.
	 */
	strncpy(fix->id, OLED_NAME, sizeof(fix->id));
	fix->smem_start  = (unsigned long)fb_info.shadow_phys;
	printk("phys=%lx p2v=%lx, v=%lx\n", fb_info.shadow_phys, phys_to_virt(fb_info.shadow_phys), fb_info.shadow);
	fix->smem_len    = OLED_MEMSIZE;
	fix->type        = FB_TYPE_PLANES;
	fix->type_aux    = 0;
	fix->visual      = FB_VISUAL_MONO01;
	fix->xpanstep    = 0;
	fix->ypanstep    = 0;
	fix->ywrapstep   = 0;
	fix->line_length = OLED_WIDTH_BYTES;
	fix->mmio_start  = 0;
	fix->mmio_len    = 0;
	fix->accel       = FB_ACCEL_NONE;

	return 0;
}

static int
oled_decode_var(const struct fb_var_screeninfo *var, void *par,
		struct fb_info_gen *info)
{
	OLED_TRACE;
	return -EINVAL;
}

static int
oled_encode_var(struct fb_var_screeninfo *var, const void *par,
		struct fb_info_gen *info)
{
	OLED_TRACE;
	/*
	 *  Fill the 'var' structure based on the values in 'par' and maybe other
	 *  values read out of the hardware.
	 */
	memset(var, 0, sizeof(*var));
	var->xres         = OLED_WIDTH;
	var->yres         = OLED_HEIGHT;
	var->xres_virtual = OLED_WIDTH;
	var->yres_virtual = OLED_HEIGHT;
	var->bits_per_pixel = 1;
	var->grayscale    = 1;
	var->width        = OLED_WIDTH_MM;
	var->height       = OLED_HEIGHT_MM;

	return 0;
}

static void
oled_get_par(void *_par, struct fb_info_gen *info)
{
	struct oledfb_par *par = (struct oledfb_par *)_par;
	OLED_TRACE;

	/*
	 *  Fill the hardware's 'par' structure.
	 */

	if (current_par_valid)
		*par = current_par;
	else {
		/* ... */
	}
}

static void
oled_set_par(const void *_par, struct fb_info_gen *info)
{
	struct oledfb_par *par = (struct oledfb_par *)_par;
	OLED_TRACE;

	/*
	 *  Set the hardware according to 'par'.
	 */
	current_par = *par;
	current_par_valid = 1;
	/* ... */
}

static int
oled_getcolreg(unsigned regno, unsigned *red, unsigned *green,
		  unsigned *blue, unsigned *transp, struct fb_info *info)
{
	OLED_TRACE;
	/*
	 *  Read a single color register and split it into colors/transparent.
	 *  The return values must have a 16 bit magnitude.
	 *  Return != 0 for invalid regno.
	 */
	if (regno == 0)
		*red = *green = *blue = *transp = 0;
	else if (regno == 1)
		*red = *green = *blue = 0xFFFF, *transp = 0;
	else
		return -EINVAL;

	return 0;
}

static int
oled_setcolreg(unsigned regno, unsigned red, unsigned green,
		unsigned blue, unsigned transp, struct fb_info *info)
{
	OLED_TRACE;
	return -EINVAL;
}

static int
oled_pan_display(const struct fb_var_screeninfo *var,
		struct fb_info_gen *info)
{
	OLED_TRACE;
	/*
	 *  Pan (or wrap, depending on the `vmode' field) the display using the
	 *  `xoffset' and `yoffset' fields of the `var' structure.
	 *  If the values don't fit, return -EINVAL.
	 */

	/* ... */
	return 0;
}

static int
oled_blank(int blank_mode, struct fb_info_gen *info)
{
	OLED_TRACE;
	/*
	 *  Blank the screen if blank_mode != 0, else unblank. If blank == NULL
	 *  then the caller blanks by setting the CLUT (Color Look Up Table) to all
	 *  black. Return 0 if blanking succeeded, != 0 if un-/blanking failed due
	 *  to e.g. a video mode which doesn't support it. Implements VESA suspend
	 *  and powerdown modes on hardware that supports disabling hsync/vsync:
	 *    blank_mode == 2: suspend vsync
	 *    blank_mode == 3: suspend hsync
	 *    blank_mode == 4: powerdown
	 */

	/* ... */
	return 0;
}

static void
oled_set_disp(const void *_par, struct display *disp,
		 struct fb_info_gen *_info)
{
	struct oledfb_info *info = (struct oledfb_info *)_info;
	OLED_TRACE;

	/*  Fill in a pointer with the virtual address of the mapped frame buffer. */
	disp->screen_base = info->shadow;

	/*
	 *  Fill in a pointer to appropriate low level text console operations (and
	 *  optionally a pointer to help data) for the video mode `par' of your
	 *  video hardware. These can be generic software routines, or hardware
	 *  accelerated routines specifically tailored for your hardware.
	 *  If you don't have any appropriate operations, you must fill in a
	 *  pointer to dummy operations, and there will be no text output.
	 */
	disp->dispsw = &fbcon_dummy;
}

static void
oled_detect(void)
{
	OLED_TRACE;
}

/* ------------ Interfaces to hardware functions ------------ */

struct fbgen_hwswitch oled_switch = {
	.detect        = oled_detect,
	.encode_fix    = oled_encode_fix,
	.decode_var    = oled_decode_var,
	.encode_var    = oled_encode_var,
	.get_par       = oled_get_par,
	.set_par       = oled_set_par,
	.getcolreg     = oled_getcolreg,
	.setcolreg     = oled_setcolreg,
    .pan_display   = oled_pan_display,
	.blank         = oled_blank,
	.set_disp      = oled_set_disp,
};

/* ------------- Frame buffer operations --------------*/

/* If all you need is that - just don't define ->fb_open */
static int
oledfb_open(struct fb_info *_info, int user)
{
	OLED_TRACE;

	struct oledfb_info *info = (struct oledfb_info *)_info;
	atomic_inc(&info->open_cnt);

	return 0;
}

/* If all you need is that - just don't define ->fb_release */
static int
oledfb_release(struct fb_info *_info, int user)
{
	OLED_TRACE;

	struct oledfb_info *info = (struct oledfb_info *)_info;
	atomic_dec(&info->open_cnt);

	return 0;
}

/*
 *  In most cases the `generic' routines (fbgen_*) should be satisfactory.
 *  However, you're free to fill in your own replacements.
 */

static struct fb_ops oledfb_ops = {
	.owner = THIS_MODULE,
	.fb_open = oledfb_open,	/* only if you need it to do something */
	.fb_release = oledfb_release,
	/* only if you need it to do something */
	.fb_get_fix = fbgen_get_fix,
	.fb_get_var = fbgen_get_var,
	.fb_set_var = fbgen_set_var,
	.fb_get_cmap = fbgen_get_cmap,
	.fb_set_cmap = fbgen_set_cmap,
	.fb_pan_display = fbgen_pan_display,
	/* .fb_ioctl = oledfb_ioctl,*/ /* optional */
};

/* ------------- I2C Driver interface --------------*/

static int oled_attach(struct i2c_adapter *adap, int addr,
		unsigned short flags, int kind)
{
	int result;

	printk(KERN_ERR "oled attach(): addr: %x\n", addr);

	/* Initialize i2c client for the OLED command register */
	strcpy(fb_info.i2c_cmd.name, oled_i2c_driver.name);
	fb_info.i2c_cmd.id = oled_i2c_driver.id;
	fb_info.i2c_cmd.flags = 0;
	fb_info.i2c_cmd.addr = addr;
	fb_info.i2c_cmd.adapter = adap;
	fb_info.i2c_cmd.driver = &oled_i2c_driver;
	fb_info.i2c_cmd.data = NULL;
	if ((result = i2c_attach_client(&fb_info.i2c_cmd)) < 0)
		return result;

	/* Initialize i2c client for the OLED data register */
	memcpy(&fb_info.i2c_data, &fb_info.i2c_cmd, sizeof(fb_info.i2c_data));
	fb_info.i2c_data.addr = addr + 1;
	if ((result = i2c_attach_client(&fb_info.i2c_data)) < 0)
	{
		i2c_detach_client(&fb_info.i2c_cmd);
		return result;
	}

	oledhw_init(&fb_info);
	return 0;
}

static int oled_probe(struct i2c_adapter *adap)
{
	static unsigned short ignore[] = { I2C_CLIENT_END };
//	static unsigned short normal_addr[] = { I2C_DRIVERID_STV8102_CMD, I2C_DRIVERID_STV8102_DATA, I2C_CLIENT_END };
	static unsigned short force_addr[] = { -1, I2C_DRIVERID_STV8102_CMD, I2C_CLIENT_END };
	static struct i2c_client_address_data addr_data = {
//		.normal_i2c =       normal_addr,
		.normal_i2c =       ignore,
		.normal_i2c_range = ignore,
		.probe =            ignore,
		.probe_range =      ignore,
		.ignore =           ignore,
		.ignore_range =     ignore,
//		.force =            ignore,
		.force =            force_addr,
	};

	OLED_TRACE;
	return i2c_probe(adap, &addr_data, oled_attach);
}

static int oled_detach(struct i2c_client *client)
{
	OLED_TRACE;
	oledhw_cleanup(&fb_info);
	i2c_detach_client(client);
	return 0;
}

static struct i2c_driver oled_i2c_driver =
{
	.name =            OLED_NAME,
	.id =              I2C_DRIVERID_STV8102_CMD,
	.flags =           I2C_DF_NOTIFY,
	.attach_adapter =  oled_probe,
	.detach_client =   oled_detach,
};


/* ------------ Frame Buffer interface ------------ */

static int
oledfb_update_var(int unit, struct fb_info *info)
{
	OLED_TRACE;
	return 0;
}

static void
oledfb_blank(int unit, struct fb_info *info)
{
	OLED_TRACE;
}

/* ------------ Module interface ------------ */

int __init
oledfb_init(void)
{
	int result = 0;
	OLED_TRACE;

	if ((result = i2c_add_driver(&oled_i2c_driver)) < 0) {
		printk(KERN_ERR OLED_NAME ": failed to register i2c driver\n");
		return result;
	}

	fb_info.gen.parsize = sizeof(current_par);
	fb_info.gen.fbhw = &oled_switch;
	fb_info.gen.fbhw->detect();
	strcpy(fb_info.gen.info.modename, OLED_FRIENDLY_NAME);
	fb_info.gen.info.node = -1;
	fb_info.gen.info.fbops = &oledfb_ops;
	fb_info.gen.info.disp = &disp;
	fb_info.gen.info.changevar = NULL;
	fb_info.gen.info.switch_con = NULL; /* optional */
	fb_info.gen.info.updatevar = &oledfb_update_var;
	fb_info.gen.info.blank = &oledfb_blank;
	fb_info.gen.info.flags = FBINFO_FLAG_DEFAULT;

	/*
	 * Prepare a shadow buffer for userland to mmap().
	 *
	 * We need to allocate physical memory because the fbmem interface
	 * wants it so.  We also need to map the buffer to a virtual address
	 * to read from it later.
	 */
	fb_info.shadow = NULL;
	fb_info.shadow2 = NULL;
	if (!(fb_info.shadow = (void *)__get_free_pages(GFP_KERNEL, OLED_MEMORDER))
		|| (!(fb_info.shadow2 = (void *)__get_free_pages(GFP_KERNEL, OLED_MEMORDER)))) {

/*FIXME	if (!(fb_info.shadow_phys = __get_free_page(GFP_KERNEL))
		|| !(fb_info.shadow = ioremap(fb_info.shadow_phys, OLED_MEMSIZE))) { */
/*FIXME	if (!(fb_info.shadow_phys = vmalloc(OLED_MEMSIZE))) { */
		printk(KERN_ERR OLED_NAME ": can't allocate shadow buffer");
		result = -1;
		goto out;
	}
	fb_info.shadow_phys = __virt_to_phys((unsigned long)fb_info.shadow);

	/* This should give a reasonable default video mode */
	fbgen_get_var(&disp.var, -1, &fb_info.gen.info);
	fbgen_do_set_var(&disp.var, 1, &fb_info.gen);
	fbgen_set_disp(-1, &fb_info.gen);
	fbgen_install_cmap(0, &fb_info.gen);

	if ((result = register_framebuffer(&fb_info.gen.info)) < 0) {
		printk(KERN_ERR OLED_NAME ": can't register framebuffer\n");
		goto out2;
	}

	#if CONFIG_OLED_REFRESH_THREAD
		fb_info.quitting = 0 /* false */;
		init_MUTEX_LOCKED(&fb_info.thread_sem);
		if ((result = kernel_thread(oledhw_thread, &fb_info, CLONE_FS | CLONE_FILES | CLONE_SIGHAND)) < 0) {
			printk(KERN_ERR OLED_NAME ": can't create refresh thread\n");
			goto out3;
		}
	#elif CONFIG_OLED_REFRESH_TIMER
		init_timer(&fb_info.refresh_timer);
		fb_info.refresh_timer.data     = (unsigned long)&fb_info;
		fb_info.refresh_timer.function = oledhw_timer;
		fb_info.refresh_timer.expires  = jiffies + OLED_REFRESH_DELAY_JIFFIES;
		add_timer(&fb_info.refresh_timer);
	#endif

	printk(KERN_INFO "fb%d: %s frame buffer device\n",
			GET_FB_IDX(fb_info.gen.info.node), fb_info.gen.info.modename);

	return 0;

out3:
	unregister_framebuffer((struct fb_info *)&fb_info);
out2:
	i2c_del_driver(&oled_i2c_driver);
out:
	free_pages((unsigned long)fb_info.shadow, OLED_MEMORDER);
	free_pages((unsigned long)fb_info.shadow2, OLED_MEMORDER);
//FIXME	iounmap(fb_info.shadow);
//FIXME	free_page(fb_info.shadow_phys);
//FIXME	vfree((void *)fb_info.shadow_phys);
	return result;
}

void
oledfb_cleanup(struct fb_info *_info)
{
	struct oledfb_info *info = (struct oledfb_info *)_info;

	OLED_TRACE;

	#if CONFIG_OLED_REFRESH_THREAD
		//printk("killing thread\n");
		info->quitting = 1 /* true */;
		down(&info->thread_sem);
		//printk("thread killed\n");
	#endif
	#if CONFIG_OLED_REFRESH_TIMER
		//del_timer_sync(info->refresh_timer):
		del_timer(&info->refresh_timer);
	#endif
	unregister_framebuffer((struct fb_info *)info);
	i2c_del_driver(&oled_i2c_driver);

	free_pages((unsigned long)fb_info.shadow, OLED_MEMORDER);
	free_pages((unsigned long)fb_info.shadow2, OLED_MEMORDER);
//FIXME	iounmap(info->shadow);
//FIXME	free_page(info->shadow_phys);
//FIXME	vfree((void *)fb_info.shadow_phys);
}

int __init
oledfb_setup(char *options)
{
	OLED_TRACE;
	/* Parse user speficied options (`video=xxxfb:') */

	return 0;
}

/* ------------------------------------------------------------------------- */

/*
 *  Modularization
 */
#ifdef MODULE
MODULE_LICENSE("GPL");
int
init_module(void)
{
	OLED_TRACE;
	return oledfb_init();
}

void
cleanup_module(void)
{
	OLED_TRACE;
	oledfb_cleanup((struct fb_info *)&fb_info);
}
#endif /* MODULE */
