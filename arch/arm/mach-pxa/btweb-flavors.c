/*
 *  linux/arch/arm/mach-pxa/btweb-flavors.c
 *
 * Management of different hardware flavors for the BTicino devices
 *
 * The boot loader should have left the GPIO as generic as possible, so we fix
 */

/***************************************************
            GPIO58 GPIO6 GPIO7 GPIO82 GPIO84   RAM  SPEED
F453          0     0     0     0      0        64   400  // 0x0
F543AV        0     0     0     0      1        64   400  // 0x1
346890        0     0     0     1      0        64   400  // 0x2
PBX-2/8/8/exp 0     0     0     1      1        64   400  // 0x3
PBX-2/8/8     0     0     1     0      0        64   400  // 0x3
PE            0     0     1     0      1        64   400  // 0x3
PI            0     0     1     1      0        64   400  // 0x3
H4684/IP      0     0     1     1      1        64   400  // 0x3
H4684/IP/8    0     1     0     0      0        64   400  // 0x3
CDP/HW        0     1     0     0      1        64   400  // 0x3
INTERFMM      0     1     0     1      0        64   400  // 0x3
MH500         0     1     0     1      1        64   400  // 0x3
MEGATICKER    0     1     1     0      0        64   400  // 0x3

OLD
F452/MH200   0     0     1     x      x      64     // 0x4-0x7
H4684        0     1     0     0      0      64   200  // 0x8
PE-monob     0     1     0     0      1      32   200  // 0x9
F453AV-proto 0     1     1     1      1      64   400  // 0xf


Moreover: GPIO63, GPIO64, GPIO69 (hw revisions)
***************************************************/

#include <linux/config.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/delay.h>

#include <asm/hardware.h>
#include <asm/irq.h>
#include <asm/errno.h>
#include <asm/setup.h>
#include <asm/mach/map.h>
#include <asm/mach/arch.h>
#include <asm/mach/irq.h>

#include <asm/arch/irq.h>

static struct btweb_features feat;
#if 0
static struct btweb_features feat_f452;
static struct btweb_features feat_h4684;
static struct btweb_features feat_h4684ip;
static struct btweb_features feat_f453av;
static struct btweb_features feat_pe;
static struct btweb_features feat_pi;
#endif

/* Low-level output routine, to spit a grand failure before printk is there */
static void __init serialout(int c)
{
        while ((FFLSR & LSR_TEMT) == 0)
                ;
        FFTHR = c;
}

struct btweb_flavor {
	int min_gpio;
	int max_gpio;
        int id;
        char *name;
        int memsize;
        int speed;
	struct btweb_features *features;
        int (*init)(struct btweb_flavor *fla, int rev);
};

static int init_common(void); /* initialization for all devices */
/* device oriented inizializations */
static int init_f453av_346890(struct btweb_flavor *fla, int rev); 
static int init_interfmm(struct btweb_flavor *fla, int rev);
static int init_h4684ip(struct btweb_flavor *fla, int rev); 
static int init_h4684ip_8(struct btweb_flavor *fla, int rev);
static int init_pe(struct btweb_flavor *fla, int rev); 
static int init_pi(struct btweb_flavor *fla, int rev); 
static int init_megaticker(struct btweb_flavor *fla, int rev);



struct btweb_gpio {
	int id;
	u32 gpdr[3];
	u32 gpsr[3];
	u32 gpcr[3];
	u32 gafr[6];
};

