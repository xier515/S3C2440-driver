

/* Always include 'config.h' first in case the user wants to turn on
   or override something. */
#include <linux/module.h>


#include <linux/errno.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/in.h>
#include <linux/skbuff.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/init.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/ip.h>

#include <asm/system.h>
#include <asm/io.h>
#include <asm/irq.h>

static struct net_device *vnet_dev;
static void emulator_rx_packet(struct sk_buff *skb, struct net_device *dev)
{
	//参考LDD3.
	unsigned char *type;
	//char *data;
	//int len;
	struct iphdr *ih;
	//struct net_device *dest;
	__be32 *saddr, *daddr, tmp;
	unsigned char tmp_dev_addr[ETH_ALEN];
	struct ethhdr *ethhdr; //数据包的源MAC、目的MAC信息

	struct sk_buff *rx_skb;

	//对源、目的 的MAC地址
	ethhdr = (struct ethhdr *)skb->data;
	memcpy(tmp_dev_addr, ethhdr->h_dest, ETH_ALEN);
	memcpy(ethhdr->h_dest, ethhdr->h_source, ETH_ALEN);
	memcpy(ethhdr->h_source, tmp_dev_addr, ETH_ALEN);

	//对调源、目的 的IP地址
	ih = (struct iphdr *)(skb->data + sizeof(struct ethhdr));
	saddr = &ih->saddr;
	daddr = &ih->daddr;

	tmp = *saddr;
	*saddr = *daddr;
	*daddr = tmp;

	//修改类型，原来0x8表示PING包。修改为0:reply
	type = skb->data + sizeof(struct ethhdr) + sizeof(struct iphdr);
	*type = 0;

	//重新计算校验码
	ih->check = 0;
	ih->check = ip_fast_csum((unsigned char *)ih, ih->ihl);

	//构造一个sk_skbuff
	rx_skb = dev_alloc_skb(skb->len + 2);
	skb_reserve(rx_skb, 2);
	memcpy(skb_put(rx_skb, skb->len), skb->data, skb->len);

	
	rx_skb->dev = dev;
	rx_skb->protocol = eth_type_trans(rx_skb, dev);
	rx_skb->ip_summed = CHECKSUM_UNNECESSARY;

	//更新统计信息
	dev->stats.rx_packets++;
	dev->stats.rx_bytes += rx_skb->len;

	//提交sk_buff
	netif_rx(rx_skb);
}
static int vnet_hard_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
	static int cnt;
	printk("vnet_hard_start_xmit cnt = %d \n", ++cnt);

	//对于真实的网卡，应该把skb里的数据发送出去。
	netif_stop_queue(dev);	//停止该网卡的队列
	//..........			//把skb的数据写入网卡
 
	//构造一个假的sk_buff上报
	emulator_rx_packet(skb, dev);
	
	dev_kfree_skb(skb);		//释放skb
	netif_wake_queue(dev);	//唤醒网卡的队列 应该放在网卡发送结束中断里。
	
	//更新统计信息
	dev->stats.tx_packets++;
	dev->stats.tx_bytes += skb->len;


	return 0;
}

static int __init virtual_net_init(void)
{
	//1.分配一个net_device结构体
	vnet_dev = alloc_netdev(0, "vnet%d", ether_setup);
	//2.设置
	vnet_dev->hard_start_xmit = vnet_hard_start_xmit;

	//设置MAC地址
	vnet_dev->dev_addr[0] = 0x08;
	vnet_dev->dev_addr[1] = 0x89;
	vnet_dev->dev_addr[2] = 0x89;
	vnet_dev->dev_addr[3] = 0x89;
	vnet_dev->dev_addr[4] = 0x88;
	vnet_dev->dev_addr[5] = 0x11;

	vnet_dev->flags |= IFF_NOARP;
	vnet_dev->features |= NETIF_F_NO_CSUM;
	//3.注册
	//register_netdevice(vnet_dev);
	register_netdev(vnet_dev);
	return 0;
}

static void __exit virtual_net_exit(void)
{
	//unregister_netdevice(vnet_dev);
	unregister_netdev(vnet_dev);
	free_netdev(vnet_dev);
}

module_init(virtual_net_init);
module_exit(virtual_net_exit);
MODULE_AUTHOR("Liang,xier515@live.com");
MODULE_LICENSE("GPL");

