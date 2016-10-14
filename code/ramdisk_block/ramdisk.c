#include <linux/module.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/timer.h>
#include <linux/genhd.h>
#include <linux/hdreg.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/wait.h>
#include <linux/blkdev.h>
#include <linux/blkpg.h>
#include <linux/delay.h>
#include <linux/io.h>

#include <asm/system.h>
#include <asm/uaccess.h>
#include <asm/dma.h>

#define RAMDISK_SIZE 1024*1024
static DEFINE_SPINLOCK(ramdisk_lock);

static int ramdisk_getgeo(struct block_device *bdev, struct hd_geometry *geo)
{
	/* ����=heads*cylinders*sectors*512 */
	geo->heads     = 2;
	geo->cylinders = 32;
	geo->sectors   = RAMDISK_SIZE/2/32/512;
	return 0;
}

static struct block_device_operations ramdisk_fops = {
	.owner = THIS_MODULE,
	.getgeo = ramdisk_getgeo,
};
static struct gendisk *ramdisk;
static struct request_queue *ramdisk_queue;

static int major;
static unsigned char *ramdisk_buf;


static void do_ramdisk_request(request_queue_t * q)
{
	static int cnt = 0;
	struct request *req;
	
	//printk("do_ramdisk_request %d.\n", ++cnt);
	while((req = elv_next_request(q)) != NULL)
	{
		//���ݴ�����Ҫ�� Դ��Ŀ�ġ�����
		unsigned long offset = req->sector << 9;
		//req->buffer
		unsigned long len = req->current_nr_sectors << 9;

		if(rq_data_dir(req) == READ)
		{
			memcpy(req->buffer, ramdisk_buf + offset, len);
		}
		else
		{
			memcpy(ramdisk_buf + offset, req->buffer, len);
		}
		end_request(req, 1);
	}
}


static int ramdisk_init(void)
{
	//1.����һ��gendisk�ṹ��
	ramdisk = alloc_disk(16); //minors ���豸�Ÿ�����Ҳ���Ƿ������� 0��ʾ�������� 
	//2.����
	//2.1 ���䡢���ö��У��ṩ��д����
	ramdisk_queue = blk_init_queue(do_ramdisk_request, &ramdisk_lock);
	ramdisk->queue = ramdisk_queue;
	//2.2 �����������ԣ���������
	major = register_blkdev(0, "ramdisk");
	sprintf(ramdisk->disk_name, "ramdisk");
	ramdisk->major = major;
	ramdisk->first_minor= 0;
	ramdisk->fops = &ramdisk_fops;
	set_capacity(ramdisk, RAMDISK_SIZE/512);
	//3.Ӳ����ز���
	ramdisk_buf = kzalloc(RAMDISK_SIZE, GFP_KERNEL);
	//4.ע��
	add_disk(ramdisk);
	return 0;
}

static void ramdisk_exit(void)
{
	unregister_blkdev(major, "ramdisk");
	del_gendisk(ramdisk);
	put_disk(ramdisk);
	blk_cleanup_queue(ramdisk_queue); 

	kfree(ramdisk_buf);
}

module_init(ramdisk_init);
module_exit(ramdisk_exit);
MODULE_LICENSE("GPL");
