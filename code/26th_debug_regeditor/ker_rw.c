#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/poll.h>
#include <linux/device.h>

#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/arch/regs-gpio.h>

#define KER_RW_R8 	0
#define KER_RW_R16 	1
#define KER_RW_R32 	2

#define KER_RW_W8 	3
#define KER_RW_W16	4
#define KER_RW_W32	5




static int major;
static struct class *cls;
static struct class_device *ker_dev;

static int ker_rw_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	volatile unsigned char *p8;
	volatile unsigned short *p16;
	volatile unsigned int *p32;

	unsigned int val = 0;
	unsigned int addr = 0;
	unsigned int buf[2];

	copy_from_user(buf, (const void __user *)arg, 8);
	addr = buf[0];
	val = buf[1];
	switch(cmd)
	{
		case KER_RW_R8:
		{
			p8 = (volatile unsigned char *)ioremap(addr, 4);
			val = *p8;
			copy_to_user((void __user *)(arg+4), &val, 4);
			iounmap(p8);
			break;
		}
		case KER_RW_R16:
		{
			p16 = (volatile unsigned short *)ioremap(addr, 4);
			val = *p16;
			copy_to_user((void __user *)(arg+4), &val, 4);
			iounmap(p16);
			break;
		}
		case KER_RW_R32:
		{
			p32 = (volatile unsigned int *)ioremap(addr, 4);
			val = *p32;
			copy_to_user((void __user *)(arg+4), &val, 4);
			iounmap(p32);
			break;
		}
		case KER_RW_W8:
		{
			p8 = (volatile unsigned char *)ioremap(addr, 4);
			*p8 = val;
			iounmap(p8);
			break;
		}
		case KER_RW_W16:
		{
			p16 = (volatile unsigned short *)ioremap(addr, 4);
			*p16 = val;
			iounmap(p16);
			break;
		}
		case KER_RW_W32:
		{
			p32 = (volatile unsigned int *)ioremap(addr, 4);
			*p32 = val;
			iounmap(p32);
			break;
		}
	}
}

static struct file_operations ker_rw_ops ={
	.owner = THIS_MODULE,
	.ioctl = ker_rw_ioctl,
};
static int __init ker_rw_init(void)
{
	major = register_chrdev(0, "ker_rw", &ker_rw_ops);
	cls = class_create(THIS_MODULE, "ker_rw");

	class_device_create(cls, NULL, MKDEV(major, 0), NULL, "ker_rw"); // /dev/ker_rw
	return 0;
}

static void __exit ker_rw_exit(void)
{
	unregister_chrdev(major, "ker_rw");
	class_device_destroy(cls, MKDEV(major, 0));
	class_destroy(cls);
}

module_init(ker_rw_init);
module_exit(ker_rw_exit);
MODULE_LICENSE("GPL");

