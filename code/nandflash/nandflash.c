
#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/clk.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/nand_ecc.h>
#include <linux/mtd/partitions.h>

#include <asm/io.h>

#include <asm/arch/regs-nand.h>
#include <asm/arch/nand.h>

struct s3c_nand_regs {
	unsigned long nfconf  ;
	unsigned long nfcont  ;
	unsigned long nfcmd   ;
	unsigned long nfaddr  ;
	unsigned long nfdata  ;
	unsigned long nfeccd0 ;
	unsigned long nfeccd1 ;
	unsigned long nfeccd  ;
	unsigned long nfstat  ;
	unsigned long nfestat0;
	unsigned long nfestat1;
	unsigned long nfmecc0 ;
	unsigned long nfmecc1 ;
	unsigned long nfsecc  ;
	unsigned long nfsblk  ;
	unsigned long nfeblk  ;
};

static struct nand_chip *s3c_nand;
static struct mtd_info *s3c_mtd;
static struct s3c_nand_regs *s3c_nand_regs;

/* NAND parititon from 2.4.18-swl5 */

static struct mtd_partition s3c_nand_partition[] = {
	[0] = {
        .name   = "bootloader",
        .size   = 0x00040000,
		.offset	= 0,
	},
	[1] = {
        .name   = "params",
        .offset = MTDPART_OFS_APPEND,
        .size   = 0x00020000,
	},
	[2] = {
        .name   = "kernel",
        .offset = MTDPART_OFS_APPEND,
        .size   = 0x00200000,
	},
	[3] = {
        .name   = "root",
        .offset = MTDPART_OFS_APPEND,
        .size   = MTDPART_SIZ_FULL,
	}
};

static void s3c2440_select_chip(struct mtd_info *mtd, int chipnr)
{
	if(chipnr == 0)
	{
		//选中  NFCONT[1] = 1
		s3c_nand_regs->nfcont &= ~(1<<1);
	}
	else
	{
		
		//取消选中 NFCONT[1] = 0
		s3c_nand_regs->nfcont |= (1<<1);
	}
}
/*
 * Hardware specific access to control-lines
 */
static void s3c2440_cmd_ctrl(struct mtd_info *mtd, int cmd, unsigned int ctrl)
{
	if (cmd == NAND_CMD_NONE)
		return;

	if (ctrl & NAND_CLE)
	{
		//发命令: NFCMMD=cmd
		s3c_nand_regs->nfcmd = cmd;
	}
	else
	{
		//发地址: NFADDR=cmd
		s3c_nand_regs->nfaddr= cmd;
	}
}

static int s3c2440_dev_ready(struct mtd_info *mtd)
{
	//return NFSTAT[0];
	return (s3c_nand_regs->nfstat & (1<<0));
}
static int s3c_nand_init(void)
{
	struct clk *clk;
	//1.分配一个nand_chip结构体
	s3c_nand = kzalloc(sizeof(struct nand_chip), GFP_KERNEL);

	s3c_nand_regs = ioremap(0x4E000000,sizeof(struct s3c_nand_regs));
	//2.设置nand_chip
	//设置nand_chip是给nand_scan函数使用的，如果不知道怎么设置，先看看nand_scan怎么用它
	//根据之前看芯片手册的经验，应该提供使能片选、发命令、发地址、发数据、读数据、判断状态的功能。
	s3c_nand->select_chip = s3c2440_select_chip;
	s3c_nand->cmd_ctrl = s3c2440_cmd_ctrl;
	s3c_nand->IO_ADDR_R = &s3c_nand_regs->nfdata;//"NFDATA的虚拟地址";
	s3c_nand->IO_ADDR_W = &s3c_nand_regs->nfdata;//"NFDATA的虚拟地址";
	s3c_nand->dev_ready = s3c2440_dev_ready;
	s3c_nand->ecc.mode = NAND_ECC_SOFT;
	//3.硬件相关设置
	//下列参数可以查手册得出。12ns 
	clk = clk_get(NULL, "nand");
	clk_enable(clk); //set CLKCON'bit[4] 
#define TACLS 0
#define TWRPH0 1
#define TWRPH1 0
	s3c_nand_regs->nfconf = (TACLS << 12) | (TWRPH0 << 8) | (TWRPH1 << 4);//时序的配置
	s3c_nand_regs->nfcont = (1 << 1) | (1 << 0);
	//4.使用nand_scan
	s3c_mtd = kzalloc(sizeof(struct mtd_info), GFP_KERNEL);
	s3c_mtd->owner = THIS_MODULE;
	s3c_mtd->priv = s3c_nand;
	nand_scan(s3c_mtd, 1);
	//5.add_mtd_partitions
	add_mtd_partitions(s3c_mtd, s3c_nand_partition, 4);
	return 0;
}

static void s3c_nand_exit(void)
{
	del_mtd_partitions(s3c_mtd);
	kfree(s3c_mtd);
	iounmap(s3c_nand_regs);
	kfree(s3c_nand);
}

module_init(s3c_nand_init);
module_exit(s3c_nand_exit);
MODULE_LICENSE("GPL");



