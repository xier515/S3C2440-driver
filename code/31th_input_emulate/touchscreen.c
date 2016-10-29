
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
#include <asm/uaccess.h>

#define INPUT_REPLAY 0

#define MYLOG_BUF_LENGTH (1024*1024)
extern int myprintf(const char *fmt, ...);

static struct input_dev *touchscreen_device;
static struct timer_list ts_timer;
static char *replay_buf;

static int replay_r = 0;
static int replay_w = 0;
static int major = 0; //replay_dev
static struct class *cls;
static struct timer_list replay_timer;

static ssize_t replay_write(struct file *file, const char __user *buf, size_t size, loff_t *offset)
{
	int err;
	//把应用程序传入的数据写入replay_buf
	if(replay_w + size >= MYLOG_BUF_LENGTH)
	{
		printk("replay_buf full.\n");
		return -EIO;
	}
	err = copy_from_user(replay_buf + replay_w, buf, size);
	if(err)
	{
		return -EIO;
	}	
	else
	{
		replay_w += size;
	}
	return size;
}
//app: ioctl(fd, CMD, ...);
static int replay_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long val)
{
	switch(cmd)
	{
		case INPUT_REPLAY:
		{
			//启动回放:根据replay_buf缓冲区里的值上报事件
			replay_timer.expires = jiffies + 5;
			add_timer(&replay_timer);
			break;
		}
	}
	return 0;
}	

//返回
static int replay_get_line(char *line)
{
	int i=0;
	//吃掉前导的空格、回车符
	while( replay_r <= replay_w)
	{
		if((replay_buf[replay_r] == ' ') || (replay_buf[replay_r] == '\n') || (replay_buf[replay_r] == '\r') || (replay_buf[replay_r] == '\t'))
			replay_r++;
		else
			break;
	}

	while( replay_r <= replay_w)
	{
		if((replay_buf[replay_r] == '\n') || (replay_buf[replay_r] == '\r'))
			break;
		else
		{
			line[i] = replay_buf[replay_r];
			replay_r++;
			i++;
		}
	}
	line[i] = '\0';
	return i;
}
static void input_relay_timer_function(unsigned long data)
{	// jiffies       type     code     val
	//0x000f0bdc 0x00000003 0x00000018 1
	//把replay_buf里的一些数据取出来，上报
	//读出第一行数据 确定time值 上报第一行
	//
	//根据下一个要上报的数据的时间mod_timer
	unsigned int time, next_time;
	unsigned int type;
	unsigned int code;
	int val;

	static unsigned int pre_time = 0, pre_type = 0, pre_code = 0;
	static int pre_val = 0;
	
	char line[100];
	int ret;

	if(pre_time != 0) //如果是不是第一次进入定时器，就上报事件
	{
		input_event(touchscreen_device, pre_type, pre_code, pre_val);
	}
	while(1)
	{
		ret = replay_get_line(line);
		if(ret == 0)
		{
			printk("end of input replay.\n");
			del_timer(&replay_timer);
			pre_time = pre_type = pre_code = 0;
			pre_val = 0;
			replay_r = replay_w = 0; 
			break;
		} 
		//处理数据
		time = 0;
		type = 0;
		code = 0;
		val = 0;
		sscanf(line, "%x %x %x %d", &time, &type, &code, &val);
		printk("%x %x %x %d\n", time, type, code, val);
		//printk(line);
		//printk('\n');
		if(!time && !type && !code && !val)
		{
			continue;
		}
		else
		{
			if((pre_time == 0) || (time == pre_time))
			{
				input_event(touchscreen_device, type, code, val);
				//printk("replay:%x %x %d.\n", type, code, val);
				if(pre_time == 0)
					pre_time = time;
			}
			else
			{
				//根据下一个要上报的数据设定定时器时间。
				mod_timer(&replay_timer, jiffies + (time - pre_time));
				
				pre_time = time;	
				pre_type = type;
				pre_code = code;
				pre_val = val;
				
				break;
			}
			
		}
	}
}
static struct file_operations fops = {
	.owner = THIS_MODULE,
	.write = replay_write,
	.ioctl = replay_ioctl,
};

