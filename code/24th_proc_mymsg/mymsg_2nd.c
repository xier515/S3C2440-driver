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


static struct proc_dir_entry *myentry;

static ssize_t mymsg_read(struct file *file, char __user *buf,
			 size_t count, loff_t *ppos)
{

	return 0;
}
const struct file_operations proc_mymsg_operations = {
	.read		= mymsg_read,
//	.poll		= kmsg_poll,
//	.open		= kmsg_open,
//	.release	= kmsg_release,
};


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

MODULE_LICENSE("GPL");

