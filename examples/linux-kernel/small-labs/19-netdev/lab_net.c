// SPDX-License-Identifier: GPL-2.0
/* Two virtual Ethernet interfaces joined back to back, with NAPI receive. */
#include <linux/etherdevice.h>
#include <linux/ethtool.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/rtnetlink.h>
#include <linux/skbuff.h>

#define LAB_RX_LIMIT 256

struct lab_net {
	struct net_device *dev;
	struct net_device *peer;
	struct napi_struct napi;
	struct sk_buff_head rxq;
};

static struct net_device *lab_devs[2];

/* Called by the peer's transmit path: the "interrupt" of this device. */
static bool lab_net_deliver(struct net_device *dev, struct sk_buff *skb)
{
	struct lab_net *priv = netdev_priv(dev);

	if (!netif_running(dev) || skb_queue_len(&priv->rxq) >= LAB_RX_LIMIT)
		return false;

	skb_queue_tail(&priv->rxq, skb);
	napi_schedule(&priv->napi);
	return true;
}

/* Atomic context, must not sleep. Owns skb from the moment it is called. */
static netdev_tx_t lab_net_xmit(struct sk_buff *skb, struct net_device *dev)
{
	struct lab_net *priv = netdev_priv(dev);
	unsigned int len = skb->len;

	/* Drop socket and timestamp state that belongs to the sender. */
	skb_scrub_packet(skb, true);
	skb_tx_timestamp(skb);

	if (lab_net_deliver(priv->peer, skb)) {
		dev_sw_netstats_tx_add(dev, 1, len);
	} else {
		dev_core_stats_tx_dropped_inc(dev);
		dev_kfree_skb_any(skb);
	}
	/* A dropped packet is still NETDEV_TX_OK: it has been consumed. */
	return NETDEV_TX_OK;
}

/* Softirq context. Handle at most budget packets per call. */
static int lab_net_poll(struct napi_struct *napi, int budget)
{
	struct lab_net *priv = container_of(napi, struct lab_net, napi);
	struct sk_buff *skb;
	int done = 0;

	while (done < budget && (skb = skb_dequeue(&priv->rxq))) {
		dev_sw_netstats_rx_add(priv->dev, skb->len);
		skb->protocol = eth_type_trans(skb, priv->dev);
		napi_gro_receive(napi, skb);	/* the stack owns skb now */
		done++;
	}

	if (done < budget && napi_complete_done(napi, done)) {
		/* A packet may have arrived between dequeue and complete. */
		if (!skb_queue_empty(&priv->rxq))
			napi_schedule(napi);
	}
	return done;
}

static int lab_net_open(struct net_device *dev)
{
	struct lab_net *priv = netdev_priv(dev);

	napi_enable(&priv->napi);
	netif_start_queue(dev);
	return 0;
}

static int lab_net_stop(struct net_device *dev)
{
	struct lab_net *priv = netdev_priv(dev);

	netif_stop_queue(dev);
	napi_disable(&priv->napi);	/* waits for a running poll */
	skb_queue_purge(&priv->rxq);
	return 0;
}

static const struct net_device_ops lab_net_ops = {
	.ndo_open = lab_net_open,
	.ndo_stop = lab_net_stop,
	.ndo_start_xmit = lab_net_xmit,
	.ndo_validate_addr = eth_validate_addr,
	.ndo_set_mac_address = eth_mac_addr,
};

static void lab_net_get_drvinfo(struct net_device *dev,
				struct ethtool_drvinfo *info)
{
	strscpy(info->driver, KBUILD_MODNAME, sizeof(info->driver));
}

static const struct ethtool_ops lab_net_ethtool_ops = {
	.get_drvinfo = lab_net_get_drvinfo,
	.get_link = ethtool_op_get_link,
};

static void lab_net_setup(struct net_device *dev)
{
	struct lab_net *priv = netdev_priv(dev);

	ether_setup(dev);
	dev->netdev_ops = &lab_net_ops;
	dev->ethtool_ops = &lab_net_ethtool_ops;
	dev->pcpu_stat_type = NETDEV_PCPU_STAT_TSTATS;
	eth_hw_addr_random(dev);

	priv->dev = dev;
	skb_queue_head_init(&priv->rxq);
	netif_napi_add(dev, &priv->napi, lab_net_poll);
}

static int __init lab_net_init(void)
{
	struct lab_net *priv;
	int i, ret;

	for (i = 0; i < 2; i++) {
		lab_devs[i] = alloc_netdev(sizeof(struct lab_net), "labnet%d",
					   NET_NAME_ENUM, lab_net_setup);
		if (!lab_devs[i]) {
			ret = -ENOMEM;
			goto err_free;
		}
	}

	/* Link the peers before anyone can transmit. */
	for (i = 0; i < 2; i++) {
		priv = netdev_priv(lab_devs[i]);
		priv->peer = lab_devs[!i];
	}

	/*
	 * One RTNL section, so userspace cannot bring up one device while the
	 * other is still unregistered.
	 */
	rtnl_lock();
	ret = register_netdevice(lab_devs[0]);
	if (ret)
		goto err_unlock;
	ret = register_netdevice(lab_devs[1]);
	if (ret) {
		unregister_netdevice(lab_devs[0]);
		goto err_unlock;
	}
	rtnl_unlock();
	return 0;

err_unlock:
	rtnl_unlock();
err_free:
	for (i = 0; i < 2; i++)
		if (lab_devs[i])
			free_netdev(lab_devs[i]);
	return ret;
}

static void __exit lab_net_exit(void)
{
	int i;

	/*
	 * Unregister both before freeing either: unregistering closes the
	 * device and waits until no transmit path can reach the peer.
	 */
	rtnl_lock();
	for (i = 0; i < 2; i++)
		unregister_netdevice(lab_devs[i]);
	rtnl_unlock();

	for (i = 0; i < 2; i++)
		free_netdev(lab_devs[i]);
}

module_init(lab_net_init);
module_exit(lab_net_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Kernel foundations lab 19: virtual netdev pair with NAPI");
