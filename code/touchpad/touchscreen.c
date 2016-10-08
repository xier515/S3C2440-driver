
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
static struct timer_list ts_timer;

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
		//printk("pen up.\n");
		input_report_abs(touchscreen_device, ABS_PRESSURE, 0);
		input_report_key(touchscreen_device, BTN_TOUCH, 0); 
		input_sync(touchscreen_device);
		enter_wait_pen_down_mode();
	}
	else
	{
		//printk("pen down.\n");
 		input_report_abs(touchscreen_device, ABS_PRESSURE, 1);
		input_report_key(touchscreen_device, BTN_TOUCH, 1); 
		input_sync(touchscreen_device);
		enter_measure_xy_mode();
		adc_start();
	}
	return IRQ_HANDLED;
}
static irqreturn_t adc_irq(int irq, void *dev_id)
{
	static int cnt = 0;
	static int x[5],y[5];
	int adc_data0, adc_data1;
	adc_data0 = (s3c_ts_reg->adcdat0 & 0x3ff);
	adc_data1 = (s3c_ts_reg->adcdat1 & 0x3ff);
	if(s3c_ts_reg->adcdat0 & (1<<15)) //检查这次按下时，触摸笔还在不在。
	{
		cnt = 0;
		enter_wait_pen_down_mode();
		input_report_abs(touchscreen_device, ABS_PRESSURE, 0);
		input_report_key(touchscreen_device, BTN_TOUCH, 0); 
		input_sync(touchscreen_device);
	}
	else
	{
		x[cnt] = adc_data0;
		y[cnt] = adc_data1;
		cnt++;
		if(cnt == 5)
		{
			//printk("x = %d, y = %d.\n", (x[0]+x[1]+x[2]+x[3]+x[4])/5, (y[0]+y[1]+y[2]+y[3]+y[4])/5);
			input_report_abs(touchscreen_device, ABS_X, (x[0]+x[1]+x[2]+x[3]+x[4])/5);
 			input_report_abs(touchscreen_device, ABS_Y, (y[0]+y[1]+y[2]+y[3]+y[4])/5);
 			input_report_abs(touchscreen_device, ABS_PRESSURE, 1);
 			input_report_key(touchscreen_device, BTN_TOUCH, 1); 
 			input_sync(touchscreen_device);
			cnt = 0;
			enter_wait_pen_up_mode();
			//在这里启动定时器来处理长按或者滑动的情况。
			mod_timer(&ts_timer, jiffies + HZ/10);
		}
		else
		{
			enter_measure_xy_mode();
			adc_start();
		}
	}
	return IRQ_HANDLED;
}
static void s3c_fs_tiemr_function(unsigned long data)
{
	if(s3c_ts_reg->adcdat0 & (1<<15)) //检查这次按下时，触摸笔还在不在。
	{
		enter_wait_pen_down_mode();
	}
	else
	{
		enter_measure_xy_mode();
		adc_start();
	}
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
	s3c_ts_reg->adcdly = 0xffff; //大延迟以缓解飞点问题。
	s3c_ts_reg->adccon = (1<<14) | (49<<6);
	
	request_irq(IRQ_TC, pen_down_up_irq, IRQF_SAMPLE_RANDOM, "ts_pen", NULL);
	request_irq(IRQ_ADC, adc_irq, IRQF_SAMPLE_RANDOM, "adc", NULL);

	init_timer(&ts_timer);
	ts_timer.function = s3c_fs_tiemr_function;
	add_timer(&ts_timer); 
	enter_wait_pen_down_mode();
	//platform_driver_register(&touchscreen_driver);
	return 0;
}

static void __exit touchscreen_exit(void)
{
	//platform_driver_unregister(&touchscreen_driver);
	del_timer(&ts_timer);
	free_irq(IRQ_TC, NULL);
	free_irq(IRQ_ADC, NULL);
	iounmap(s3c_ts_reg);
	input_unregister_device(touchscreen_device);
	input_free_device(touchscreen_device);
}

module_init(touchscreen_init);
module_exit(touchscreen_exit);
MODULE_LICENSE("GPL");


