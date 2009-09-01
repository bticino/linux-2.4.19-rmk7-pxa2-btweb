/*
 * $Id:
 *
 * Map driver for the BtWeb platform.
 * Adapted from lubbock.c
 *
 * Author:	Nicolas Pitre
 * Copyright:	(C) 2001 MontaVista Software Inc.
 * Copyright:   (C) 2004 Alessandro Rubini
 * Copyright:   (C) 2005,2006,2007,2008 Raffaele Recalcati - BTicino SpA
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <asm/io.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#include <linux/mtd/partitions.h>

#define BTWEB_FLASH_32M  /* */
#define BTWEB_PARTITIONS /* For all BTWEB products*/

#define WINDOW_ADDR 	0
//#define WINDOW_ADDR 	0x04000000
#define WINDOW_SIZE 	64*1024*1024

int bt_mtd=0;

static __u8 btweb_read8(struct map_info *map, unsigned long ofs)
{
	return *(__u8 *)(map->map_priv_1 + ofs);
}

static __u16 btweb_read16(struct map_info *map, unsigned long ofs)
{
	return *(__u16 *)(map->map_priv_1 + ofs);
}

static __u32 btweb_read32(struct map_info *map, unsigned long ofs)
{
	return *(__u32 *)(map->map_priv_1 + ofs);
}

static void btweb_copy_from(struct map_info *map, void *to, unsigned long from, ssize_t len)
{
	memcpy(to, (void *)(map->map_priv_1 + from), len);
}

static void btweb_write8(struct map_info *map, __u8 d, unsigned long adr)
{
	*(__u8 *)(map->map_priv_1 + adr) = d;
}

static void btweb_write16(struct map_info *map, __u16 d, unsigned long adr)
{
	*(__u16 *)(map->map_priv_1 + adr) = d;
}

static void btweb_write32(struct map_info *map, __u32 d, unsigned long adr)
{
	*(__u32 *)(map->map_priv_1 + adr) = d;
}

static void btweb_copy_to(struct map_info *map, unsigned long to, const void *from, ssize_t len)
{
	memcpy((void *)(map->map_priv_1 + to), from, len);
}

static struct map_info btweb_map = {
	name: "BtWeb flash",
	size: WINDOW_SIZE,
	read8:		btweb_read8,
	read16:		btweb_read16,
	read32:		btweb_read32,
	copy_from:	btweb_copy_from,
	write8:		btweb_write8,
	write16:	btweb_write16,
	write32:	btweb_write32,
	copy_to:	btweb_copy_to
};


#ifdef BTWEB_PARTITIONS /* F453AV fix flash partitions */

/* In a redboot point of view
fis create -b 0x880000 -l 0x060000 -f 0x40000 "conf"
fis create -b 0x880000 -l 0x060000 -f 0xa0000 "conf_copy"
fis create -b 0x880000 -l 0x120000 -f 0x100000 "Kernel"
fis create -b 0x880000 -l 0x8A0000 -f 0x220000 "btweb_only"
fis create -b 0x880000 -l 0x280000 -f 0xAC0000 "btweb_appl"
fis create -b 0x880000 -l 0x280000 -f 0xD40000 "btweb_appl_copy"
fis create -b 0x880000 -l 0x020000 -f 0xFC0000 "extra"
*/

static struct mtd_partition btweb_partitions[] = {
	{
		name:		"Bootloader",	/* mtd0 */
		size:		0x00040000,
		offset:		0,
	},{
		name:		"conf",
		size:		0x00060000,	/* mtd1 */    
		offset:		0x00040000,
	},{
		name:		"conf_copy",	/* mtd2 */
		size:		0x00060000,
		offset:		0x000a0000,
	},{
		name:		"Kernel",	/* mtd3 */
		size:		0x00120000,
		offset:		0x00100000,
	},{
		name:		"btweb_only",	/* mtd4 */
		size:		0x008a0000,
		offset:		0x00220000,
	},{
		name:		"btweb_app",	/* mtd5 */
		size:		0x00280000,
		offset:		0x00ac0000
	},{
		name:		"btweb_app_copy",	/* mtd6 */
		size:		0x00280000,
		offset:		0x00d40000
	},{
		name:		"u-boot-env",	/* mtd7 */
		size:		0x00020000,
		offset:		0x00fc0000
	},{
		name:		"u-boot-env-maybe", 	/* mtd8 */
		size:		0x00020000,
		offset:		0x00fe0000
#ifdef BTWEB_FLASH_32M
        },{
                name:           "zImage1",	/* mtd9 */
                size:           0x00120000,
                offset:         0x01000000
        },{
                name:           "btweb_only1",	/* mtd10 */
                size:           0x008a0000,
                offset:         0x01120000,
        },{
                name:           "extra",	/* mtd11 */
                size:           0x00640000,
                offset:         0x019c0000

        }
#else
        },{
                name:           "extra",	/* mtd9 */
                size:           0x00020000,
                offset:         0x00fc0000

        }
