
#include <linux/config.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>

extern unsigned char btweb_bigbuf1[];
#define BB1_SIZE (256*1024)

struct page *bb1_nopage(struct vm_area_struct *vma,
                              unsigned long address, int unused)
{
    unsigned long offset = address - vma->vm_start;
    int pgoff = offset >> PAGE_SHIFT;
    struct page *page;

//    printk("bb1: page fault at 0x%08lx (0x%08lx - %i)\n",
//	   address, offset, pgoff);
 
    if (offset > BB1_SIZE)
	return NOPAGE_SIGBUS;

    page = virt_to_page(btweb_bigbuf1 + offset);
//    printk("bb1: got page %p\n", page);

    /* Increment the usage count of the page and return it */
    get_page(page);
    return page;
}

struct vm_operations_struct bb1_vm_ops = {
    nopage: bb1_nopage, 
};

int bb1_mmap(struct file *filp, struct vm_area_struct *vma)
{
    unsigned long offset = (vma)->vm_pgoff << PAGE_SHIFT;

    /* only mmap at offset 0 */
    if (offset) {
        printk(KERN_WARNING "bb1: mmap at non-0 offset\n");
        return -ENXIO;
    }
    /* don't check size, our nopage will SIGBUS any sinner */
    if (vma->vm_ops) {
        printk(KERN_WARNING "bb1: mmap with non-default vm_ops\n");
        return -EINVAL;
    }
    vma->vm_flags |= VM_RESERVED;
    vma->vm_file = filp;
    vma->vm_ops = &bb1_vm_ops;
    return 0;
}

struct file_operations bb1_fops = {
    owner:   THIS_MODULE,
    mmap:    bb1_mmap,
};


struct miscdevice bb1_misc = {
    minor: 48,
    name: "bb1",
    fops: &bb1_fops,
};

int bb1_init(void)
{
    return misc_register(&bb1_misc);
}

void bb1_exit(void)
{
    misc_deregister(&bb1_misc);
}


module_init(bb1_init);
module_exit(bb1_exit);
