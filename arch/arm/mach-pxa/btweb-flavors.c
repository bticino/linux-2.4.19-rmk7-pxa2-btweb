/*
 *  linux/arch/arm/mach-pxa/btweb-flavors.c
 *
 * Management of different hardware flavors for the BTicino devices
 *
 * The boot loader should have left the GPIO as generic as possible, so we fix
 */

/***************************************************
           GPIO6 GPIO7 GPIO82 GPIO84   RAM  SPEED
F453         0     0     0      0      64   400  // 0x0
F543AV       0     0     0      1      64   400  // 0x1
2fili        0     0     1      0      64   400  // 0x2
PBX-com      0     0     1      1      32   200  // 0x3
F452/MH200   0     1     x      x      64   200  // 0x4-0x7
H4684        1     0     0      0      64   200  // 0x8
PE-monob     1     0     0      1      32   200  // 0x9

F453AV-proto 1     1     1      1      64   400  // 0xf


Moreover: GPIO58, GPIO63, GPIO64, GPIO69 (hw revisions)
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

static struct btweb_features feat_f452;
static struct btweb_features feat_h4684;
static struct btweb_features feat_f453av;


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
static int init_f453av_346890_interfmm(struct btweb_flavor *fla, int rev); /* device oriented customizations */ 

struct btweb_gpio {
	int id;
	u32 gpdr[3];
	u32 gpsr[3];
	u32 gpcr[3];
	u32 gafr[6];
};

/* Generic settings for the various implementations */
static struct btweb_flavor fltab[] __initdata = {
        {0x0,0x0, BTWEB_F453,   "F453",   64, 400, NULL,         NULL},
        {0x1,0x1, BTWEB_F453AV, "F453AV", 64, 400, &feat_f453av, &init_f453av_346890_interfmm},
        {0x2,0x2, BTWEB_2F,     "2F",     64, 400, &feat_f453av, &init_f453av_346890_interfmm},
        {0x3,0x3, BTWEB_PBX,    "PBX",    32, 400, NULL,         NULL},
        {0x4,0x7, BTWEB_F452,   "F452",   64, 400, &feat_f452,   NULL},
        {0x8,0x8, BTWEB_H4684,  "H4684",  64, 400, &feat_h4684,  NULL},
        {0x9,0x9, BTWEB_PE_M,   "PE-M",   32, 400, NULL,         NULL},
	/* Following is "master A" for the F453AV, used in prototypes */
        {0xf,0xf, BTWEB_F453AVA,"F453AVA",64, 400, &feat_f453av, NULL},
	{-1,}
};

/* GPIO settings for the various implementations -- FIXME */
struct btweb_gpio gpios[] __initdata = {
	{
		.id = BTWEB_F453AV,
		/* The following values has been verified with gpio_command_rimaster_B */

		.gpdr = { 0xDF93F62B /* 0xD389BE0B */ ,/* 0x7840BBEA no cf */ 0x78FFBBEB /* orig 0xFCFFBB83 */ , 0x000BFFDE /* 0x000BFFFF */},
		.gpsr = { 0x0800040A, 0x00000000, 0x00000000},
		.gpcr = { 0x07937221, 0x78403168, 0x000A3FDE},
		.gafr = { 0x80400000,
			  0x591A8010, 
			  0x00908019,
			  0xAAA58AAA,
			  0xAAAAAAAA,
			  0x00000046
		}
	},
        {
                .id = BTWEB_2F,
                /* The following values has been verified with gpio_command_rimaster_B */

                .gpdr = { 0xDF93F62B /* 0xD389BE0B */ ,/* 0x7840BBEA no cf */ 0x78FFBBEB /* orig 0xFCFFBB83 */ , 0x000BFFDE /* 0x000BFFFF */},
                .gpsr = { 0x0800040A, 0x00000000, 0x00000000},
                .gpcr = { 0x07937221, 0x78403168, 0x000A3FDE},
                .gafr = { 0x80400000,
                          0x591A8010,
                          0x00908019,
                          0xAAA58AAA,
                          0xAAAAAAAA,
                          0x00000046
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
	int i, id;

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

        printk(KERN_ALERT "btweb_find_device: Selected id=%d for gpio settings\n",gptr->id);

	for (i = 0; i < 3; i++) {
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

	init_common();

	/* set up features */
	btweb_features = *fptr->features;
	if (fptr->init)
		return fptr->init(fptr, btweb_globals.hw_version);
	return 0;
}



/* the data */

static struct btweb_features feat_f452 __initdata = {
	.eth_reset = 63,
	.eth_irq = 23,
	.e2_wp = 70,
	.rtc_irq = 8,
	.led = 71, /* currently unused */
	.usb_irq = -1,
	.backlight = -1,
	.pic_reset = 62,
	.buzzer = 0,
	.mdcnfg = 0x19C9,
	.ctrl_hifi = -1,
	.ctrl_video = -1,
	.virt_conf = -1,
	.abil_mod_video = -1,
        .abil_dem_video = -1,
	.abil_mod_hifi = -1,
        .abil_dem_hifi = -1,
	.abil_fon = -1,
	.cf_irq = -1,
	.usb_soft_enum_n = -1,
	.usb_pxa_slave_connected = -1,

};

static struct btweb_features feat_h4684 __initdata = {
	.eth_reset = 3,
	.eth_irq = 22,
	.e2_wp = 10,
	.rtc_irq = 8,
	.led = -1,
	.usb_irq = 26,
	.backlight = 12,
	.pic_reset = 44,
	.buzzer = 1,
	.mdcnfg = 0x19C9,
	.ctrl_hifi = -1,
	.ctrl_video = -1,
	.virt_conf = -1,
        .abil_mod_video = -1,
        .abil_dem_video = -1,
        .abil_mod_hifi = -1,
        .abil_dem_hifi = -1,
        .abil_fon = -1,
        .cf_irq = -1,
        .usb_soft_enum_n = -1,
        .usb_pxa_slave_connected = -1,
};

static struct btweb_features feat_f453av __initdata  = {
	.eth_reset = 3,
	.eth_irq = 22,
	.e2_wp = 10,
	.rtc_irq = 8,
	.led = 40,
	.usb_irq = 21,
	.backlight = -1,
	.pic_reset = 44,
	.buzzer = -1,
	.mdcnfg = 0x19C9,
	.ctrl_hifi = -1,
	.ctrl_video = 1,
	.virt_conf = 2,
	.abil_mod_video = 35,
	.abil_dem_video = 16,
	.abil_mod_hifi = 41,
	.abil_dem_hifi = 12,
	.abil_fon = 54,
	.cf_irq = 11,
	.usb_soft_enum_n = 27,
	.usb_pxa_slave_connected = 0,
};

/* finally the init procedures */

/* Inizialitation common to all devices */
static int init_common(){
	/* This enables the BTUART for Pic Multimedia*/
	CKEN |= CKEN7_BTUART;
	set_GPIO_mode(GPIO42_BTRXD_MD);
	set_GPIO_mode(GPIO43_BTTXD_MD);

	return 0;
}

static int init_f453av_346890_interfmm(struct btweb_flavor *fla, int rev) {

	printk("Customizing %s, revision is %d\n",fla->name,rev);

        /* This enables the STUART for Pic Antintrusione*/
        CKEN |= CKEN5_STUART;
        set_GPIO_mode(GPIO46_STRXD_MD);
        set_GPIO_mode(GPIO47_STTXD_MD);

	/* To be sure that pic has been resetted properly */
	udelay(1000);
	GPSR(btweb_features.pic_reset) = GPIO_bit(btweb_features.pic_reset);

	return 0;

}


