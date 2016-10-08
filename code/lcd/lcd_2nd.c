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

// LCD CONTROLLER
#define LCDCON1     (*(volatile unsigned long *)0x4d000000) //LCD control 1
#define LCDCON2     (*(volatile unsigned long *)0x4d000004) //LCD control 2
#define LCDCON3     (*(volatile unsigned long *)0x4d000008) //LCD control 3
#define LCDCON4     (*(volatile unsigned long *)0x4d00000c) //LCD control 4
#define LCDCON5     (*(volatile unsigned long *)0x4d000010) //LCD control 5
#define LCDSADDR1   (*(volatile unsigned long *)0x4d000014) //STN/TFT Frame buffer start address 1
#define LCDSADDR2   (*(volatile unsigned long *)0x4d000018) //STN/TFT Frame buffer start address 2
#define LCDSADDR3   (*(volatile unsigned long *)0x4d00001c) //STN/TFT Virtual screen address set
#define REDLUT      (*(volatile unsigned long *)0x4d000020) //STN Red lookup table
#define GREENLUT    (*(volatile unsigned long *)0x4d000024) //STN Green lookup table 
#define BLUELUT     (*(volatile unsigned long *)0x4d000028) //STN Blue lookup table
#define DITHMODE    (*(volatile unsigned long *)0x4d00004c) //STN Dithering mode
#define TPAL        (*(volatile unsigned long *)0x4d000050) //TFT Temporary palette
#define LCDINTPND   (*(volatile unsigned long *)0x4d000054) //LCD Interrupt pending
#define LCDSRCPND   (*(volatile unsigned long *)0x4d000058) //LCD Interrupt source
#define LCDINTMSK   (*(volatile unsigned long *)0x4d00005c) //LCD Interrupt mask
#define LPCSEL      (*(volatile unsigned long *)0x4d000060) //LPC3600 Control
#define PALETTE     (*(volatile unsigned long *)0x4d000400)  //Palette start address

struct lcd_controller_register{
	unsigned long lcdcon1;
	unsigned long lcdcon2;
	unsigned long lcdcon3;
	unsigned long lcdcon4;
	unsigned long lcdcon5;
	unsigned long lcdsaddr1;
	unsigned long lcdsaddr2;
	unsigned long lcdsaddr3;
	unsigned long redlut;
	unsigned long greenlut;
	unsigned long bluelut;
	unsigned long reserve[9];
	unsigned long dithmode;
	unsigned long tpal;
	unsigned long lcdintpnd;
	unsigned long lcdsrcpnd;
	unsigned long lcdintmsk;
	unsigned long lpcsel;
};
static struct fb_info *s3c_lcd;
static struct lcd_controller_register *lcd_regs;
static volatile unsigned int *gpbcon;
static volatile unsigned int *gpbdat;
static volatile unsigned int *gpccon;
static volatile unsigned int *gpdcon;
static volatile unsigned int *gpgcon;
static volatile unsigned int *gpgdat;

static unsigned int pseudo_palette[16];

static inline u_int chan_to_field(u_int chan, struct fb_bitfield *bf)
{
	chan &= 0xffff;
	chan >>= 16 - bf->length;
	return chan << bf->offset;
}
static int s3c_lcdfb_setcolreg(unsigned int regno, unsigned int red,
			     unsigned int green, unsigned int blue,
			     unsigned int transp, struct fb_info *info)
{
	unsigned int val;
	if(regno>16) return -1;
	val  = chan_to_field(red, &info->var.red);
	val |= chan_to_field(green, &info->var.green);
	val |= chan_to_field(blue, &info->var.blue);
	pseudo_palette[regno] = val; 
	return 0;
}
static struct fb_ops s3c_ops = {
	.owner		= THIS_MODULE,
	.fb_setcolreg	= s3c_lcdfb_setcolreg,
	.fb_fillrect	= cfb_fillrect,
	.fb_copyarea	= cfb_copyarea,
	.fb_imageblit	= cfb_imageblit,
};


