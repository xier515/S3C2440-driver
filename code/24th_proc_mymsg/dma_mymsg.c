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
#include <linux/dma-mapping.h>

#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/hardware.h>

extern int myprintf(const char *fmt, ...);

struct s3c_dma_regs{
	unsigned long disrc;
	unsigned long disrcc;
	unsigned long didst;
	unsigned long didstc;
	unsigned long dcon;
	unsigned long dstat;
	unsigned long dcsrc;
	unsigned long dcdst;
	unsigned long dmasktrig;
};

static volatile struct s3c_dma_regs *s3c_dma0_regs;

static DECLARE_WAIT_QUEUE_HEAD(dma_wq);
static volatile int ev_dma = 0;

static int major;

#define MEM_CPY_NO_DMA 	0
#define MEM_CPY_DMA		1

#define BUF_SIZE 256*1024

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
			}
			if(memcmp(src, dst, BUF_SIZE) == 0)
			{
				myprintf("MEM_CPY_NO_DMA OK\n");
			}
			else
			{
				myprintf("MEM_CPY_NO_DMA ERROR\n");
			}
			break;
		}
		case MEM_CPY_DMA:
		{
			s3c_dma0_regs->disrc = src_phys;
			s3c_dma0_regs->disrcc= (0<<1) | (0<<0);
			s3c_dma0_regs->didst = dst_phys;
			s3c_dma0_regs->didstc= (0<<2) | (0<<1) | (0<<0);
			s3c_dma0_regs->dcon =(1<<30)| (1<<29) | (0<<28)|(1<<27)| (0<<23) | (0<<20) | BUF_SIZE;

			//启动DMA
			s3c_dma0_regs->dmasktrig = (1<<1) | (1<<0);

			//什么时候完成? 等待中断
			//休眠
			ev_dma = 0;//它是0则休眠
			wait_event_interruptible(dma_wq, ev_dma);
			
			if(memcmp(src, dst, BUF_SIZE) == 0)
			{
				myprintf("MEM_CPY_DMA OK\n");
			}
			else
			{
				myprintf("MEM_CPY_DMA ERROR\n");
			}
			
			break;
		}
	}
	return 0;
}

static struct file_operations dma_fops = {
	.owner = THIS_MODULE,
	.ioctl = dma_ioctl,
};
static irqreturn_t s3c_dma_irq(int irq, void *devid)
{
	ev_dma = 1;
	wake_up_interruptible(&dma_wq);
	return 0;
}

static int __init s3c_dma_init(void)
{
	s3c_dma0_regs = ioremap(0x4B0000C0, sizeof(struct s3c_dma_regs));
	if(request_irq(IRQ_DMA3, s3c_dma_irq, IRQF_DISABLED, "s3c_dma", 1))
	{
		myprintf("can't request_irq for DMA3\n");
		return -EBUSY;
	}
	/*分配SRC, DST对应的缓冲区*/
	/*kzalloc分配出来的空间，虚拟地址是连续的。但物理地址可能会不连续。*/
	src = (char *)dma_alloc_writecombine(NULL, BUF_SIZE, &src_phys, GFP_KERNEL);
	if(src == NULL)
	{
		free_irq(IRQ_DMA3, 1);
		myprintf("can't alloc buffer for src.\n");
		return -ENOMEM;
	}	
	dst = (char *) dma_alloc_writecombine(NULL, BUF_SIZE, &dst_phys, GFP_KERNEL);
	if(dst == NULL)
	{
		free_irq(IRQ_DMA3, 1);
		myprintf("can't alloc buffer for dst.\n");
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
	free_irq(IRQ_DMA3, 1);	
	iounmap(s3c_dma0_regs);
	
	dma_free_writecombine(NULL , BUF_SIZE, src, src_phys);
	dma_free_writecombine(NULL , BUF_SIZE, dst, dst_phys);

}

module_init(s3c_dma_init);
module_exit(s3c_dma_exit);
MODULE_LICENSE("GPL");


