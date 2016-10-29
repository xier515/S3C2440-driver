
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/fs.h>
#include <linux/device.h>

#include <asm/uaccess.h>
#include <asm/io.h>

static unsigned short ignore[] = { I2C_CLIENT_END };
static unsigned short normal_addr[] = { 0x50, I2C_CLIENT_END };
static unsigned short forces_addr1[] = { ANY_I2C_BUS, 0x60, I2C_CLIENT_END};
static unsigned short *forces[] = {	forces_addr1, NULL, };
static struct i2c_driver at24cxx_driver;
static struct i2c_client *at24cxx_client;
static int major;

static ssize_t at24cxx_read(struct file *file, char __user *buf, size_t size, loff_t * offset)
{
	unsigned char address;
	unsigned char data;
	struct i2c_msg msg[2];
	int ret;
	//address = buf[0]
	//data    = buf[1]

	copy_from_user(&address, buf, 1);
	//数据传输三要素 读之前要先把要读的地址写给芯片
	msg[0].addr = at24cxx_client->addr;//目的
	msg[0].buf = &address;//源
	msg[0].len = 1;//长度
	msg[0].flags = 0; //表示写

	//数据传输三要素 启动读操作
	msg[0].addr = at24cxx_client->addr;//源
	msg[0].buf = &data;//目的
	msg[0].len = 1;//长度
	msg[0].flags = I2C_M_RD; //表示读
	
	ret = i2c_transfer(at24cxx_client->adapter, msg, 2);
	if(ret == 2)
	{
		copy_to_user(buf+1, &data, 1);
		return 2;
	}
	else
		return -EIO;
}
static ssize_t at24cxx_write(struct file *file, const char __user *buf, size_t size, loff_t *offset)
{
	unsigned char val[2];
	struct i2c_msg msg[1];
	int ret;
	//address = buf[0]
	//data    = buf[1]

	copy_from_user(val, buf, 2);
	//数据传输三要素
	msg[0].addr = at24cxx_client->addr;//目的
	msg[0].buf = val;//源
	msg[0].len = 2;//长度
	msg[0].flags = 0; //表示写
	
	ret = i2c_transfer(at24cxx_client->adapter, msg, 1);
	if(ret == 1)
		return 2;
	else
		return -EIO;
}
	
static struct file_operations at24cxx_fops = {
	.owner = THIS_MODULE,
	.read = at24cxx_read,
	.write = at24cxx_write,
};

static struct class *cls;

static struct i2c_client_address_data addr_data = {
	.normal_i2c = ignore, //要发地址信号才能确定是否存在这个设备
	.probe = ignore,
	.ignore = ignore,
	.forces = forces,//强制认为存在这个设备　
};

static int at24cxx_detect(struct i2c_adapter *adapter, int address, int kind)
{
	struct i2c_client *new_client;

	new_client = kzalloc(sizeof(struct i2c_client), GFP_KERNEL);
	//构造一个i2c_client结构体， 收发数据的时候要使用它
	new_client->addr = address;//设备的地址
	new_client->adapter = adapter;//指向I2C适配器
	new_client->driver = &at24cxx_driver;//指向设备驱动
	strcpy(new_client->name, "at24cxx");
	
	i2c_attach_client(new_client);
	at24cxx_client = new_client;
	major = register_chrdev(0, "at24cxx", &at24cxx_fops);
	cls = class_create(THIS_MODULE, "at24cxx");
	class_device_create(cls, NULL, MKDEV(major, 0), NULL, "at24cxx");
	
	printk("at24cxx_detect\n");
	return 0;
}
static int at24cxx_attach(struct i2c_adapter *adapter)
{
	return i2c_probe(adapter, &addr_data, at24cxx_detect);
}
static int at24cxx_detach(struct i2c_client *client)
{
	class_device_destroy(cls, MKDEV(major, 0));
	class_destroy(cls);
	unregister_chrdev(major, "at24cxx");
	i2c_detach_client(client);
	kfree(i2c_get_clientdata(client));
	printk("at24cxx_detach\n");
	return 0;
}


static struct i2c_driver at24cxx_driver = {
	.driver = {
		.name = "at24cxx",
	},
	.attach_adapter = at24cxx_attach,
	.detach_client = at24cxx_detach,
};

static int __init at24cxx_init(void)
{
	i2c_add_driver(&at24cxx_driver);
	return 0;
}

static void __exit at24cxx_exit(void)
{
	i2c_del_driver(&at24cxx_driver);
}

module_init(at24cxx_init);
module_exit(at24cxx_exit);
MODULE_LICENSE("GPL");
