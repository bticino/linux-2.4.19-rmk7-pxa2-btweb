/*
 * Alessandro Rubini, 2005. GPL 2 or later.
 * Parts (i2c) by Simon G. Vogl, Frodo Looijaard. GPL 2 or later.
 */
#include <linux/config.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/sysctl.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <asm/hardware.h>

#define trace(format, arg...) printk(KERN_INFO __FILE__ ": " format "\n" , ## arg)

/*
 * The buzzer is always there; the pointer is set in btweb.c::btweb_init.
 * The structure of this code comes from drivers/char/vt.c
 */
static int btsys_buz[2];
static int btsys_benable = 1; /* enabled by default */
static int btsys_upsidedown;
static int btsys_tago;

static void
btweb_kd_nosound(unsigned long ignored)
{
	/* turn the gpio to input mode */
	set_GPIO_mode( 16 | GPIO_IN);
	/* mark that everything it quiet */
	btsys_buz[0] = 0;
}

void
btweb_kd_mksound(unsigned int hz, unsigned int ticks)
{
	static struct timer_list sound_timer = { function: btweb_kd_nosound };
	unsigned int count = 0;


	/* 3.6864 MHz / prescaler; prescaler must be 64 so we can go low */
	PWM_CTRL0 = 63;

	if (hz > 20 && hz < 32767)
		count = ((3686400+32)/64) /hz;
	if (count > 1023) {count = 1023;}
	del_timer(&sound_timer);

	if (!btsys_benable)
		count=0;

	if (count) {
		PWM_PERVAL0 = count;
		PWM_PWDUTY0 = count/2;
		/* set as PWM output */
		set_GPIO_mode( GPIO16_PWM0_MD );
		if (ticks) {
			sound_timer.expires = jiffies+ticks;
			add_timer(&sound_timer);
		}
	} else
		btweb_kd_nosound(0);
}

static int btsys_lcd = 1;
void btweb_backlight(int onoff)
{
	btsys_lcd = onoff;
	if (btsys_lcd)
		GPSR(btweb_features.backlight) =
			GPIO_bit(btweb_features.backlight);
	else
		GPCR(btweb_features.backlight) =
			GPIO_bit(btweb_features.backlight);
}


#ifdef CONFIG_SYSCTL

static int btsys_led = 0;
static int btsys_cntr = 255;
static int btsys_ctrl_hifi = 0;
static int btsys_ctrl_video = 0;
static int btsys_virt_conf = 0;
static int btsys_abil_mod_video = 0;
static int btsys_abil_dem_video = 0;
static int btsys_abil_mod_hifi = 0;
static int btsys_abil_dem_hifi = 0;
static int btsys_abil_fon = 0;

static int bool_min[] = {0};
static int bool_max[] = {1};
static int byte_min[] = {0};
static int byte_max[] = {255};

#ifdef CONFIG_I2C
/* This part copied by i2c-dev.c (Simon G. Vogl, Frodo Looijaard) */

int btsys_detach_client(struct i2c_client *client)
{
        return 0;
}

int btsys_command(struct i2c_client *client, unsigned int cmd, void *arg)
{
        return -1;
}

static struct i2c_client btsys_client;

int btsys_attach_adapter(struct i2c_adapter *adap)
{
	if (btsys_client.adapter) {
		printk("btweb-sysctl: supporting only one I2C adapter\n");
		return -EBUSY;
	}
	btsys_client.adapter = adap;
	return 0;
}

static struct i2c_driver btsys_driver = {
        name:           "btweb-sysctl dummy driver",
        id:             I2C_DRIVERID_I2CDEV, /* Hmm... */
        flags:          I2C_DF_DUMMY,
        attach_adapter: btsys_attach_adapter,
        detach_client:  btsys_detach_client,
        command:        btsys_command,
};

static struct i2c_client btsys_client = {
        name:           "btweb-sysctl entry",
        id:             1,
        flags:          0,
        addr:           -1,
        driver:         &btsys_driver,
};

static int btsys_i2c_do(int addr, int reg, int val)
{
	static int registered;
	char data[2] = {reg, val};

	if (!registered) {
		/*
		 * We would like to add this driver at init time, but we
		 * need a process context as it sleeps on a semaphore.
		 */
		int res;
		if ((res = i2c_add_driver(&btsys_driver))) {
			printk("btsys: can't register i2c driver\n");
			return res;
		}
		registered++;
	}
	btsys_client.addr = addr;
	if (i2c_master_send(&btsys_client, data, 2) == 2)
		return 0;
	return -EIO;
}
#else /* !I2C */
static int btsys_i2c_do(int addr, int reg, int val)
{
	return 0;
} 
#endif /* I2C */