static int __init lcd_init(void)
{
	//1.分配一个fb_info
	s3c_lcd = framebuffer_alloc(0, NULL);
	//2.设置
	//2.1设置固定的参数
	strcpy(s3c_lcd->fix.id, "mylcd");
	s3c_lcd->fix.smem_len = 480*272*16;
	s3c_lcd->fix.type = FB_TYPE_PACKED_PIXELS;
	s3c_lcd->fix.visual = FB_VISUAL_TRUECOLOR;
	s3c_lcd->fix.xpanstep = 0;
	s3c_lcd->fix.ypanstep = 0;
	s3c_lcd->fix.ywrapstep = 0;
	s3c_lcd->fix.line_length = 480*2;
	//2.2设置可变的参数
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
	
	//2.3设置操作函数
		
	s3c_lcd->fbops = &s3c_ops;
	
	//2.4其他的设置
	s3c_lcd->screen_size = 480*272*2;
	s3c_lcd->pseudo_palette = pseudo_palette; //伪调色板
	
	//3.硬件相关的操作
	//3.1配置用于LCD的GPIO
	gpbcon = ioremap(0x56000010, 8);
	gpbdat = gpbcon + 1;
	gpccon = ioremap(0x56000020, 8);
	gpdcon = ioremap(0x56000030, 8);
	gpgcon = ioremap(0x56000060, 8);
	gpgdat = gpgcon + 1;
	
	*gpccon = 0xaaaaaaaa;
	*gpdcon = 0xaaaaaaaa;
	
	*gpbcon &= ~(0x1); //backlight
	*gpbcon |= 1;
	*gpbdat &= ~1;

	*gpgcon |= (0x3)<<8; //LCD电源使能  
	
	//3.2根据LCD手册配置LCD控制器
	lcd_regs = ioremap(0x4D000000, sizeof(struct lcd_controller_register));
	lcd_regs->lcdcon1 = (4<<8) | (0x03<<5) | (0x0c<<1);
	lcd_regs->lcdcon2 = (1<<24) | (271<<14) | (1<<6) | (9);
	lcd_regs->lcdcon3 = (1<<19) | (479<<8) | (1);
	lcd_regs->lcdcon4 = 40;
	lcd_regs->lcdcon5 = (1<<11) | (0<<9) | (0<<8) | (1<<0) ;

	//3.3分配framebuffer，并把基地址告诉LCD控制器
	s3c_lcd->screen_base = (char *)dma_alloc_writecombine(NULL, s3c_lcd->screen_size, &s3c_lcd->fix.smem_start, GFP_KERNEL);
	s3c_lcd->fix.smem_len = 480*272*2; 

	lcd_regs->lcdsaddr1 = (s3c_lcd->fix.smem_start >> 1) & ~(3<<30);
	lcd_regs->lcdsaddr2 = ((s3c_lcd->fix.smem_start + s3c_lcd->fix.smem_len) >> 1) & 0x1fffff;
	lcd_regs->lcdsaddr3 = 480;
	lcd_regs->lcdcon5 |=  (1<<3);
	
	lcd_regs->lcdcon1 |= 1;//启动LCD控制器
	*gpbdat |= 1;//启动背光
	//4.注册
	register_framebuffer(s3c_lcd);
	return 0;
}

static void __exit lcd_exit(void)
{
	lcd_regs->lcdcon1 &= ~1;//关闭LCD
	*gpbdat &= ~1;//关闭背光
	unregister_framebuffer(s3c_lcd);
	dma_free_writecombine(NULL, s3c_lcd->screen_size, &s3c_lcd->fix.smem_start, GFP_KERNEL);
	iounmap(lcd_regs);
	iounmap(gpbcon);
	iounmap(gpccon);
	iounmap(gpdcon);
	iounmap(gpgcon);
	framebuffer_release(s3c_lcd);
}

module_init(lcd_init);
module_exit(lcd_exit);

MODULE_LICENSE("GPL");
