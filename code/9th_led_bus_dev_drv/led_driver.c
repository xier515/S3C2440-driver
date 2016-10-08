
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
	*gpio_con |= (0x1<<(pin*2));//���ó����ģʽ
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
	.owner =  THIS_MODULE, //ָ�����ģ��ʱ�Զ�������__this_module����
	.open = led_open,
	.write = led_write,
	
};

//probe�����Ĺ����Ǹ���device�ṩ����Դ������Ӳ����Դ��
//��������ַ��豸���������ע�ᣬ�豸��Ĵ�����
static int led_probe(struct platform_device *pdev)
{
	struct resource *res;
	
	/*����platform_device����Դ����ioremap */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	gpio_con = ioremap(res->start, res->end - res->start+1);
	gpio_dat = gpio_con +1;

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	pin = res->start;
	
	/*ע���ַ��豸��������*/
	major = register_chrdev(0, "myleds", &led_fops);

	cls = class_create(THIS_MODULE, "myleds");
	class_device_create(cls, NULL, MKDEV(major, 0), NULL, "led");
	printk("led_probe");
	return 0;
}
//remove���probe�����������Դ�ͷţ��������豸�����١�
static int led_remove(struct platform_device *pdev)
{
	

	/*ж���ַ��豸��������*/
	class_device_destroy(cls, MKDEV(major,0));
	class_destroy(cls);

	unregister_chrdev(major, "myleds");
	/*����platform_device����Դ����iounmap */ 
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


//���������Ĺ�����Ҫ�������һ��platform_driver�ṹ�壬����ṹ�����probe������remove������
//����ṹ�廹Ҫ����֧�ֵ��豸���֡�

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

