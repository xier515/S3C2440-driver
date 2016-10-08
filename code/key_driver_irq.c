#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/irq.h>


#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/arch/regs-gpio.h>


/*the driver class struct was supported by wdev????  */
static struct class	*key_drv_class;
static struct class_device	*key_drv_class_dev;
static struct timer_list buttons_timer;

volatile unsigned int *gpfcon;
volatile unsigned int *gpfdat;
volatile unsigned int *gpgcon;
volatile unsigned int *gpgdat;

struct pin_desc{
	unsigned int pin;
	unsigned int key_val;
};

unsigned char  key_val;
static volatile int ev_press = 0; 

static DECLARE_WAIT_QUEUE_HEAD(button_waitq);

struct pin_desc pins_desc[4] = {
	{S3C2410_GPF0, 0x01}, 	
	{S3C2410_GPF2, 0x02}, 	
	{S3C2410_GPG3, 0x03}, 	
	{S3C2410_GPG11, 0x04}
};
static struct pin_desc *irq_pd;
struct fasync_struct button_async;

static irqreturn_t buttons_irq(int irq, void *dev_id)
{
	irq_pd = (struct pin_desc *) dev_id;
	mod_timer(&buttons_timer, jiffies+2);
	return IRQ_RETVAL(IRQ_HANDLED);
}
int key_drv_open(struct inode *inode, struct file *file)
{
	request_irq(IRQ_EINT0, buttons_irq, IRQT_BOTHEDGE, "S2", &pins_desc[0]);
	request_irq(IRQ_EINT2, buttons_irq, IRQT_BOTHEDGE, "S3", &pins_desc[1]);
	request_irq(IRQ_EINT11, buttons_irq, IRQT_BOTHEDGE, "S4", &pins_desc[2]);
	request_irq(IRQ_EINT19, buttons_irq, IRQT_BOTHEDGE, "S5", &pins_desc[3]);
	return 0;
}
ssize_t key_drv_read(struct file *file, char __user *buf, size_t len, loff_t *ppos) 
{
	if(len != 1)
		return ~EINVAL;
	wait_event_interruptible(button_waitq, ev_press);
	ev_press = 0;
	copy_to_user(buf, &key_val, 1);
	return 0;
}

int key_drv_close(struct inode *inode, struct file *file)
{
	free_irq(IRQ_EINT0, &pins_desc[0]);
	free_irq(IRQ_EINT2, &pins_desc[1]);
	free_irq(IRQ_EINT11, &pins_desc[2]);
	free_irq(IRQ_EINT19, &pins_desc[3]);
	return 0;
}
int key_drv_fasync(int fd, struct file *file, int on)
{
	printk("driver: key_drv_fasync\n");
	return fasync_helper(fd, file, on, &button_async);
}
/*this struct was define in /linux/fs.h */
static struct file_operations key_drv_fops = {
	.owner	=	THIS_MODULE,
	.open	=	key_drv_open,
	.read	=	key_drv_read,
	.release	=	key_drv_close,
	.fasync	=	key_drv_fasync,
};

int major;//store 
static void buttons_timer_function(unsigned long data)
{
	struct pin_desc * pindesc = (struct pin_desc *)irq_pd;
	unsigned int pinval;
	
	if(!pindesc)
		return ;
	pinval = s3c2410_gpio_getpin(pindesc->pin);
	if(pinval)
	{
		/* release  */
		key_val = pindesc->key_val | 0x80;
	}
	else
	{
		/* press */	
		key_val = pindesc->key_val;
	}
	ev_press = 1;
	wake_up_interruptible(&button_waitq);
	
	kill_fasync(&button_async, SIGIO, POLL_IN);
}
int key_drv_init(void)
{
	init_timer(&buttons_timer);
	buttons_timer.function = buttons_timer_function;
	add_timer(&buttons_timer);
	major = register_chrdev(0, "key_drv", &key_drv_fops);

	gpfcon = (volatile unsigned int *)ioremap(0x56000050, 16);
	gpfdat = gpfcon + 1;

	gpgcon = (volatile unsigned int *)ioremap(0x56000060, 16);
	gpgdat = gpgcon + 1;

	key_drv_class = class_create(THIS_MODULE, "Button");
	key_drv_class_dev = class_device_create(key_drv_class, NULL, MKDEV(major, 0), NULL, "buttons");	
	return 0;
}

int key_drv_exit(void)
{
	class_device_unregister(key_drv_class_dev);
	class_destroy(key_drv_class);
	
	iounmap(gpfcon);
	iounmap(gpgcon);
	unregister_chrdev(major, "key_drv");
	return 0;
}


module_init(key_drv_init);
module_exit(key_drv_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("LiangJiaxiang");
