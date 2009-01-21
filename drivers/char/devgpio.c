/*
 * Trivial generic /dev/gpio support. Quick and dirty to meet deadlines
 *
 * Copyright (C) 2005   Alessandro Rubini <rubini@linux.it>
 * Copyright (C) 2005   BTicino.it
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/ctype.h>

#include <asm/uaccess.h>
#include <asm/hardware.h>

#define DEVGPIO_MINOR 239 /* bah! */


/* Only write:
 *       Mx<num>=<mode>     change mode (0,1,2,3), x=i,o
 *       <num>=<bool>      set or clear
 * Reading a gpio bit is not supported (ioctl could be considered)
 */

ssize_t gpio_write(struct file *file, const char *ubuf, size_t count,
		   loff_t *offp)
{
	unsigned int domode=0, inout=0, i, j;
	char buf[16];
	char *p;

	if (count > 15) count = 15;
	if (copy_from_user(buf, ubuf, count))
		return -EFAULT;
	buf[count] = '\0';

	p = buf;
	/* skip blanks: it may be all blanks, then accept it */
	while (*p && isspace(*p))
		p++;
	if (!*p)
		return count;

	/* mode: accept M and I or O */
	if (*p == 'm' || *p == 'M') {
		domode++, p++;
		if (*p == 'i' || *p == 'I')
			inout= 0;
		else if (*p == 'o' || *p == 'O')
			inout = GPIO_MD_MASK_DIR;
		else return -EINVAL;
		p++;
	}

	/* get numbers */
	if (sscanf(p, "%i=%i", &i, &j) != 2)
		return -EINVAL;
	if (!i || i > 80 || j > 3)
		return -EINVAL;
	if (!domode && j > 1)
		return -EINVAL;

	/* act */
	if (domode) {
		if (!j)     domode =                0;
		if (j == 1) domode = GPIO_ALT_FN_1_IN;
		if (j == 2) domode = GPIO_ALT_FN_2_IN;
		if (j == 3) domode = GPIO_ALT_FN_3_IN;
		domode |= inout;
		set_GPIO_mode( i | domode);
	} else {
		if (j)
			GPSR(i) = GPIO_bit(i);
		else
			GPCR(i) = GPIO_bit(i);
	}
	*offp += count;
	return count;
}



static struct file_operations gpio_fops = {
	.write = gpio_write,
};

struct miscdevice gpio_misc = {
	.minor = DEVGPIO_MINOR,
	.name = "gpio",
	.fops = &gpio_fops,
};

static __init int gpio_init(void)
{
	int ret = misc_register(&gpio_misc);
	return ret;
}

static __exit void gpio_exit(void)
{
	misc_deregister(&gpio_misc);
}

module_init(gpio_init);
module_exit(gpio_exit);

