
#define S3C2440_MPLL_200MHZ ((0x5c<<12)|(0x01<<4)|(0x02))
#define MEM_CTL_BASE 0x48000000
.text
.global _start
_start:

/*1.关看门狗*/
	ldr r0, =0x53000000
	mov r1, #0
	str r1, [r0]
/*2.设置时钟*/

	ldr r0, =0x4c000014
	mov r1, #0x03
	str r1, [r0]

	/*存储器异步模式*/
	mrc p15, 0, r1, c1, c0, 0
	orr r1, r1, #0xc0000000
	mcr p15, 0, r1, c1, c0, 0

	ldr r0, =0x4c000004
	ldr r1, =S3C2440_MPLL_200MHZ
	str r1, [r0]
/*3.初始化SDRAM*/
	ldr r0, =MEM_CTL_BASE
	adr r1, sdram_config
	add r3, r0, #(13*4)
	
1: //循环写入SDRAM的配置信息
	ldr r2, [r1], #4
	str r2, [r0], #4
	cmp r0, r3
	bne 1b

	
/*4.重定位:把bootloader本身的代码从flash复制到它的链接地址里去*/
	ldr sp, =0x34000000

	mov r0, #0
	ldr r1, =_start  //链接地址
	ldr r2, =__bss_start
	sub r2, r2, r1
	bl copy_code_to_sdram
	bl clear_bss
/*5.执行main*/
	ldr lr, =halt
	ldr pc, =main
/**/

halt:
	b halt
sdram_config:
	.long 0x22011110
	.long 0x00000700
	.long 0x00000700
	.long 0x00000700
	.long 0x00000700
	.long 0x00000700
	.long 0x00000700
	.long 0x00018005
	.long 0x00018005
	.long 0x008c04f4
	.long 0x000000b1
	.long 0x00000030
	.long 0x00000030