static int btsys_apply(int name)
{
	switch(name) {
		case BTWEB_LED:
			if (btweb_features.led < 0)
				return -EOPNOTSUPP;
			if (!btsys_led)
				GPSR(btweb_features.led) =
					GPIO_bit(btweb_features.led);
			else
                                GPCR(btweb_features.led) =
                                        GPIO_bit(btweb_features.led);
			break;
 		case BTWEB_LCD:
			if (btweb_features.backlight < 0)
				return -EOPNOTSUPP;
			btweb_backlight(btsys_lcd);
			break;
		case BTWEB_CNTR:
                        if (btweb_features.backlight < 0)
                                return -EOPNOTSUPP;
			return btsys_i2c_do(0x28, 0xa9, btsys_cntr);
			break;
		case BTWEB_BUZZER:
            if (!btweb_features.buzzer)
                                return -EOPNOTSUPP;
			btweb_kd_mksound(btsys_buz[0], btsys_buz[1]);
			btsys_buz[1] = 0; /* to be safe next time */
			break;
		case BTWEB_BENABLE:
                        if (!btweb_features.buzzer)
                                return -EOPNOTSUPP;
			if (!btsys_benable)
				btweb_kd_nosound(0);
			break;
		case BTWEB_UPSIDEDOWN:
			if (!btweb_features.backlight)
				return -EOPNOTSUPP;
			if (btsys_upsidedown)
				GPSR(54) = GPIO_bit(54);
			else
				GPCR(54) = GPIO_bit(54);
			break;
		case BTWEB_TAGO:
			break;
		case BTWEB_CTRL_HIFI:
			if (btweb_features.ctrl_hifi < 0)
				return -EOPNOTSUPP;
			if (btsys_ctrl_hifi){
				GPSR(btweb_features.ctrl_hifi) =
					GPIO_bit(btweb_features.ctrl_hifi);
			}
			else{
				GPCR(btweb_features.ctrl_hifi) =
					GPIO_bit(btweb_features.ctrl_hifi);
			}
		break;
		case BTWEB_CTRL_VIDEO:
			if (btweb_features.ctrl_video < 0)
				return -EOPNOTSUPP;
			if (btsys_ctrl_video){
				GPSR(btweb_features.ctrl_video) =
					GPIO_bit(btweb_features.ctrl_video);
			}
			else{
				GPCR(btweb_features.ctrl_video) =
					GPIO_bit(btweb_features.ctrl_video);
			}
		break;
		case BTWEB_VIRT_CONF:
			break;
		break;
		case BTWEB_ABIL_MOD_VIDEO:
			if (btweb_features.abil_mod_video < 0)
				return -EOPNOTSUPP;
			if (btsys_abil_mod_video){
				GPSR(btweb_features.abil_mod_video) =
					GPIO_bit(btweb_features.abil_mod_video);
			}
			else{
				GPCR(btweb_features.abil_mod_video) =
					GPIO_bit(btweb_features.abil_mod_video);
			}
		break;
               case BTWEB_ABIL_DEM_VIDEO:
                        if (btweb_features.abil_dem_video < 0)
                                return -EOPNOTSUPP;
                        if (btsys_abil_dem_video){
                                GPSR(btweb_features.abil_dem_video) =
                                        GPIO_bit(btweb_features.abil_dem_video);
                        }
                        else{
                                GPCR(btweb_features.abil_dem_video) =
                                        GPIO_bit(btweb_features.abil_dem_video);
                        }
                break;
		case BTWEB_ABIL_MOD_HIFI:
			if (btweb_features.abil_mod_hifi < 0)
				return -EOPNOTSUPP;
			if (btsys_abil_mod_hifi){
				GPSR(btweb_features.abil_mod_hifi) =
					GPIO_bit(btweb_features.abil_mod_hifi);
			}
			else{
				GPCR(btweb_features.abil_mod_hifi) =
					GPIO_bit(btweb_features.abil_mod_hifi);
			}
		break;
                case BTWEB_ABIL_DEM_HIFI:
                        if (btweb_features.abil_dem_hifi < 0)
                                return -EOPNOTSUPP;
                        if (btsys_abil_dem_hifi){
                                GPSR(btweb_features.abil_dem_hifi) =
                                        GPIO_bit(btweb_features.abil_dem_hifi);
                        }
                        else{
                                GPCR(btweb_features.abil_dem_hifi) =
                                        GPIO_bit(btweb_features.abil_dem_hifi);
                        }
                break;
		case BTWEB_ABIL_FON:
			if (btweb_features.abil_fon < 0)
				return -EOPNOTSUPP;
			if (btsys_abil_fon){
				GPSR(btweb_features.abil_fon) =
					GPIO_bit(btweb_features.abil_fon);
			}
			else{
				GPCR(btweb_features.abil_fon) =
					GPIO_bit(btweb_features.abil_fon);
			}
		break;
		}
	return 0;
}

