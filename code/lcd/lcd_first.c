#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/fb.h>
#include <linux/init.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/wait.h>
#include <linux/platform_device.h>
#include <linux/clk.h>

#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/div64.h>

#include <asm/mach/map.h>
#include <asm/arch/regs-lcd.h>
#include <asm/arch/regs-gpio.h>
#include <asm/arch/fb.h>

static struct fb_info *s3c_lcd;

static struct fb_ops s3c_ops = {
	.owner		= THIS_MODULE,
//	.fb_setcolreg	= atmel_lcdfb_setcolreg,
	.fb_fillrect	= cfb_fillrect,
	.fb_copyarea	= cfb_copyarea,
	.fb_imageblit	= cfb_imageblit,
};


static int __init lcd_init(void)
{
	//1.����һ��fb_info
	s3c_lcd = framebuffer_alloc(0, NULL);
	//2.����
	//2.1���ù̶��Ĳ���
	strcpy(s3c_lcd->fix.id, "mylcd");
	s3c_lcd->fix.smem_len = 480*272*16;
	s3c_lcd->fix.type = FB_TYPE_PACKED_PIXELS;
	s3c_lcd->fix.visual = FB_VISUAL_TRUECOLOR;
	s3c_lcd->fix.xpanstep = 0;
	s3c_lcd->fix.ypanstep = 0;
	s3c_lcd->fix.ywrapstep = 0;
	s3c_lcd->fix.line_length = 480*2;
	//2.2���ÿɱ�Ĳ���
	s3c_lcd->var.xres = 480;
	s3c_lcd->var.yres = 272;
	s3c_lcd->var.xres_virtual = 480;
	s3c_lcd->var.yres_virtual = 272;
	s3c_lcd->var.xoffset = 0;
	s3c_lcd->var.yoffset = 0;
	s3c_lcd->var.bits_per_pixel = 16;
	
	s3c_lcd->var.red.offset = 11;
	s3c_lcd->var.red.length = 5;
	s3c_lcd->var.red.msb_right = 0;
	
	s3c_lcd->var.green.offset = 5;
	s3c_lcd->var.green.length = 6;
	s3c_lcd->var.green.msb_right = 0;
	
	s3c_lcd->var.blue.offset = 0;
	s3c_lcd->var.blue.length = 5;
	s3c_lcd->var.blue.msb_right = 0;

	s3c_lcd->var.activate = FB_ACTIVATE_NOW;
	s3c_lcd->var.height = 105;
	s3c_lcd->var.width = 54;

	//ignore some unimportant setting;
	
	//2.3���ò�������
		
	s3c_lcd->fbops = &s3c_ops;
	
	//2.4����������
	//s3c_lcd->screen_base = ; //�Դ�������ַ
	s3c_lcd->screen_size = 480*272*2;
	//s3c_lcd->pseudo_palette = ; //α��ɫ��
	
	//3.Ӳ����صĲ���
	//3.1��������LCD��GPIO
	//3.2����LCD�ֲ�����LCD������
	//3.3����framebuffer�����ѻ���ַ����LCD������
	s3c_lcd->fix.smem_start = 0x0;
	s3c_lcd->fix.smem_len = 480*272*2;
	//4.ע��
	register_framebuffer(s3c_lcd);
	return 0;
}

static void __exit lcd_exit(void)
{

}

module_init(lcd_init);
module_exit(lcd_exit);

MODULE_LICENSE("GPL");