/* Generic settings for the various implementations */
static struct btweb_flavor fltab[] __initdata = {
	{0x0,0x0, BTWEB_F453,      "F453",      64, 400, NULL,  NULL},
	{0x1,0x1, BTWEB_F453AV,    "F453AV",    64, 400, &feat, &init_f453av_346890},
	{0x2,0x2, BTWEB_2F,	   "2F",        64, 400, &feat, &init_f453av_346890},
	{0x3,0x3, BTWEB_PBX288exp, "PBX288EXP", 64, 400, NULL,  NULL},
	{0x4,0x4, BTWEB_PBX288,    "PBX288",    64, 400, NULL,  NULL},
	{0x5,0x5, BTWEB_PE,    	   "PE",        64, 400, &feat, &init_pe},
	{0x6,0x6, BTWEB_PI,        "PI",        64, 400, &feat, &init_pi},
	{0x7,0x7, BTWEB_H4684_IP,  "H4684_IP",  64, 400, &feat, &init_h4684ip},
        {0x8,0x8, BTWEB_H4684_IP_8,"H4684_IP_8",64, 400, &feat, &init_h4684ip_8},
        {0x9,0x9, BTWEB_CDP_HW,    "CDP_HW",    64, 400, NULL,  NULL},
        {0xa,0xa, BTWEB_INTERFMM,  "INTERFMM",  64, 400, &feat, &init_interfmm},
        {0xb,0xb, BTWEB_MH500,     "MH500",     64, 400, NULL,  NULL},
        {0xc,0xc, BTWEB_MEGATICKER,"MEGATICKER",64, 400, &feat, &init_megaticker},
#if 0  /* Old Rubini */
        {0x4,0x7, BTWEB_F452,   "F452",   64, 400, &feat_f452,   NULL},
        {0x8,0x8, BTWEB_H4684,  "H4684",  64, 400, &feat_h4684,  NULL},
        {0x9,0x9, BTWEB_PE_M,   "PE-M",   32, 400, NULL,         NULL},
#endif
	/* Following is "master A" for the F453AV, used in prototypes */
        {0xf,0xf, BTWEB_F453AVA,"F453AVA",64, 400, &feat,  &init_f453av_346890},
	{-1,}
};

