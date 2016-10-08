
#include <linux/module.h>
#include <linux/version.h>

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

#include <asm/uaccess.h>
#include <asm/io.h>

static int major;

static struct class *cls;
static volatile unsigned long *gpio_dat;
static volatile unsigned long *gpio_con;
static int pin;

static int led_open(struct inode *inode, struct file *file)
{
	*gpio_con &= ~(0x3<<(pin*2));
	*gpio_con |= (0x1<<(pin*2));//配置成输出模式
	return 0;
}
static ssize_t led_write(struct file *file, const char __user *buf, size_t	count, loff_t *ppos)
{
	unsigned int val;
	if(copy_from_user(&val, buf, count) < 0)	return -1;
	if(val == 1)
	{
		*(gpio_dat) &= ~(1<<pin);
		//printk("led on\n");
	}
	else 
	{	
		*(gpio_dat) |= (1<<pin);
		//printk("led off\n");
	}
	return 0;
}

static struct file_operations led_fops = {
	.owner =  THIS_MODULE, //指向便宜模块时自动创建的__this_module变量
	.open = led_open,
	.write = led_write,
	
};

//probe函数的工作是根据device提供的资源表申请硬件资源。
//并且完成字符设备驱动程序的注册，设备类的创建。
static int led_probe(struct platform_device *pdev)
{
	struct resource *res;
	
	/*根据platform_device的资源进行ioremap */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	gpio_con = ioremap(res->start, res->end - res->start+1);
	gpio_dat = gpio_con +1;

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	pin = res->start;
	
	/*注册字符设备驱动程序*/
	major = register_chrdev(0, "myleds", &led_fops);

	cls = class_create(THIS_MODULE, "myleds");
	class_device_create(cls, NULL, MKDEV(major, 0), NULL, "led");
	printk("led_probe");
	return 0;
}
//remove则把probe函数申请的资源释放，创建的设备类销毁。
static int led_remove(struct platform_device *pdev)
{
	

	/*卸载字符设备驱动程序*/
	class_device_destroy(cls, MKDEV(major,0));
	class_destroy(cls);

	unregister_chrdev(major, "myleds");
	/*根据platform_device的资源进行iounmap */ 
	iounmap(gpio_con);
	printk("led_remove");
	return 0;
}
static struct platform_driver led_driver = {
	.probe = led_probe,
	.remove = led_remove,
	.driver = {
		.name = "myled",
	}
};


//这里所做的工作主要就是完成一个platform_driver结构体，这个结构体包含probe函数与remove函数。
//这个结构体还要包含支持的设备名字。

static int __init led_drv_init(void)
{
	platform_driver_register(&led_driver);
	return 0;
}
static void __exit led_drv_exit(void)
{
	platform_driver_unregister(&led_driver);
}

module_init(led_drv_init);
module_exit(led_drv_exit);
MODULE_LICENSE("GPL");