#endif

};

static struct mtd_partition btweb_extra_partitions[] = {
	{
		name:		"Bootloader", 		/* mtd0 */
		size:		0x00040000,
		offset:		0,
	},{
		name:		"conf",			/* mtd1 */
		size:		0x00060000,
		offset:		0x00040000,
	},{
		name:		"conf_copy",		/* mtd2 */
		size:		0x00060000,
		offset:		0x000a0000,
	},{
		name:		"Kernel",		/* mtd3 */
		size:		0x00120000,
		offset:		0x00100000,
	},{
		name:		"btweb_only",		/* mtd4 */
		size:		0x008a0000,
		offset:		0x00220000,
	},{
		name:		"btweb_app",		/* mtd5 */
		size:		0x00280000,
		offset:		0x00ac0000
	},{
		name:		"btweb_app_copy",	/* mtd6 */
		size:		0x00280000,
		offset:		0x00d40000
	},{
		name:		"u-boot-env",		/* mtd7 */
		size:		0x00020000,
		offset:		0x00fc0000
	},{
		name:		"u-boot-env-maybe",	/* mtd8 */
		size:		0x00020000,
		offset:		0x00fe0000
#ifdef BTWEB_FLASH_32M
        },{
                name:           "zImage1",		/* mtd9 */
                size:           0x00120000,
                offset:         0x01000000
        },{
                name:           "btweb_only1",		/* mtd10 */
                size:           0x00480000,
                offset:         0x01120000,
        },{
                name:           "extra",		/* mtd11 */
                size:           0x00a60000,
                offset:         0x015a0000

        }
#else
        },{
                name:           "extra",		/* mtd9 */
                size:           0x00020000,
                offset:         0x00fc0000

        }
#endif

};

static struct mtd_partition btweb_extra_partitions_intmm[] = {
	{
		name:		"Bootloader", 		/* mtd0 */
		size:		0x00040000,
		offset:		0,
	},{
		name:		"conf",			/* mtd1 */
		size:		0x00060000,
		offset:		0x00040000,
	},{
		name:		"conf_copy",		/* mtd2 */
		size:		0x00060000,
		offset:		0x000a0000,
	},{
		name:		"Kernel",		/* mtd3 */
		size:		0x00120000,
		offset:		0x00100000,
	},{
		name:		"btweb_only",		/* mtd4 */
		size:		0x00960000,
		offset:		0x00220000,
	},{
		name:		"btweb_app",		/* mtd5 */
		size:		0x00220000,
		offset:		0x00b80000
	},{
		name:		"btweb_app_copy",	/* mtd6 */
		size:		0x00220000,
		offset:		0x00da0000
	},{
		name:		"u-boot-env",		/* mtd7 */
		size:		0x00020000,
		offset:		0x00fc0000
	},{
		name:		"u-boot-env-maybe",	/* mtd8 */
		size:		0x00020000,
		offset:		0x00fe0000
#ifdef BTWEB_FLASH_32M
        },{
                name:           "zImage1",		/* mtd9 */
                size:           0x00120000,
                offset:         0x01000000
        },{
                name:           "btweb_only1",		/* mtd10 */
                size:           0x00480000,
                offset:         0x01120000,
        },{
                name:           "extra",		/* mtd11 */
                size:           0x00a60000,
                offset:         0x015a0000

        }
#else
        },{
                name:           "extra",		/* mtd9 */
                size:           0x00020000,
                offset:         0x00fc0000

        }
#endif

};

static struct mtd_partition btweb_partitions_mh500[] = {
	{
		name:		"Bootloader",	/* mtd0 */
		size:		0x00040000,
		offset:		0,
	},{
		name:		"conf",
		size:		0x00060000,	/* mtd1 */    
		offset:		0x00040000,
	},{
		name:		"conf_copy",	/* mtd2 */
		size:		0x00060000,
		offset:		0x000a0000,
	},{
		name:		"Kernel",	/* mtd3 */
		size:		0x00100000,
		offset:		0x00100000,
	},{
		name:		"btweb_only",	/* mtd4 */
		size:		0x00400000,
		offset:		0x00200000,
	},{
		name:		"btweb_app",	/* mtd5 */
		size:		0x001E0000,
		offset:		0x00600000
	},{
		name:		"btweb_app_copy",	/* mtd6 */
		size:		0x00020000,
		offset:		0x007E0000
	},{
		name:		"u-boot-env",	/* mtd10 */
		size:		0x00020000,
		offset:		0x00fc0000
	},{
		name:		"u-boot-env-maybe", 	/* mtd11 */
		size:		0x00020000,
		offset:		0x00fe0000
        },{
                name:           "zImage1",	/* mtd7 */
                size:           0x00100000,
                offset:         0x00800000
        },{
                name:           "btweb_only1",	/* mtd8 */
                size:           0x00400000,
                offset:         0x00900000,
        },{
                name:           "extra",	/* mtd9 */
                size:           0x002c0000,
                offset:         0x00d00000
        }
};
////

