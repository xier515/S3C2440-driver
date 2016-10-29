#include <setup.h>

static struct tag *params;
void setup_start_tag(void)
{
	params = (struct tag *) 0x30000100;
	params->hdr.tag = ATAG_CORE;
	params->hdr.size = tag_size(tag_core);

	params->u.core.flags = 0;
	params->u.core.pagesize = 0;
	params->u.core.rootdev = 0;

	params = tag_next(params);
}

void setup_memory_tags(void)
{
	params->hdr.tag = ATAG_MEM;
	params->hdr.size = tag_size(tag_mem32);

	params->u.mem = 0x30000000;
	params->u.mem.size = 64*1024*1024;

	params = tag_next(params); 
}
int strlen(char *str)
{
	int i=0;
	while(str[i])
	{
		i++;
	}
}
void strcpy(char *dest, char *src)
{
	char *tmp = dest;
	while((*dest++ = *src++) != '\0');
}
void setup_commandline_tags(char *cmdline)
{
	params->hdr.tag = ATAG_CMDLINE;
	params->hdr.size = (sizeof(struct tag_header) + len + 3) >> 2;

	strcpy(params->u.cmdline.cmdline, cmdline);
	
	params = tag_next(params); 
}

void setup_end_tag(void)
{
	params->hdr.tag = ATAG_NONE;
	params->hdr.size = 0;
}
void main(void)
{
	void (*theKernel)(int zero, int arch, uint params);
	//帮内核设置串口:
	uart0_init();
	puts("copy kernel from nandflash.\n");
	//从NAND FLASH读内核到内存。
	nand_read(0x60000+64, 0x30008000, 0x200000);
	puts("copy kernel done.\n");
	//设置参数
	setup_start_tag();
	setup_memory_tags();
	setup_commandline_tags("noinitrd root=/dev/nfs nfsroot=192.168.3.102:/home/liang/NFS_S3C2440 ip=192.168.3.99:192.168.3.102:192.168.3.1:255.255.255.0:liang:eth0:on init=/linuxrc console=ttySAC0 console=tty1 user_debug=0xff");
	setup_end_tag();
	
	//跳转执行
	theKernel = (void (*)(int, int ,unsigned int params))0x3000800;
	theKernel(0, 362, 0x30000100);
}
