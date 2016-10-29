#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/proc_fs.h>

#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/arch/regs-gpio.h>

static DECLARE_WAIT_QUEUE_HEAD(mymsg_waitq);

#define MYLOG_BUF_LENGTH 1024
static char tmp_buf[MYLOG_BUF_LENGTH];
static char mylog_buf[MYLOG_BUF_LENGTH];
static int mylog_r;
static int mylog_w;


//实现一个环形队列
static int is_empty_mylog(void)
{
	return (mylog_r == mylog_w);
}
static int is_full_mylog(void)
{
	return ((mylog_w+1) % MYLOG_BUF_LENGTH == mylog_r);
}
static void mylog_putc(char c)
{
	if(is_full_mylog())
	{
		//丢弃一个数据
		mylog_r = (mylog_r + 1) % MYLOG_BUF_LENGTH;
	}
	mylog_buf[mylog_w] = c;
	mylog_w = (mylog_w + 1) % MYLOG_BUF_LENGTH;
	wake_up_interruptible(&mymsg_waitq);
}
static int mylog_getc(char *p)
{
	if(is_empty_mylog())
		return 0;

	*p = mylog_buf[mylog_r];
	mylog_r = (mylog_r + 1) % MYLOG_BUF_LENGTH;
	return 1;
}


int myprintf(const char *fmt, ...)
{
	va_list args;
	int i,j;

	va_start(args, fmt);
	i=vsnprintf(tmp_buf, INT_MAX, fmt, args);
	va_end(args);
	for(j=0; j<i; j++)
	{
		mylog_putc(tmp_buf[j]);
	}
	return i;
}

static ssize_t mymsg_read(struct file *file, char __user *buf,
			 size_t count, loff_t *ppos)
{
	int error = 0;
	int i = 0;
	char c;
	if ((file->f_flags & O_NONBLOCK) && is_empty_mylog())
		return -EAGAIN;
	error = wait_event_interruptible(mymsg_waitq,
						!is_empty_mylog());
	
	while(!error && (!is_empty_mylog()) && i<count)
	{
		mylog_getc(&c);
		error = copy_to_user(buf, &c, 1);
		i++;
		buf++; // 被坑
	}

	if(!error)
		error = i;
	return error;
}
const struct file_operations proc_mymsg_operations = {
	.read		= mymsg_read,
//	.poll		= kmsg_poll,
//	.open		= kmsg_open,
//	.release	= kmsg_release,
};

static struct proc_dir_entry *myentry;

static int __init mymsg_init(void)
{
	myentry = create_proc_entry("mymsg", S_IRUSR, &proc_root);
	if (myentry)
		myentry->proc_fops = &proc_mymsg_operations;
	return 0;
}

static void __exit mymsg_exit(void)
{
	remove_proc_entry("mykmsg", &proc_root);
}

module_init(mymsg_init);
module_exit(mymsg_exit);
EXPORT_SYMBOL(myprintf);
MODULE_LICENSE("GPL");

