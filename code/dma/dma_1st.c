#include <linux/module.h>
#include <linux/version.h>

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/pm.h>
#include <linux/sysctl.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/poll.h>

#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/hardware.h>

static int major;

#define MEM_CPY_NO_DMA 	0
#define MEM_CPY_DMA		1

#define BUF_SIZE 1024*1024
static char *src;
static unsigned long src_phys;
static char *dst;
static unsigned long dst_phys;

static struct class *cls;

static int dma_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	int i;
	
	switch(cmd)
	{
		case MEM_CPY_NO_DMA:
		{
			for(i=0; i<BUF_SIZE; i++)
			{
				dst[i] = src[i];
				if(memcmp(src, dst, BUF_SIZE) == 0)
				{
					printk("MEM_CPY_NO_DMA OK\n");
				}
				else
				{
					printk("MEM_CPY_NO_DMA ERROR\n");
				}
			}
			break;
		}
		case MEM_CPY_DMA:
		{
			
			break;
		}
	}
}

static struct file_operations dma_fops = {
	.owner = THIS_MODULE,
	.ioctl = dma_ioctl,
};

static int __init s3c_dma_init(void)
{
	

	/*分配SRC, DST对应的缓冲区*/
	/*kzalloc分配出来的空间，虚拟地址是连续的。但物理地址可能会不连续。*/
	src = dma_alloc_writecombine(NULL, BUF_SIZE, &src_phys, GFP_KERNEL);
	if(src == NULL)
	{
		printk("can't alloc buffer for src.\n");
		return -ENOMEM;
	}	
	dst = dma_alloc_writecombine(NULL, BUF_SIZE, &dst_phys, GFP_KERNEL);
	if(dst == NULL)
	{
		printk("can't alloc buffer for dst.\n");
		return -ENOMEM;
	}	
	major = register_chrdev(0, "dma", &dma_fops);
	cls = class_create(THIS_MODULE, "dma");
	class_device_create(cls, NULL, MKDEV(major, 0), NULL, "dma");
	return 0;
}
static void __exit s3c_dma_exit(void)
{
	class_device_destroy(cls, MKDEV(major, 0));
	class_destroy(cls);
	
	unregister_chrdev(major, "dma");

	
	dma_free_writecombine(NULL , BUF_SIZE, src, src_phys);
	dma_free_writecombine(NULL , BUF_SIZE, src, src_phys);
	
}

module_init(s3c_dma_init);
module_exit(s3c_dma_exit);
MODULE_LICENSE("GPL");