/* GPIO settings for the various implementations -- FIXME */
struct btweb_gpio gpios[] __initdata = {
	{
		.id = BTWEB_F453AV,
                .gpdr = { 0xDF93F62B ,0x78FFBBEB ,0x000BFFDE},
                .gpsr = { 0x0800840A, 0x00008982, 0x0001C000},
                .gpcr = { 0x07937221, 0x78403068, 0x000A3FDE},
                .gafr = { 0x80400000,
                          0x55000010,
                          0x60908019,
                          0x00058AAA,
                          0xA0000000,
                          0x00000002
		}
	},
        {
                .id = BTWEB_2F,
                .gpdr = { 0xDF93F62B ,0x78FFBBEB ,0x000BFFDE},
                .gpsr = { 0x0800840A, 0x00008982, 0x0001C000},
                .gpcr = { 0x07937221, 0x78403068, 0x000A3FDE},
                .gafr = { 0x80400000,
                          0x55000010,
                          0x60908019,
                          0x00058AAA,
                          0xA0000000,
                          0x00000002

                }
        },
        {
                .id = BTWEB_INTERFMM,
                .gpdr = { 0xDF93F62F ,0x78FFBBEB ,0x000BFFDE},
                .gpsr = { 0x0800840A, 0x00008982, 0x0001C000},
                .gpcr = { 0x07937225, 0x78403068, 0x000A3FDE},
                .gafr = { 0x80400000,
                          0x55000010,
                          0x60908019,
                          0x00058AAA,
                          0xA0000000,
                          0x00000002

                }
        },
        {
                .id = BTWEB_H4684_IP, 
                .gpdr = { 0xF81FD63F,0x03FFFB9B,0x000BC000},
                .gpsr = { 0x08008408,0x00000882,0x0001C000},
                .gpcr = { 0x201C5237,0x03FFF318,0x000A0000},
                .gafr = { 0x80000000,
                          0x511A800A,
                          0x00908019,
                          0x00000000,
                          0xA0000000,
                          0x00000002
                }
        },
        {
                .id = BTWEB_H4684_IP_8, 
                .gpdr = { 0xF81FD63F,0x03FFFB9B,0x000BC000},
                .gpsr = { 0x08008408,0x00000882,0x0001C000},
                .gpcr = { 0x201C5237,0x03FFF318,0x000A0000},
                .gafr = { 0x80000000,
                          0x511A800A,
                          0x00908019,
                          0x00000000,
                          0xA0000000,
                          0x00000002
                }
        },
        {
                .id = BTWEB_MEGATICKER, /* to be fix - now are like BTWEB_H4684_IP */
                .gpdr = { 0xF81FD63F,0x03FFFB9B,0x000BC000},
                .gpsr = { 0x08008408,0x00000882,0x0001C000},
                .gpcr = { 0x201C5237,0x03FFF318,0x000A0000},
                .gafr = { 0x80000000,
                          0x511A800A,
                          0x00908019,
                          0x00000000,
                          0xA0000000,
                          0x00000002
                }
        },
        {
                .id = BTWEB_PE,
                .gpdr = { 0xCB93973F ,0x03FFBBCA ,0x0001C000},
                .gpsr = { 0x0A038C29, 0x00FEA882, 0x0001C000},
                .gpcr = { 0xC1901316, 0x03FD1348, 0x00000000},
                .gafr = { 0x80400000,
                          0xA51A801A,
                          0x60908019,
                          0x00000008,
                          0xA0000000,
                          0x00000002
                }
        },
        {
                .id = BTWEB_PI,
                .gpdr = { 0xCB93BF1F ,0xFF7FBB9A,0x001BFFFF},
                .gpsr = { 0x0A038409, 0x0002A882, 0x0001C000},
                .gpcr = { 0xC1903B16, 0xFF7D1318, 0x001A3FFF},
                .gafr = { 0x80000000,
                          0xA51A801A,
                          0x60908019,
                          0x00000008,
                          0xA0000000,
                          0x00000002
                }
        },
	/* Last table is BTWEB_ANY, a catch-all default */
	{
		.id = BTWEB_ANY, /* This is currently the same as 453AV */
		.gpdr = { 0xDF93FE2F, 0xFC40BBEA, 0x000BFFFF},
		.gpsr = { 0x00000402, 0x00000000, 0x00000000},
		.gpcr = { 0x0F937A2D, 0xFC403368, 0x000A3FFF},
		.gafr = { 0x80000000,
			  0xA5000010,
			  0x00908019,
			  0x00058AAA,
			  0xA0000000,
			  0x00000002
		},
	}
};

/* return a 4-bit number from 4 GPIO configured as input */
static int __init get_4_gpio(int a, int b, int c, int d)
{
	/* turn them all to inputs (better safe than sorry) */
	GPDR(a) &= ~GPIO_bit(a);
	GPDR(b) &= ~GPIO_bit(b);
	GPDR(c) &= ~GPIO_bit(c);
	GPDR(d) &= ~GPIO_bit(d);
	
	return
		!!(GPLR(a) & GPIO_bit(a)) << 3 |
		!!(GPLR(b) & GPIO_bit(b)) << 2 |
		!!(GPLR(c) & GPIO_bit(c)) << 1 |
		!!(GPLR(d) & GPIO_bit(d)) << 0 ;
}

