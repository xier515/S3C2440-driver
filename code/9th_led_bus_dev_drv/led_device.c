#include <linux/module.h>
#include <linux/version.h>

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/serial_core.h>
#include <linux/platform_device.h>


/*分配/设置/注册一个platform_device */
//这里主要定义了一个platform_device,里面包括硬件需要使用的资源。内存地址与IRQ中断号之类的。
static struct resource led_resource[]={
	[0] = {
		.start  = 0x56000050,
		.end    = 0x56000050 + 8 - 1,
		.flags  = IORESOURCE_MEM,
	},
	[1] = {
		.start  = 4,
		.end   = 4,
		.flags  = IORESOURCE_IRQ,
	},
}; 
void	led_release(struct device * dev)
{

}

static struct platform_device led_device = {
	.name = "myled",
	.id = -1,
	.num_resources = ARRAY_SIZE(led_resource),
	.resource = led_resource,
	.dev = {
		.release = led_release,
	},
};
static int __init led_dev_init(void)
{
	platform_device_register(&led_device);
	return 0;
}

static void __exit led_dev_exit(void)
{
	platform_device_unregister(&led_device);
}
module_init(led_dev_init);
module_exit(led_dev_exit);
MODULE_LICENSE("GPL");

