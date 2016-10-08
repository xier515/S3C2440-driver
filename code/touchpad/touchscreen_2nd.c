
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/init.h>
#include <linux/serio.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <asm/io.h>
#include <asm/irq.h>

static struct input_dev *touchscreen_device;

struct s3c_ts_reg {
	unsigned long adccon;
	unsigned long adctsc;
	unsigned long adcdly;
	unsigned long adcdat0;
	unsigned long adcdat1;
	unsigned long adcupdn;
};
static volatile struct s3c_ts_reg *s3c_ts_reg;
static int touchscreen_probe(struct platform_device *pdev)
{
	
}
static int touchscreen_remove(struct platform_device *pdev)
{
	
}

static void enter_wait_pen_down_mode(void)
{
	s3c_ts_reg->adctsc = 0xd3;
}

static void enter_wait_pen_up_mode(void)
{
	s3c_ts_reg->adctsc = 0x1d3;
}
static void enter_measure_xy_mode(void)
{
	s3c_ts_reg->adctsc = (1<<3) | (1<<2);
}
static void adc_start(void)
{
	s3c_ts_reg->adccon |= (1<<0);
}
static irqreturn_t pen_down_up_irq(int irq, void *dev_id)
{
	if(s3c_ts_reg->adcdat0 & (1<<15))
	{
		printk("pen up.\n");
		enter_wait_pen_down_mode();
	}
	else
	{
		printk("pen down.\n");
		//enter_wait_pen_up_mode();
		enter_measure_xy_mode();
		adc_start();
	}
	return IRQ_HANDLED;
}
static irqreturn_t adc_irq(int irq, void *dev_id)
{
	static int cnt = 0;
	enter_wait_pen_up_mode();
	printk("adc_irq cnt = %d : x = %ld, y = %ld.\n", ++cnt, (s3c_ts_reg->adcdat0 & 0x3ff), (s3c_ts_reg->adcdat1 & 0x3ff));
	return IRQ_HANDLED;
}
static struct platform_driver touchscreen_driver = {
	.driver = {
		.name = "s3c2440 TouchScreen",
		.owner = THIS_MODULE,
	},
	.probe = touchscreen_probe,
	.remove = touchscreen_remove,
};

static int __init touchscreen_init(void)
{
	struct clk *clk ;
	//1.分配一个input-device结构体。
	touchscreen_device = input_allocate_device();

	//2.设置
	//2.1 能产生哪些事件
	set_bit(EV_KEY, touchscreen_device->evbit);
	set_bit(EV_ABS, touchscreen_device->evbit);
	//2.2 能产生这类事件里的哪些事件
	set_bit(BTN_TOUCH, touchscreen_device->keybit);
	
	input_set_abs_params(touchscreen_device, ABS_X, 0, 0x3FF, 0, 0);
	input_set_abs_params(touchscreen_device, ABS_Y, 0, 0x3FF, 0, 0);
	input_set_abs_params(touchscreen_device, ABS_PRESSURE, 0, 1, 0, 0);
	//3.注册
	input_register_device(touchscreen_device);

	//4.硬件相关操作。
	//4.1 使能时钟
	clk = clk_get(NULL, "adc");
	clk_enable(clk);
	//4.2设置ADC寄存器
	s3c_ts_reg = ioremap(0x58000000, sizeof(struct s3c_ts_reg));

	s3c_ts_reg->adccon = (1<<14) | (49<<6);

	request_irq(IRQ_TC, pen_down_up_irq, IRQF_SAMPLE_RANDOM, "ts_pen", NULL);
	request_irq(IRQ_ADC, adc_irq, IRQF_SAMPLE_RANDOM, "adc", NULL);
	enter_wait_pen_down_mode();
	//platform_driver_register(&touchscreen_driver);
	return 0;
}

static void __exit touchscreen_exit(void)
{
	//platform_driver_unregister(&touchscreen_driver);
	free_irq(IRQ_TC, NULL);
	iounmap(s3c_ts_reg);
	input_unregister_device(touchscreen_device);
	input_free_device(touchscreen_device);
}

module_init(touchscreen_init);
module_exit(touchscreen_exit);
MODULE_LICENSE("GPL");