/* checker function, uses minmax but takes hold of the values immediately */
int btsys_proc(ctl_table *table, int write, struct file *filp,
	       void *buff, size_t *lenp)
{
	int retval;

	/* first update the touch-ago value */
	btsys_tago = (jiffies - btweb_globals.last_touch) / HZ;

	if (table->extra1){
		retval = proc_dointvec_minmax(table, write, filp, buff, lenp);
	}
	else{
		retval = proc_dointvec(table, write, filp, buff, lenp);
	}
	if (!write || retval < 0) return retval;
	/* written: apply the change */
	return btsys_apply(table->ctl_name);
}

/* the same, for sysctl(2) I/O */
static int btsys_sysctl(ctl_table *table, int *name, int nlen,
			void *oldval, size_t *oldlenp,
			void *newval, size_t newlen, void **context)
{
	int retval;

	/* first update the touch-ago value */
	btsys_tago = (jiffies - btweb_globals.last_touch) / HZ;

	retval = sysctl_intvec(table, name, nlen, oldval, oldlenp,
			       newval, newlen, context);
	if (!newval || retval < 0) return retval;
	trace("Modifying %s",table->procname);
	return btsys_apply(table->ctl_name);
}

ctl_table btsys_table[] = {
	{
		.ctl_name =      BTWEB_LED,
		.procname =      "led",
		.data =          &btsys_led,
		.maxlen =        sizeof(int),
		.mode =          0644,
		.proc_handler =  btsys_proc,
		.strategy =      btsys_sysctl,
		.extra1 =        bool_min,
		.extra2 =        bool_max,
	},
	{
		.ctl_name =      BTWEB_LCD,
		.procname =      "backlight",
		.data =          &btsys_lcd,
		.maxlen =        sizeof(int),
		.mode =          0644,
		.proc_handler =  btsys_proc,
		.strategy =      btsys_sysctl,
		.extra1 =        bool_min,
		.extra2 =        bool_max,
	},
	{
		.ctl_name =      BTWEB_CNTR,
		.procname =      "contrast",
		.data =          &btsys_cntr,
		.maxlen =        sizeof(int),
		.mode =          0644,
		.proc_handler =  btsys_proc,
		.strategy =      btsys_sysctl,
		.extra1 =        byte_min,
		.extra2 =        byte_max,
	},
	{
		.ctl_name =      BTWEB_BUZZER,
		.procname =      "buzzer",
		.data =          btsys_buz,
		.maxlen =        2*sizeof(int),
		.mode =          0644,
		.proc_handler =  btsys_proc,
		.strategy =      btsys_sysctl,
	},
	{
		.ctl_name =      BTWEB_BENABLE,
		.procname =      "buzzer_enable",
		.data =          &btsys_benable,
		.maxlen =        sizeof(int),
		.mode =          0644,
		.proc_handler =  btsys_proc,
		.strategy =      btsys_sysctl,
		.extra1 =        bool_min,
		.extra2 =        bool_max,
	},
	{
		.ctl_name =      BTWEB_UPSIDEDOWN,
		.procname =      "upsidedown",
		.data =          &btsys_upsidedown,
		.maxlen =        sizeof(int),
		.mode =          0644,
		.proc_handler =  btsys_proc,
		.strategy =      btsys_sysctl,
		.extra1 =        bool_min,
		.extra2 =        bool_max,
	},
	{
		.ctl_name =      BTWEB_TAGO,
		.procname =      "touch_ago",
		.data =          &btsys_tago,
		.maxlen =        sizeof(int),
		.mode =          0444,
		.proc_handler =  btsys_proc,
		.strategy =      btsys_sysctl,
	},
	{
		.ctl_name =      BTWEB_RTCINVALID,
		.procname =      "rtc_invalid",
		.data =          &btweb_globals.rtc_invalid,
		.maxlen =        sizeof(int),
		.mode =          0444,
		.proc_handler =  proc_dointvec,
		.strategy =      sysctl_intvec,
	},
	{
		.ctl_name =      BTWEB_HWNAME,
		.procname =      "name",
		.data =          btweb_globals.name,
		.maxlen =        BTWEB_NAMELEN,
		.mode =          0444,
		.proc_handler =  proc_dostring,
		.strategy =      sysctl_string,
	},
	{
		.ctl_name =      BTWEB_HWVERSION,
		.procname =      "hw_version",
		.data =          &btweb_globals.hw_version,
		.maxlen =        sizeof(int),
		.mode =          0444,
		.proc_handler =  proc_dointvec,
		.strategy =      sysctl_intvec,
	},
	{0,}
};