int __init btweb_find_device(int *memsize)
{
	struct btweb_flavor *fptr;
	struct btweb_gpio *gptr;
	int i, id,ret;

	/* retrieve the ID and hw version (even if not always meaningful */
	id = get_4_gpio(6, 7, 82, 84);
        printk(KERN_ALERT "btweb_find_device: read id=%d from 4 gpios\n",id);

	btweb_globals.hw_version = get_4_gpio(58, 63, 64, 69);
        printk(KERN_ALERT "btweb_find_device: read hw_version=%d from 4 gpios\n",btweb_globals.hw_version);

	for (fptr = fltab; fptr->min_gpio >= 0; fptr++)
		if (id >= fptr->min_gpio && id <= fptr->max_gpio)
			break;

	if (fptr->min_gpio < 0) return -ENODEV;

	if (!fptr->features) {
		char s[80];
		/* This won't reach the console with printk, unfortunately */
		sprintf(s, "btweb_find_device: hw ID is 0x%x (unknown), "
			"please fix btweb-flavors.c\n", id);
		for (i=0; s[i]; i++)
			serialout(s[i]);
		return -ENODEV;
	}

	/* set up globals */
	strcpy(btweb_globals.name, fptr->name);
	btweb_globals.flavor = fptr->id;
	btweb_globals.last_touch = jiffies;
	*memsize = fptr->memsize << 20;

	printk(KERN_ALERT "btweb_find_device: Detected \"%s\" device (hw version %i)\n",
	       btweb_globals.name, btweb_globals.hw_version);

	/* setup GPIO */
	for (gptr = gpios; gptr->id != id && gptr->id != BTWEB_ANY; gptr++)
		/* nothing */;

	for (i = 0; i < 3; i++) {
#if 0
		GPSR(i*32)    = gptr->gpsr[i];     /* set */
                GPCR(i*32)    = gptr->gpcr[i];     /* clear */
                GPDR(i*32)    = gptr->gpdr[i];     /* direction */
		GAFR(i*32)    = gptr->gafr[i*2];   /* gafr_l */
		GAFR(i*32+16) = gptr->gafr[i*2+1]; /* gafr_h */
#endif
                GPCR(i*32)    = gptr->gpcr[i];     /* clear */
		GPSR(i*32)    = gptr->gpsr[i];     /* set */
		GAFR(i*32)    = gptr->gafr[i*2];   /* gafr_l */
		GAFR(i*32+16) = gptr->gafr[i*2+1]; /* gafr_h */
                GPDR(i*32)    = gptr->gpdr[i];     /* direction */

	}
	/* Change speed if needed: the boot loader sets up 200MHz */
	if (fptr->speed != 200 && fptr->speed != 400) {
		printk("Unsupported speed %i, keeping 200MHz\n", fptr->speed);
	}
	if (fptr->speed == 400) {
		int cpuspeed = 0x161;
		printk("Switching to 400MHz\n");
		/* Frequency change sequence (3.4.7 and 3.7.1) */
		CCCR = cpuspeed;
		__asm__("mcr     p14, 0, %0, c6, c0, 0" :
			/* no output */ : "r" (2) : "memory");
	}	

	/* set up features */
	btweb_features = *fptr->features;

	if (fptr->init)
		ret = fptr->init(fptr, btweb_globals.hw_version);

        /* common features */
        init_common();

        printk(KERN_ALERT "btweb_find_device: end customization of \"%s\" device (hw version %i)\n",btweb_globals.name, btweb_globals.hw_version);

	return ret;
}



/* the data */

static struct btweb_features feat __initdata = {
	/* system */
	.tvia_reset = -1,
        .eth_reset = -1,
        .eth_irq = -1,
        .e2_wp = -1,
        .rtc_irq = -1,
        .led = -1,
        .usb_irq = -1,
        .pic_reset = -1,
        .buzzer = -1,
        .mdcnfg = -1,
        .virt_conf = -1,
        .cf_irq = -1,
        .usb_soft_enum_n = -1,
        .usb_pxa_slave_connected = -1,

	/* video */
        .abil_mod_video = -1,
        .abil_dem_video = -1,
        .ctrl_video = -1,
	.abil_tlk_i2c_addr = -1,
	.abil_tlk_reg = -1,
	.lighting_level_i2c_addr = -1,
	.lighting_level_reg = -1,

	/* display */
	.backlight = -1,
	.bright_i2c_addr = -1,
	.bright_port = -1,
	.contr_i2c_addr = -1,
	.contr_port = -1,
	.color_i2c_addr = -1,
	.color_port = -1,