static struct mtd_partition btweb_partitions_f453[] = {
	{
		name:		"Bootloader",	/* mtd0 */
		size:		0x00040000,
		offset:		0,
	},{
		name:		"conf",
		size:		0x00060000,	/* mtd1 */    
		offset:		0x00040000,
	},{
		name:		"conf_copy",	/* mtd2 */
		size:		0x00060000,
		offset:		0x000a0000,
	},{
		name:		"Kernel",	/* mtd3 */
		size:		0x00100000,
		offset:		0x00100000,
	},{
		name:		"btweb_only",	/* mtd4 */
		size:		0x00580000,
		offset:		0x00200000,
	},{
		name:		"btweb_app",	/* mtd5 */
		size:		0x001E0000,
		offset:		0x00780000
	},{
		name:		"btweb_app_copy",	/* mtd6 */
		size:		0x00020000,
		offset:		0x00960000
	},{
		name:		"u-boot-env",	/* mtd7 */
		size:		0x00020000,
		offset:		0x00980000
	},{
		name:		"u-boot-env-maybe", 	/* mtd8 */
		size:		0x00020000,
		offset:		0x009a0000
        },{
                name:           "zImage1",	/* mtd9 */
                size:           0x00100000,
                offset:         0x009c0000
        },{
                name:           "btweb_only1",	/* mtd10 */
                size:           0x00400000,
                offset:         0x00ac0000,
        },{
                name:           "extra",	/* mtd11 */
                size:           0x00100000,
                offset:         0x00ec0000
        }
};
////

// btweb custom partitions for OMIZZY
static struct mtd_partition btweb_partitions_lgr03617[] = {
        {
                name:           "Bootloader",   /* mtd0 */
                size:           0x00040000,
                offset:         0,
        },{
                name:           "conf",
                size:           0x00060000,     /* mtd1 */
                offset:         0x00040000,
        },{
                name:           "conf_copy",    /* mtd2 */
                size:           0x00060000,
                offset:         0x000a0000,
        },{
                name:           "Kernel",       /* mtd3 */
                size:           0x00100000,
                offset:         0x00100000,
        },{
                name:           "btweb_only",   /* mtd4 */
                size:           0x00600000,
                offset:         0x00200000,
        },{
                name:           "btweb_app",    /* mtd5 */
                size:           0x001c0000,
                offset:         0x00800000
        },{
                name:           "btweb_app_copy",       /* mtd6 */
                size:           0x00020000,
                offset:         0x00A20000
        },{
                name:           "u-boot-env",   /* mtd7 */
                size:           0x00020000,
                offset:         0x00fc0000
        },{
                name:           "u-boot-env-maybe",     /* mtd8 */
                size:           0x00020000,
                offset:         0x00fe0000
        },{
                name:           "zImage1",      /* mtd9 */
                size:           0x00100000,
                offset:         0x009e0000
        },{
                name:           "btweb_only1",  /* mtd10 */
                size:           0x00400000,
                offset:         0x00ae0000,
        },{
                name:           "extra",        /* mtd11 */
                size:           0x00140000,
                offset:         0x00ee0000
        }
};
////

static struct mtd_partition btweb_custom_1[] = {
	{
		name:		"Bootloader", 		/* mtd0 */
		size:		0x00040000,
		offset:		0,
	},{
		name:		"conf",			/* mtd1 */
		size:		0x00060000,
		offset:		0x00040000,
	},{
		name:		"conf_copy",		/* mtd2 */
		size:		0x00060000,
		offset:		0x000a0000,
	},{
		name:		"Kernel",		/* mtd3 */
		size:		0x00120000,
		offset:		0x00100000,
	},{
		name:		"pool_1",		/* mtd4 */
		size:		0x00DE0000,
		offset:		0x00220000,
	},{
		name:		"pool_2",		/* mtd5 */
		size:		0x01000000,
		offset:		0x01000000
	}
};

