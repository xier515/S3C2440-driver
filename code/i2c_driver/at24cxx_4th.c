
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/fs.h>
#include <linux/device.h>

static unsigned short ignore[] = { I2C_CLIENT_END };
static unsigned short normal_addr[] = { 0x50, I2C_CLIENT_END };
static unsigned short forces_addr1[] = { ANY_I2C_BUS, 0x60, I2C_CLIENT_END};
static unsigned short *forces[] = {	forces_addr1, NULL, };
static struct i2c_driver at24cxx_driver;

static int major;

static ssize_t at24cxx_read(struct file *file, char __user *buff, size_t size, loff_t * offset)
{
	return 0;
}
static ssize_t at24cxx_write(struct file *file, const char __user *buff, size_t size, loff_t *offset)
{
	return 0;
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
