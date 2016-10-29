

#define NFCONF ((volatile unsigned int *)0x4E000000 )

#define NFCONT ((volatile unsigned int *)0x4E000004 )

#define NFCMMD ((volatile unsigned char *)0x4E000008 )

#define NFADDR ((volatile unsigned char *)0x4E00000C )

#define NFDATA ((volatile unsigned char *)0x4E000010 )

#define NFSTAT ((volatile unsigned int *)0x4E000020 )


int isBootFromNorFlash(void)
{
	volatile int *p = (volatile int * )0;
	int val;

	val = *p;
	*p = 0x12345678;
	if(*p == 0x12345678)
	{
		*p = val; //写成功，是nand启动
		return 0;
	}
	else 
	{
		//写失败，是nor启动。nor不能像内存一样写。
		return 1;
	}
}
void copy_code_to_sdram(unsigned int *src, unsigned int *src, unsigned int length)
{
	int i = 0;
	if(isBootFromNorFlash())
	{
		while(i < length)
		{
			*dest = *src;
			dest++;
			src++;
		}
	}
	else
	{
		//nand_init();
		//nand_read(src, src, length;
	}
}

void clean_bss(void)
{
	extern int __bss_start, __bss_end;
	int *p = &__bss_start;

	for(; p < &__bss_end; p++)
	{
		*p = 0;
	}
}

void nand_init(void)
{
#define	TACLS	0
#define TWRPH0	1
#define TWRPH1	0

	NFCONF = (TACLS<<12) | (TWRPH0<<8) | (TWRPH1<<4);

	NFCONT = (1<<4) | (1<<1) | (1<<0);
}


void nand_select(void)
{
	NFCONT &= ~(1<<1);
}

void nand_deselect(void)
{
	NFCONT |= (1<<1);
}
void nand_cmd(unsigned char cmd)
{
	volatile int i;
	NFCMMD = cmd;
	for(i=0; i<10; i++);
}

void nand_addr(unsigned int addr)
{
	unsigned int col = addr>>11;
	unsigned int page = addr>>11;
	volatile int i;

	NFADDR = col & 0xff;
	for(i=0; i<10; i++);
	NFADDR = (col >> 8) & 0xff;
	for(i=0; i<10; i++);
	NFADDR = (page) & 0xff;
	for(i=0; i<10; i++);
	NFADDR = (page >> 8) & 0xff;
	for(i=0; i<10; i++);
	NFADDR = (page >> 16) & 0xff;
	for(i=0; i<10; i++);
}
void nand_wait_ready(void)
{
	while( !(NFSTAT & 1));
}

unsigned char nand_data(void)
{
	return NFDATA;
}
void nand_read(const unsigned int addr, unsigned char *buf, unsigned int len)
{
	int col = src / 2048;
	int i;

	//选中
	nand_select();

	while(i < len)
	{
		//发出读命令00h
		nand_cmd(0x00);

		//发出地址 分5步发
		nand_cmd(addr);
 
		
		//发出读命令30h
		nand_cmd(0x30);
		

		//判断状态
		nand_wait_ready();
		
		
		//读数据
		for(; (col < 2048) && (i < len); col++)
		{
			buf[i] = nand_data();
			i++;
			addr++;
		}
		col = 0;
	}
    //取消选中
	nand_deselect();
}

#define PCLK            50000000    // init.c中的clock_init函数设置PCLK为50MHz
#define UART_CLK        PCLK        //  UART0的时钟源设为PCLK
#define UART_BAUD_RATE  115200      // 波特率
#define UART_BRD        ((UART_CLK  / (UART_BAUD_RATE * 16)) - 1)

/*
 * 初始化UART0
 * 115200,8N1,无流控
 */
void uart0_init(void)
{
    GPHCON  |= 0xa0;    // GPH2,GPH3用作TXD0,RXD0
    GPHUP   = 0x0c;     // GPH2,GPH3内部上拉

    ULCON0  = 0x03;     // 8N1(8个数据位，无较验，1个停止位)
    UCON0   = 0x05;     // 查询方式，UART时钟源为PCLK
    UFCON0  = 0x00;     // 不使用FIFO
    UMCON0  = 0x00;     // 不使用流控
    UBRDIV0 = UART_BRD; // 波特率为115200
}

/*
 * 发送一个字符
 */
void putc(unsigned char c)
{
    /* 等待，直到发送缓冲区中的数据已经全部发送出去 */
    while (!(UTRSTAT0 & TXD0READY));
    
    /* 向UTXH0寄存器中写入数据，UART即自动将它发送出去 */
    UTXH0 = c;
}

void puts(char *str)
{
	int i = 0;
	while (str[i])
	{
		putc(str[i]);
		i++;
	}
}

void puthex(unsigned int val)
{
	/* 0x1234abcd */
	int i;
	int j;
	
	puts("0x");

	for (i = 0; i < 8; i++)
	{
		j = (val >> ((7-i)*4)) & 0xf;
		if ((j >= 0) && (j <= 9))
			putc('0' + j);
		else
			putc('A' + j - 0xa);
		
	}
	
}