	/* audio */
        .ctrl_hifi = -1,
        .abil_mod_hifi = -1,
        .abil_dem_hifi = -1,
        .abil_fon = -1,
	.amp_vol_i2c_addr = -1,
	.amp_vol_port = -1,
	.abil_amp = -1,
	.amp_left = -1,
	.amp_right = -1,
	.abil_fon_ip = -1,
	.mute_speaker = -1,
	.abil_mic = -1,
	.mic_vol_i2c_addr = -1,
        .mic_vol_port = -1,
        .speaker_vol_i2c_addr = -1,
        .speaker_vol_port = -1,

	/* status led */
        .led_serr = -1,
        .led_escl = -1,
        .led_mute = -1,
        .led_conn = -1,
        .led_alarm = -1,
	.led_tlc = -1,

	/* power management */
        .low_voltage = -1,
        .power_sense = -1,
	
	/* tlc */	
	.fc_tilt = -1,
	.fc_pan = -1,
	.fc2_tilt = -1,
	.fc2_pan = -1,
	.abil_motor_ctrl = -1,
	.MIN1A = -1,
	.MIN1B = -1,
	.MIN2A = -1,
	.MIN2B = -1,
	.MIN3A = -1,
	.MIN3B = -1,
	.MIN4A = -1, 
	.MIN4B = -1,

	.rx_tx_485 = -1,
};

/* finally the init procedures */

/* Inizialitation common to all devices */
static int init_common(){
	/* This enables the BTUART for Pic Multimedia*/
	CKEN |= CKEN7_BTUART;
	set_GPIO_mode(GPIO42_BTRXD_MD);
	set_GPIO_mode(GPIO43_BTTXD_MD);

	if (btweb_features.pic_reset!=-1) {
		/* Exiting after a while to be sure that pic has been resetted properly */
		udelay(1000);
		GPSR(btweb_features.pic_reset) = GPIO_bit(btweb_features.pic_reset);
	}

	return 0;
}


