
/* local hack */
#ifndef __BACKPORT_26_H__
#define __BACKPORT_26_H__

#include <linux/slab.h>
#include <asm/hardware.h>

#ifndef gfp_t
#define gfp_t int
#endif

#ifndef __le16
#define __le16 u16
#endif

#ifndef __le32
#define __le32 u16
#endif

#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})

extern inline void *kzalloc(int size, int prio)
{
	void *res = kmalloc(size, prio);
	if (!res) return res;
	memset(res, 0, size);
	return res;
}

extern inline void pxa_set_cken(int clock, int enable)
{
        unsigned long flags;
        local_irq_save(flags);

        if (enable)
                CKEN |= clock;
        else
                CKEN &= ~clock;

        local_irq_restore(flags);
}

#define list_for_each_entry(pos, head, member)                          \
        for (pos = list_entry((head)->next, typeof(*pos), member);      \
             prefetch(pos->member.next), &pos->member != (head);        \
             pos = list_entry(pos->member.next, typeof(*pos), member))

typedef void irqreturn_t;
#define IRQ_NONE
#define IRQ_HANDLED
#define IRQ_RETVAL(x)

extern inline struct tty_driver *alloc_tty_driver(int lines)
{
        struct tty_driver *driver;

        driver = kmalloc(sizeof(struct tty_driver), GFP_KERNEL);
        if (driver) {
                memset(driver, 0, sizeof(struct tty_driver));
                driver->magic = TTY_DRIVER_MAGIC;
                driver->num = lines;
                /* later we'll move allocation of tables here */
        }
        return driver;
}

extern inline size_t strlcpy(char *dest, const char *src, size_t size)
{
        size_t ret = strlen(src);

        if (size) {
                size_t len = (ret >= size) ? size - 1 : ret;
                memcpy(dest, src, len);
                dest[len] = '\0';
        }
        return ret;
}

#endif /* __BACKPORT_26_H__ */