#else  /* Old small partitions */
static struct mtd_partition btweb_partitions[] = {
	{
		name:		"Bootloader",
		size:		0x00040000,
		offset:		0,
	},{
		name:		"Kernel",
		size:		0x00100000,
		offset:		0x00040000,
	},{
		name:		"Filesystem",
		size:		MTDPART_SIZ_FULL,
		offset:		0x00140000
	}
};
#endif

#define NB_OF(x)  (sizeof(x)/sizeof(x[0]))

static struct mtd_info *mymtd;
static struct mtd_partition *parsed_parts;

extern int parse_redboot_partitions(struct mtd_info *master, struct mtd_partition **pparts);

static int __init init_btweb(void)
{
	struct mtd_partition *parts;
	int nb_parts = 0;
	int parsed_nr_parts = 0;
	char *part_type = "static";

	/* was done with BOOT_DEF, ignore it now */
	btweb_map.buswidth = 2;
	printk( "Probing BtWeb flash at physical address 0x%08x (%d-bit buswidth)\n",
		WINDOW_ADDR, btweb_map.buswidth * 8 );
	btweb_map.map_priv_1 = (unsigned long)__ioremap(WINDOW_ADDR, WINDOW_SIZE, 0);
	if (!btweb_map.map_priv_1) {
		printk("Failed to ioremap\n");
		return -EIO;
	}
	mymtd = do_map_probe("cfi_probe", &btweb_map);
	if (!mymtd) {
		iounmap((void *)btweb_map.map_priv_1);
		return -ENXIO;
	}
	mymtd->module = THIS_MODULE;

#ifdef CONFIG_MTD_REDBOOT_PARTS
	if (parsed_nr_parts == 0) {
		int ret = parse_redboot_partitions(mymtd, &parsed_parts);

		if (ret > 0) {
			part_type = "RedBoot";
			parsed_nr_parts = ret;
		}
	}
#endif

	if (parsed_nr_parts > 0) {
		parts = parsed_parts;
		nb_parts = parsed_nr_parts;
	} else if ((btweb_globals.flavor==BTWEB_PE)||(btweb_globals.flavor==BTWEB_PBX288exp)||(btweb_globals.flavor==BTWEB_INTERFMM)||(btweb_globals.flavor==BTWEB_H4684_IP)) {
		printk(KERN_NOTICE "Mtd BTWEB partitions (extra 10MB) - app smaller\n");
		parts = btweb_extra_partitions;
		nb_parts = NB_OF(btweb_extra_partitions);
	} else if ((btweb_globals.flavor==BTWEB_BMNE500) || \
		  (btweb_globals.flavor==BTWEB_MH200)) {
		printk(KERN_NOTICE "Mtd BTWEB partitions for BMNE500 or F453 or MH200\n");
		parts = btweb_partitions_mh500;
		nb_parts = NB_OF(btweb_partitions_mh500);
	} else if (btweb_globals.flavor==BTWEB_F453) {
		printk(KERN_NOTICE "Mtd BTWEB partitions for F453 \n");
		parts = btweb_partitions_f453;
		nb_parts = NB_OF(btweb_partitions_f453);
	} else if (bt_mtd==1) {
		printk(KERN_NOTICE "btweb_custom_1 flash partition\n");
		parts = btweb_custom_1;
		nb_parts = NB_OF(btweb_custom_1);
        } else if (btweb_globals.flavor==BTWEB_LGR03617) {
                printk(KERN_NOTICE "Mtd BTWEB partitions for OMIZZY\n");
                parts = btweb_partitions_lgr03617;
                nb_parts = NB_OF(btweb_partitions_lgr03617);
	} else {
		printk(KERN_NOTICE "Mtd BTWEB partitions\n");
		parts = btweb_partitions;
		nb_parts = NB_OF(btweb_partitions);
	}
	if (nb_parts) {
		printk(KERN_NOTICE "Using %s partition definition\n", part_type);
		add_mtd_partitions(mymtd, parts, nb_parts);
	} else {
		add_mtd_device(mymtd);
	}
	return 0;
}

static void __exit cleanup_btweb(void)
{
	if (mymtd) {
		del_mtd_partitions(mymtd);
		map_destroy(mymtd);
		if (parsed_parts)
			kfree(parsed_parts);
	}
	if (btweb_map.map_priv_1)
		iounmap((void *)btweb_map.map_priv_1);
	return;
}

module_init(init_btweb);
module_exit(cleanup_btweb);

MODULE_PARM(bt_mtd, "i");
MODULE_PARM_DESC (bt_mtd, "btweb: Id for custom flash partition");