static int init_f453av_346890(struct btweb_flavor *fla, int rev) {

	printk("Customizing %s, revision is %d\n",fla->name,rev);

        btweb_features.tvia_reset = 45;
        btweb_features.eth_reset = 3;
        btweb_features.eth_irq = 22;
        btweb_features.e2_wp = 10;
        btweb_features.rtc_irq = 8;
        btweb_features.led = 40;
        btweb_features.usb_irq = 21;
        btweb_features.pic_reset = 44;
        btweb_features.mdcnfg = 0x19C9;
        btweb_features.ctrl_video = 1;
        btweb_features.virt_conf = 2;
        btweb_features.abil_mod_video = 35;
        btweb_features.abil_dem_video = 16;
        btweb_features.abil_mod_hifi = 41;
        btweb_features.abil_dem_hifi = 12;
        btweb_features.abil_fon = 54;
        btweb_features.cf_irq = 11;
        btweb_features.usb_soft_enum_n = 27;
        btweb_features.usb_pxa_slave_connected = 0;


        /* This enables the STUART for Pic Antintrusione*/
        CKEN |= CKEN5_STUART;
        set_GPIO_mode(GPIO46_STRXD_MD);
        set_GPIO_mode(GPIO47_STTXD_MD);

	return 0;

}
static int init_interfmm(struct btweb_flavor *fla, int rev) {

	printk("Customizing %s, revision is %d\n",fla->name,rev);

	/* Maybe i2c should be set to 400Khz */

        btweb_features.tvia_reset = 45;
        btweb_features.eth_reset = 3;
        btweb_features.eth_irq = 22;
        btweb_features.e2_wp = 10;
        btweb_features.rtc_irq = 8;
        btweb_features.led = 40;
        btweb_features.usb_irq = 21;
        btweb_features.pic_reset = 44;
        btweb_features.mdcnfg = 0x19C9;
        btweb_features.ctrl_video = 1;
        btweb_features.abil_mic = 2;
        btweb_features.abil_mod_video = 35;
        btweb_features.abil_dem_video = 16;
        btweb_features.abil_mod_hifi = 41;
        btweb_features.abil_dem_hifi = 12;
        btweb_features.abil_fon = 54;
        btweb_features.cf_irq = 11;
        btweb_features.usb_soft_enum_n = 27;
        btweb_features.usb_pxa_slave_connected = 0;

	btweb_features.lighting_level_i2c_addr = 0x39;
        btweb_features.lighting_level_reg = 0x0d;

        /* This enables the STUART for Pic Antintrusione*/
        CKEN |= CKEN5_STUART;
        set_GPIO_mode(GPIO46_STRXD_MD);
        set_GPIO_mode(GPIO47_STTXD_MD);

	return 0;

}
static int init_h4684ip(struct btweb_flavor *fla, int rev) {

        printk("Customizing %s, revision is %d\n",fla->name,rev);
        
	btweb_features.eth_reset = 3;
        btweb_features.eth_irq = 22;
        btweb_features.e2_wp = 10;
        btweb_features.rtc_irq = 8;
        btweb_features.backlight = 12;
        btweb_features.pic_reset = 44;
        btweb_features.buzzer = 1;
        btweb_features.mdcnfg = 0x19C9;
        btweb_features.usb_soft_enum_n = 27;
        btweb_features.usb_pxa_slave_connected = 0;

	btweb_features.bright_i2c_addr = 0x28;
        btweb_features.bright_port = 0xaa;
        btweb_features.contr_i2c_addr = 0x28;
        btweb_features.contr_port = 0xa9;
	btweb_features.abil_amp = 5;

	/* Enabling buzzer clock */
	CKEN |= CKEN0_PWM0;

        return 0;
}
static int init_h4684ip_8(struct btweb_flavor *fla, int rev) {
        printk("Customizing %s, revision is %d\n",fla->name,rev);

        btweb_features.eth_reset = 3;
        btweb_features.eth_irq = 22;
        btweb_features.e2_wp = 10;
        btweb_features.rtc_irq = 8;
        btweb_features.backlight = 12;
        btweb_features.pic_reset = 44;
        btweb_features.buzzer = 1;
        btweb_features.mdcnfg = 0x19C9;
        btweb_features.usb_soft_enum_n = 27;
        btweb_features.usb_pxa_slave_connected = 0;

        btweb_features.bright_i2c_addr = 0x28;
        btweb_features.bright_port = 0xaa;
        btweb_features.amp_vol_i2c_addr = 0x28;
        btweb_features.amp_vol_port = 0xa9;
        btweb_features.abil_amp = 5;

        /* Enabling buzzer clock */
        CKEN |= CKEN0_PWM0;

        return 0;
}
static int init_megaticker(struct btweb_flavor *fla, int rev) {
        printk("Customizing %s, revision is %d\n",fla->name,rev);

        btweb_features.eth_reset = 3;
        btweb_features.eth_irq = 22;
        btweb_features.e2_wp = 10;
        btweb_features.rtc_irq = 8;
        btweb_features.backlight = 12;
        btweb_features.buzzer = 1;
        btweb_features.mdcnfg = 0x19C9;

        btweb_features.bright_i2c_addr = 0x28;
        btweb_features.bright_port = 0xa9;
        btweb_features.contr_i2c_addr = 0x28;
        btweb_features.contr_port = 0xaa;

	btweb_features.rx_tx_485 = 9;
        /* Enabling buzzer clock */
        CKEN |= CKEN0_PWM0;

        return 0;
}

