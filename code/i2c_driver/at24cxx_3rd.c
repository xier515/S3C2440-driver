
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/mutex.h>

static unsigned short ignore[] = { I2C_CLIENT_END };
static unsigned short normal_addr[] = { 0x50, I2C_CLIENT_END };
static unsigned short forces_addr1[] = { ANY_I2C_BUS, 0x60, I2C_CLIENT_END};
static unsigned short *forces[] = {	forces_addr1, NULL, };
static struct i2c_driver at24cxx_driver;

static struct i2c_client_address_data addr_data = {
	.normal_i2c = ignore, //Ҫ����ַ�źŲ���ȷ���Ƿ��������豸
	.probe = ignore,
	.ignore = ignore,
	.forces = forces,//ǿ����Ϊ��������豸��
};

static int at24cxx_detect(struct i2c_adapter *adapter, int address, int kind)
{
	struct i2c_client *new_client;

	new_client = kzalloc(sizeof(struct i2c_client), GFP_KERNEL);
	//����һ��i2c_client�ṹ�壬 �շ����ݵ�ʱ��Ҫʹ����
	new_client->addr = address;//�豸�ĵ�ַ
	new_client->adapter = adapter;//ָ��I2C������
	new_client->driver = &at24cxx_driver;//ָ���豸����
	strcpy(new_client->name, "at24cxx");
	
	i2c_attach_client(new_client);
	
	printk("at24cxx_detect\n");
	return 0;
}
static int at24cxx_attach(struct i2c_adapter *adapter)
{
	return i2c_probe(adapter, &addr_data, at24cxx_detect);
}
static int at24cxx_detach(struct i2c_client *client)
{
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