ctl_table btsys_table_F453AV[] = {
	{
		.ctl_name =      BTWEB_LED,
		.procname =      "led",
		.data =          &btsys_led,
		.maxlen =        sizeof(int),
		.mode =          0644,
		.proc_handler =  btsys_proc,
		.strategy =      btsys_sysctl,
		.extra1 =        bool_min,
		.extra2 =        bool_max,
	},
	{
		.ctl_name =      BTWEB_RTCINVALID,
		.procname =      "rtc_invalid",
		.data =          &btweb_globals.rtc_invalid,
		.maxlen =        sizeof(int),
		.mode =          0444,
		.proc_handler =  proc_dointvec,
		.strategy =      sysctl_intvec,
	},
	{
		.ctl_name =      BTWEB_HWNAME,
		.procname =      "name",
		.data =          btweb_globals.name,
		.maxlen =        BTWEB_NAMELEN,
		.mode =          0444,
		.proc_handler =  proc_dostring,
		.strategy =      sysctl_string,
	},
	{
		.ctl_name =      BTWEB_HWVERSION,
		.procname =      "hw_version",
		.data =          &btweb_globals.hw_version,
		.maxlen =        sizeof(int),
		.mode =          0444,
		.proc_handler =  proc_dointvec,
		.strategy =      sysctl_intvec,
	},
	{
		.ctl_name =      BTWEB_CTRL_HIFI,
		.procname =      "ctrl_hifi",
		.data =          &btsys_ctrl_hifi,
		.maxlen =        sizeof(int),
		.mode =          0644,
		.proc_handler =  btsys_proc,
		.strategy =      btsys_sysctl,
		.extra1 =        bool_min,
		.extra2 =        bool_max,
	},
	{
		.ctl_name =      BTWEB_CTRL_VIDEO,
		.procname =      "ctrl_video",
		.data =          &btsys_ctrl_video,
		.maxlen =        sizeof(int),
		.mode =          0644,
		.proc_handler =  btsys_proc,
		.strategy =      btsys_sysctl,
		.extra1 =        bool_min,
		.extra2 =        bool_max,
	},
	{
		.ctl_name =      BTWEB_VIRT_CONF,
		.procname =      "virt_conf",
		.data =          &btsys_virt_conf,
		.maxlen =        sizeof(int),
		.mode =          0444,
		.proc_handler =  proc_dointvec,
		.strategy =      sysctl_intvec,
	},
	{
		.ctl_name =      BTWEB_ABIL_MOD_VIDEO,
		.procname =      "abil_mod_video",
		.data =          &btsys_abil_mod_video,
		.maxlen =        sizeof(int),
		.mode =          0644,
		.proc_handler =  btsys_proc,
		.strategy =      btsys_sysctl,
		.extra1 =        bool_min,
		.extra2 =        bool_max,
	},
        {
                .ctl_name =      BTWEB_ABIL_DEM_VIDEO,
                .procname =      "abil_dem_video",
                .data =          &btsys_abil_dem_video,
                .maxlen =        sizeof(int),
                .mode =          0644,
                .proc_handler =  btsys_proc,
                .strategy =      btsys_sysctl,
                .extra1 =        bool_min,
                .extra2 =        bool_max,
        },
	{
		.ctl_name =      BTWEB_ABIL_MOD_HIFI,
		.procname =      "abil_mod_hifi",
		.data =          &btsys_abil_mod_hifi,
		.maxlen =        sizeof(int),
		.mode =          0644,
		.proc_handler =  btsys_proc,
		.strategy =      btsys_sysctl,
		.extra1 =        bool_min,
		.extra2 =        bool_max,
	},
        {
                .ctl_name =      BTWEB_ABIL_DEM_HIFI,
                .procname =      "abil_dem_hifi",
                .data =          &btsys_abil_dem_hifi,
                .maxlen =        sizeof(int),
                .mode =          0644,
                .proc_handler =  btsys_proc,
                .strategy =      btsys_sysctl,
                .extra1 =        bool_min,
                .extra2 =        bool_max,
        },
	{
		.ctl_name =      BTWEB_ABIL_FON,
		.procname =      "abil_fon",
		.data =          &btsys_abil_fon,
		.maxlen =        sizeof(int),
		.mode =          0644,
		.proc_handler =  btsys_proc,
		.strategy =      btsys_sysctl,
		.extra1 =        bool_min,
		.extra2 =        bool_max,
	},
        {
		.ctl_name =      BTWEB_USB_PXA_SLAVE_CONNECTED,
                .procname =      "usb_pxa_slave_connected",
                .data =          &btweb_globals.usb_pxa_slave_connected,
                .maxlen =        sizeof(int),
                .mode =          0444,
                .proc_handler =  proc_dointvec,
                .strategy =      sysctl_intvec,
	},

	{0,}
};