static int init_pe(struct btweb_flavor *fla, int rev) {
        printk("Customizing %s, revision is %d\n",fla->name,rev);

        btweb_features.tvia_reset = 45;
        btweb_features.eth_reset = 3;
        btweb_features.eth_irq = 22;
        btweb_features.e2_wp = 10;
        btweb_features.rtc_irq = 8;
        btweb_features.backlight = 12;
        btweb_features.pic_reset = 44;
        btweb_features.buzzer = 1;
        btweb_features.mdcnfg = 0x19C9;
        btweb_features.ctrl_video = 1;
        btweb_features.abil_mod_video = 35;
        btweb_features.abil_fon = 41;
        btweb_features.usb_soft_enum_n = 27;
        btweb_features.usb_pxa_slave_connected = 0;

	btweb_features.mic_vol_i2c_addr = 0x29; /* verify */
        btweb_features.mic_vol_port =  0xaa;/* verify */
        btweb_features.speaker_vol_i2c_addr = 0x29; /* verify */ 
        btweb_features.speaker_vol_port = 0xa9; /* verify */

        btweb_features.bright_i2c_addr = 0x28;
        btweb_features.bright_port = 0xaa;
        btweb_features.contr_i2c_addr = 0x28;
        btweb_features.contr_port = 0xa9;

        btweb_features.abil_tlk_i2c_addr = 0x30;
        btweb_features.abil_tlk_reg = 0x3c;
        btweb_features.lighting_level_i2c_addr = 0x30;
        btweb_features.lighting_level_reg = 0x00;

	btweb_features.abil_mic = 0;
	btweb_features.abil_amp = 9;
	btweb_features.fc2_tilt = 11;
	btweb_features.power_sense = 14;
	btweb_features.led_tlc = 20;
	btweb_features.fc2_pan = 36;
	btweb_features.abil_motor_ctrl = 48;
	btweb_features.MIN1A = 50;
        btweb_features.MIN1B = 51;
        btweb_features.MIN2A = 52;
        btweb_features.MIN2B = 53;
        btweb_features.MIN3A = 54;
        btweb_features.MIN3B = 55;
        btweb_features.MIN4A = 56;
        btweb_features.MIN4B = 57;

	btweb_features.fc_tilt = 81;
	btweb_features.fc_pan = 83;
	btweb_features.abil_fon_ip = 84;

	 /* Enabling buzzer clock */
        CKEN |= CKEN0_PWM0;


        return 0;
}

static int init_pi(struct btweb_flavor *fla, int rev) {

	printk("Customizing %s, revision is %d\n",fla->name,rev);

        btweb_features.tvia_reset = 45;/*??*/
	btweb_features.eth_reset = 3;
        btweb_features.eth_irq = 22;
        btweb_features.e2_wp = 10;
        btweb_features.rtc_irq = 8;
        btweb_features.backlight = 36;
        btweb_features.pic_reset = 44;
        btweb_features.buzzer = 1;
        btweb_features.mdcnfg = 0x19C9;
        btweb_features.ctrl_video = 1;/*??*/
        btweb_features.abil_dem_video = 35;
        btweb_features.usb_soft_enum_n = 27;
        btweb_features.usb_pxa_slave_connected = 0;

	/* lcd */
        btweb_features.bright_i2c_addr = 0x28;
        btweb_features.bright_port = 0xa9;
        btweb_features.contr_i2c_addr = 0x29;
        btweb_features.contr_port = 0xaa;
        btweb_features.color_i2c_addr = 0x28,
        btweb_features.color_port = 0xaa,

        /* audio */
        btweb_features.abil_dem_hifi = 20,/*<-*/
        btweb_features.abil_fon = 41,
        btweb_features.amp_vol_i2c_addr = 0x29,
        btweb_features.amp_vol_port = 0xa9,
        btweb_features.abil_amp = 9;
        btweb_features.amp_left = 11;
        btweb_features.amp_right = 13;
        btweb_features.abil_fon_ip = 84;
        btweb_features.mute_speaker = 50;
 	btweb_features.abil_mic = 0;

        /* status led */
        btweb_features.led_serr = 1;
        btweb_features.led_escl = 51;
        btweb_features.led_mute = 52;
        btweb_features.led_conn = 53;
        btweb_features.led_alarm = 54;

        /* power management */
        btweb_features.low_voltage = 55;
        btweb_features.power_sense = 14;

	 /* Enabling buzzer clock */
        CKEN |= CKEN0_PWM0;


        return 0;
}