struct s3c_ts_reg {
	unsigned long adccon;
	unsigned long adctsc;
	unsigned long adcdly;
	unsigned long adcdat0;
	unsigned long adcdat1;
	unsigned long adcupdn;
};
static volatile struct s3c_ts_reg *s3c_ts_reg;
static void write_input_event_to_file(unsigned int time, unsigned int type, unsigned int code, int val)
{
	myprintf("0x%08x 0x%08x 0x%08x %d\n", time, type, code ,val);
}
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
		write_input_event_to_file(jiffies, EV_ABS, ABS_PRESSURE, 0);
		
		input_report_key(touchscreen_device, BTN_TOUCH, 0); 
		write_input_event_to_file(jiffies, EV_KEY, BTN_TOUCH, 0);
		
		input_sync(touchscreen_device);
		write_input_event_to_file(jiffies, EV_SYN, SYN_REPORT, 0);
		enter_wait_pen_down_mode();
	}
	else
	{
		//printk("pen down.\n");
 		input_report_abs(touchscreen_device, ABS_PRESSURE, 1);
 		write_input_event_to_file(jiffies, EV_ABS, ABS_PRESSURE, 1);
 		
		input_report_key(touchscreen_device, BTN_TOUCH, 1); 
		write_input_event_to_file(jiffies, EV_KEY, BTN_TOUCH, 1);
		
		input_sync(touchscreen_device);
		write_input_event_to_file(jiffies, EV_SYN, SYN_REPORT, 0);
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
		write_input_event_to_file(jiffies, EV_ABS, ABS_PRESSURE, 0);
		
		input_report_key(touchscreen_device, BTN_TOUCH, 0); 
		write_input_event_to_file(jiffies, EV_KEY, BTN_TOUCH, 0);
		
		input_sync(touchscreen_device);
		write_input_event_to_file(jiffies, EV_SYN, SYN_REPORT, 0);
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
			write_input_event_to_file(jiffies, EV_ABS, ABS_X, (x[0]+x[1]+x[2]+x[3]+x[4])/5);
 			input_report_abs(touchscreen_device, ABS_Y, (y[0]+y[1]+y[2]+y[3]+y[4])/5);
 			write_input_event_to_file(jiffies, EV_ABS, ABS_Y, (y[0]+y[1]+y[2]+y[3]+y[4])/5);
 			input_report_abs(touchscreen_device, ABS_PRESSURE, 1);
 			write_input_event_to_file(jiffies, EV_ABS, ABS_PRESSURE, 1);
 			
 			input_report_key(touchscreen_device, BTN_TOUCH, 1); 
 			write_input_event_to_file(jiffies, EV_KEY, BTN_TOUCH, 1);
 			
 			input_sync(touchscreen_device);
 			write_input_event_to_file(jiffies, EV_SYN, SYN_REPORT, 0);
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
	replay_buf = kmalloc(MYLOG_BUF_LENGTH, GFP_KERNEL);
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

	major = register_chrdev(0, "input_emu", &fops);
	cls = class_create(THIS_MODULE, "input_replay");
	device_create(cls, NULL, MKDEV(major, 0), "input_emu");

	init_timer(&replay_timer);
	replay_timer.function = input_relay_timer_function;
	//add_timer(&replay_timer);
	return 0;
}

static void __exit touchscreen_exit(void)
{
	//del_timer(&replay_timer);
	device_destroy(cls, MKDEV(major, 0));
	class_destroy(cls);
	unregister_chrdev(major, "input_emu");
	//platform_driver_unregister(&touchscreen_driver);
	
	del_timer(&ts_timer);
	free_irq(IRQ_TC, NULL);
	free_irq(IRQ_ADC, NULL);
	iounmap(s3c_ts_reg);
	input_unregister_device(touchscreen_device);
	input_free_device(touchscreen_device);
	kfree(replay_buf);
}

module_init(touchscreen_init);
module_exit(touchscreen_exit);
MODULE_LICENSE("GPL");