static ctl_table dev_table[] = {
	{
		.ctl_name =      DEV_BTWEB,
		.procname =      "btweb",
		.mode =          0555,
		.child =         btsys_table,
	},
	{0,}
};

static ctl_table root_table[] = {
	{
		.ctl_name =      CTL_DEV,
		.procname =      "dev",
		.mode =          0555,
		.child =         dev_table,
	},
	{0,}
};
  
static struct ctl_table_header *btsys_header;

int btsys_init(void)
{
	if (btweb_features.led >= 0) {
		set_GPIO_mode(btweb_features.led | GPIO_OUT);
		btsys_apply(BTWEB_LED);
	}
        if (btweb_features.backlight >= 0) {
                set_GPIO_mode(btweb_features.backlight | GPIO_OUT);
		btsys_apply(BTWEB_LCD);
		//btsys_apply(BTWEB_CNTR); -- can't do this with no process
        }

	if ((btweb_globals.flavor==BTWEB_F453AV)||(btweb_globals.flavor==BTWEB_FPGA)) {
		trace("btsys_init: Mapping F453AV or FPGA I/O");
		dev_table[0].child=btsys_table_F453AV;
		trace("btsys_init1: modifying devtable named: %s",dev_table[0].procname);
		
		if (btweb_features.abil_mod_video >= 0) {
			set_GPIO_mode(btweb_features.abil_mod_video | GPIO_OUT);
			btsys_apply(BTWEB_ABIL_MOD_VIDEO);
		}
                if (btweb_features.abil_dem_video >= 0) {
                        set_GPIO_mode(btweb_features.abil_dem_video | GPIO_OUT);
                        btsys_apply(BTWEB_ABIL_DEM_VIDEO);
                }
		if (btweb_features.ctrl_hifi >= 0) {
			set_GPIO_mode(btweb_features.ctrl_hifi | GPIO_OUT);
			btsys_apply(BTWEB_CTRL_HIFI);
		}
		if (btweb_features.ctrl_video >= 0) {
			set_GPIO_mode(btweb_features.ctrl_video | GPIO_OUT);
			btsys_apply(BTWEB_CTRL_VIDEO);
		}
		if (btweb_features.abil_mod_hifi >= 0) {
			set_GPIO_mode(btweb_features.abil_mod_hifi | GPIO_OUT);
			btsys_apply(BTWEB_ABIL_MOD_HIFI);
		}
                if (btweb_features.abil_dem_hifi >= 0) {
                        set_GPIO_mode(btweb_features.abil_dem_hifi | GPIO_OUT);
                        btsys_apply(BTWEB_ABIL_DEM_HIFI);
                }
		if (btweb_features.abil_fon >= 0) {
			set_GPIO_mode(btweb_features.abil_fon | GPIO_OUT);
			btsys_apply(BTWEB_ABIL_FON);
		}
	}

	btsys_header = register_sysctl_table(root_table, 0);


	if (btsys_header) return 0;
		
	
	return -EBUSY;
}

void btsys_exit(void)
{
	unregister_sysctl_table(btsys_header);
}

module_init(btsys_init);
module_exit(btsys_exit);

#endif /* CONFIG_SYSCTL */
