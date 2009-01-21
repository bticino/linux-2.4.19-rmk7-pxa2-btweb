/*
 * $Id:
 *
 * Map driver for the Trizeps-2 module.
 *
 * Author:	Luc De Cock
 * Copyright:	(C) 2003 Teradyne DS, Ltd.
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


#define WINDOW_ADDR 	0
#define WINDOW_SIZE 	16*1024*1024

static __u8 trizeps2_read8(struct map_info *map, unsigned long ofs)
{
	return *(__u8 *)(map->map_priv_1 + ofs);
}

static __u16 trizeps2_read16(struct map_info *map, unsigned long ofs)
{
	return *(__u16 *)(map->map_priv_1 + ofs);
}

static __u32 trizeps2_read32(struct map_info *map, unsigned long ofs)
{
	return *(__u32 *)(map->map_priv_1 + ofs);
}

static void trizeps2_copy_from(struct map_info *map, void *to, unsigned long from, ssize_t len)
{
	memcpy(to, (void *)(map->map_priv_1 + from), len);
}

static void trizeps2_write8(struct map_info *map, __u8 d, unsigned long adr)
{
	*(__u8 *)(map->map_priv_1 + adr) = d;
}

static void trizeps2_write16(struct map_info *map, __u16 d, unsigned long adr)
{
	*(__u16 *)(map->map_priv_1 + adr) = d;
}

static void trizeps2_write32(struct map_info *map, __u32 d, unsigned long adr)
{
	*(__u32 *)(map->map_priv_1 + adr) = d;
}

static void trizeps2_copy_to(struct map_info *map, unsigned long to, const void *from, ssize_t len)
{
	memcpy((void *)(map->map_priv_1 + to), from, len);
}

static struct map_info trizeps2_map = {
	name:		"Trizeps-2 flash",
	size:		WINDOW_SIZE,
	read8:		trizeps2_read8,
	read16:		trizeps2_read16,
	read32:		trizeps2_read32,
	copy_from:	trizeps2_copy_from,
	write8:		trizeps2_write8,
	write16:	trizeps2_write16,
	write32:	trizeps2_write32,
	copy_to:	trizeps2_copy_to
};

static struct mtd_partition trizeps2_partitions[] = {
	{
		name:		"Bootloader",
		size:		0x00040000,
		offset:		0,
		mask_flags:	MTD_WRITEABLE  /* force read-only */
	},{
		name:		"Bootloader (backup)",
		size:		0x00040000,
		offset:		0x00040000,
		mask_flags:	MTD_WRITEABLE  /* force read-only */
	},{
		name:		"Kernel",
		size:		0x000C0000,
		offset:		0x00080000,
	},{
		name:		"Filesystem",
		size:		MTDPART_SIZ_FULL,
		offset:		0x00140000
	}
};

#define NB_OF(x)  (sizeof(x)/sizeof(x[0]))

static struct mtd_info *mymtd;
static struct mtd_partition *parsed_parts;

extern int parse_redboot_partitions(struct mtd_info *master, struct mtd_partition **pparts);

static int __init init_trizeps2(void)
{
	struct mtd_partition *parts;
	int nb_parts = 0;
	int parsed_nr_parts = 0;
	char *part_type = "static";

	trizeps2_map.buswidth = (BOOT_DEF & 1) ? 2 : 4;
	printk( "Probing Trizeps-2 flash at physical address 0x%08x (%d-bit buswidth)\n",
		WINDOW_ADDR, trizeps2_map.buswidth * 8 );
	trizeps2_map.map_priv_1 = (unsigned long)__ioremap(WINDOW_ADDR, WINDOW_SIZE, 0);
	if (!trizeps2_map.map_priv_1) {
		printk("Failed to ioremap\n");
		return -EIO;
	}
	mymtd = do_map_probe("cfi_probe", &trizeps2_map);
	if (!mymtd) {
		iounmap((void *)trizeps2_map.map_priv_1);
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
	} else {
		parts = trizeps2_partitions;
		nb_parts = NB_OF(trizeps2_partitions);
	}
	if (nb_parts) {
		printk(KERN_NOTICE "Using %s partition definition\n", part_type);
		add_mtd_partitions(mymtd, parts, nb_parts);
	} else {
		add_mtd_device(mymtd);
	}
	return 0;
}

static void __exit cleanup_trizeps2(void)
{
	if (mymtd) {
		del_mtd_partitions(mymtd);
		map_destroy(mymtd);
		if (parsed_parts)
			kfree(parsed_parts);
	}
	if (trizeps2_map.map_priv_1)
		iounmap((void *)trizeps2_map.map_priv_1);
	return 0;
}

module_init(init_trizeps2);
module_exit(cleanup_trizeps2);

