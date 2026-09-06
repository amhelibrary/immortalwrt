// SPDX-License-Identifier: GPL-2.0
/*
* HNAT mainline-style flowtable front-end.
*
* Keep the HNAT hardware FOE path, but accept nftables/tc flowtable
* (flow_cls_offload) rules like the mainline mtk_ppe driver.  This avoids
* hard-coding LAN/WAN/PPD/ext interface roles and lets the netfilter
* flowtable drive the original/reply directions.
*/

#include <linux/etherdevice.h>
#include <linux/if_ether.h>
#include <linux/if_vlan.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <linux/list.h>
#include <linux/netdevice.h>
#include <linux/spinlock.h>
#include <linux/string.h>

#include <net/dsa.h>
#include <net/flow_offload.h>
#include <net/ip.h>
#include <net/ipv6.h>
#include <net/netfilter/nf_flow_table.h>
#include <net/pkt_cls.h>

#include "hnat.h"
#include "hnat_api.h"
#include "nf_hnat_mtk.h"
#include "../mtk_eth_soc.h"

struct hnat_flow_offload_entry {
	struct list_head list;
	unsigned long cookie;
	u16 ppe_index;
	u16 hash;
	struct foe_entry data;
	bool bound;
};

static LIST_HEAD(hnat_flow_offload_list);
static DEFINE_SPINLOCK(hnat_flow_offload_lock);

struct hnat_flow_data {
	struct ethhdr eth;
	union {
		struct {
			__be32 src_addr;
			__be32 dst_addr;
		} v4;
	struct {
		struct in6_addr src_addr;
		struct in6_addr dst_addr;
	} v6;
};
__be16 src_port;
__be16 dst_port;
u8 dscp;
u8 ttl;
};

static struct hnat_flow_offload_entry *
hnat_flow_offload_find_by_cookie_locked(unsigned long cookie)
{
	struct hnat_flow_offload_entry *fe;

	list_for_each_entry(fe, &hnat_flow_offload_list, list) {
		if (fe->cookie == cookie)
		return fe;
	}

return NULL;
}

bool hnat_flow_offload_get_template(u32 ppe_id, u32 hash,
const struct foe_entry *hwe,
struct foe_entry *dst)
{
	struct hnat_flow_offload_entry *fe;
	bool found = false;

	if (!dst || !hwe || ppe_id >= CFG_PPE_NUM ||
	hash >= hnat_priv->foe_etry_num)
	return false;

	spin_lock_bh(&hnat_flow_offload_lock);
	list_for_each_entry(fe, &hnat_flow_offload_list, list) {
		if (fe->ppe_index != ppe_id || fe->hash != hash)
		continue;

		if (!hnat_flow_entry_match(&fe->data, (struct foe_entry *)hwe))
		continue;

		memcpy(dst, &fe->data, sizeof(*dst));
		fe->bound = true;
		found = true;
		break;
	}
spin_unlock_bh(&hnat_flow_offload_lock);

return found;
}

static void hnat_flow_offload_mangle_eth(const struct flow_action_entry *act,
void *eth)
{
	void *dest = eth + act->mangle.offset;
	const void *src = &act->mangle.val;

	if (act->mangle.offset > 8)
	return;

	if (act->mangle.mask == 0xffff) {
		src += 2;
		dest += 2;
	}

memcpy(dest, src, act->mangle.mask ? 2 : 4);
}

static int hnat_flow_mangle_ports(const struct flow_action_entry *act,
struct hnat_flow_data *data)
{
	u32 val = ntohl(act->mangle.val);

	switch (act->mangle.offset) {
		case 0:
		if (act->mangle.mask == ~htonl(0xffff))
		data->dst_port = cpu_to_be16(val);
		else
		data->src_port = cpu_to_be16(val >> 16);
		break;
		case 2:
		data->dst_port = cpu_to_be16(val);
		break;
		default:
		return -EINVAL;
	}

return 0;
}

static int hnat_flow_mangle_ipv4(const struct flow_action_entry *act,
struct hnat_flow_data *data)
{
	__be32 *dest;

	switch (act->mangle.offset) {
		case offsetof(struct iphdr, saddr):
		dest = &data->v4.src_addr;
		break;
		case offsetof(struct iphdr, daddr):
		dest = &data->v4.dst_addr;
		break;
		default:
		return -EINVAL;
	}

memcpy(dest, &act->mangle.val, sizeof(u32));

return 0;
}

static struct net_device *
hnat_flow_offload_to_mtk_dev(struct mtk_eth *eth, struct net_device *dev)
{
	struct net_device *conduit = dev;
	int i;

	if (!dev)
	return NULL;

	if (netdev_uses_dsa(dev) && hnat_dsa_get_port(&conduit) >= 0)
	dev = conduit;

	for (i = 0; i < ARRAY_SIZE(eth->netdev); i++) {
		if (eth->netdev[i] && dev->netdev_ops == eth->netdev[i]->netdev_ops)
		return eth->netdev[i];
	}

return NULL;
}

static int hnat_flow_offload_get_ppe_index(struct mtk_eth *eth,
struct net_device *idev)
{
	struct mtk_mac *mac;

	idev = hnat_flow_offload_to_mtk_dev(eth, idev);
	if (!idev)
	return -EINVAL;

	mac = netdev_priv(idev);
	if (WARN_ON(mac->ppe_idx >= eth->soc->ppe_num))
	return -EINVAL;

	return mac->ppe_idx;
}

static void hnat_flow_set_mac(struct foe_entry *foe, u8 *src, u8 *dst)
{
	if (IS_IPV4_GRP(foe)) {
		foe->ipv4_hnapt.dmac_hi = swab32(*(u32 *)dst);
		foe->ipv4_hnapt.dmac_lo = swab16(*(u16 *)(dst + 4));
		foe->ipv4_hnapt.smac_hi = swab32(*(u32 *)src);
		foe->ipv4_hnapt.smac_lo = swab16(*(u16 *)(src + 4));
	} else {
	foe->ipv6_5t_route.dmac_hi = swab32(*(u32 *)dst);
	foe->ipv6_5t_route.dmac_lo = swab16(*(u16 *)(dst + 4));
	foe->ipv6_5t_route.smac_hi = swab32(*(u32 *)src);
	foe->ipv6_5t_route.smac_lo = swab16(*(u16 *)(src + 4));
}
}

static int hnat_flow_offload_prepare(struct foe_entry *foe, int type,
int l4proto, u8 *src_mac, u8 *dst_mac)
{
	memset(foe, 0, sizeof(*foe));

	foe->bfib1.pkt_type = type;
	foe->bfib1.state = UNBIND;
	foe->bfib1.udp = (l4proto == IPPROTO_UDP) ? 1 : 0;
	foe->bfib1.cah = 1;
	foe->bfib1.ttl = 1;

	if (type == IPV4_HNAPT || type == IPV4_HNAT) {
		foe->ipv4_hnapt.sp_tag = htons(ETH_P_IP);
		foe->ipv4_hnapt.iblk2.port_ag = 0xf;
		if (hnat_priv->data->per_flow_accounting)
		foe->ipv4_hnapt.iblk2.mibf = 1;
	} else if (type == IPV6_5T_ROUTE || type == IPV6_3T_ROUTE) {
	foe->ipv6_5t_route.sp_tag = htons(ETH_P_IPV6);
	foe->ipv6_5t_route.iblk2.port_ag = 0xf;
	if (hnat_priv->data->per_flow_accounting)
	foe->ipv6_5t_route.iblk2.mibf = 1;
} else {
return -EOPNOTSUPP;
}

hnat_flow_set_mac(foe, src_mac, dst_mac);

return 0;
}

static void hnat_flow_set_ipv4_tuple(struct foe_entry *foe, bool egress,
__be32 src_addr, __be16 src_port,
__be32 dst_addr, __be16 dst_port)
{
	if (egress) {
		foe->ipv4_hnapt.new_sip = be32_to_cpu(src_addr);
		foe->ipv4_hnapt.new_dip = be32_to_cpu(dst_addr);
		foe->ipv4_hnapt.new_sport = be16_to_cpu(src_port);
		foe->ipv4_hnapt.new_dport = be16_to_cpu(dst_port);
	} else {
	foe->ipv4_hnapt.sip = be32_to_cpu(src_addr);
	foe->ipv4_hnapt.dip = be32_to_cpu(dst_addr);
	foe->ipv4_hnapt.sport = be16_to_cpu(src_port);
	foe->ipv4_hnapt.dport = be16_to_cpu(dst_port);
}
}

static void hnat_flow_set_ipv6_tuple(struct foe_entry *foe,
__be32 *src_addr, __be16 src_port,
__be32 *dst_addr, __be16 dst_port)
{
	foe->ipv6_5t_route.ipv6_sip0 = be32_to_cpu(src_addr[0]);
	foe->ipv6_5t_route.ipv6_sip1 = be32_to_cpu(src_addr[1]);
	foe->ipv6_5t_route.ipv6_sip2 = be32_to_cpu(src_addr[2]);
	foe->ipv6_5t_route.ipv6_sip3 = be32_to_cpu(src_addr[3]);
	foe->ipv6_5t_route.ipv6_dip0 = be32_to_cpu(dst_addr[0]);
	foe->ipv6_5t_route.ipv6_dip1 = be32_to_cpu(dst_addr[1]);
	foe->ipv6_5t_route.ipv6_dip2 = be32_to_cpu(dst_addr[2]);
	foe->ipv6_5t_route.ipv6_dip3 = be32_to_cpu(dst_addr[3]);
	foe->ipv6_5t_route.sport = be16_to_cpu(src_port);
	foe->ipv6_5t_route.dport = be16_to_cpu(dst_port);
}

static int hnat_flow_offload_set_output(struct mtk_eth *eth,
struct net_device *odev,
struct foe_entry *foe)
{
	struct net_device *conduit;
	struct mtk_mac *mac;
	int gmac;

	if (!odev)
	return -EINVAL;

	conduit = hnat_flow_offload_to_mtk_dev(eth, odev);
	if (!conduit)
	return -EOPNOTSUPP;

	mac = netdev_priv(conduit);
	gmac = HNAT_GMAC_FP(mac->id);
	if (gmac < 0)
	return -EOPNOTSUPP;

	if (IS_IPV4_GRP(foe))
	foe->ipv4_hnapt.iblk2.dp = gmac;
	else
	foe->ipv6_5t_route.iblk2.dp = gmac;

	return 0;
}

static int hnat_flow_offload_replace(struct mtk_eth *eth,
struct flow_cls_offload *f)
{
	struct flow_rule *rule = flow_cls_offload_flow_rule(f);
	struct net_device *idev = NULL, *odev = NULL;
	struct flow_action_entry *act;
	struct hnat_flow_offload_entry *fe, *old;
	struct foe_entry foe = {};
	struct hnat_flow_data data = {};
	unsigned long cookie = f->cookie;
	int ppe_index;
	int offload_type = 0;
	u16 addr_type = 0;
	u8 l4proto = 0;
	int err = 0;
	int i;

	if (flow_rule_match_key(rule, FLOW_DISSECTOR_KEY_META)) {
		struct flow_match_meta match;

		flow_rule_match_meta(rule, &match);
		idev = __dev_get_by_index(&init_net, match.key->ingress_ifindex);
		ppe_index = hnat_flow_offload_get_ppe_index(eth, idev);
		if (ppe_index < 0)
		return -EOPNOTSUPP;
	} else {
	return -EOPNOTSUPP;
}

if (flow_rule_match_key(rule, FLOW_DISSECTOR_KEY_CONTROL)) {
	struct flow_match_control match;

	flow_rule_match_control(rule, &match);
	addr_type = match.key->addr_type;
} else {
return -EOPNOTSUPP;
}

if (flow_rule_match_key(rule, FLOW_DISSECTOR_KEY_BASIC)) {
	struct flow_match_basic match;

	flow_rule_match_basic(rule, &match);
	l4proto = match.key->ip_proto;
} else {
return -EOPNOTSUPP;
}

if (flow_rule_match_key(rule, FLOW_DISSECTOR_KEY_IP)) {
	struct flow_match_ip match;

	flow_rule_match_ip(rule, &match);
	data.dscp = match.key->tos;
	data.ttl = match.key->ttl;
}

switch (addr_type) {
	case FLOW_DISSECTOR_KEY_IPV4_ADDRS:
	offload_type = IPV4_HNAPT;
	break;
	case FLOW_DISSECTOR_KEY_IPV6_ADDRS:
	offload_type = IPV6_5T_ROUTE;
	break;
	default:
	return -EOPNOTSUPP;
}

flow_action_for_each(i, act, &rule->action) {
	switch (act->id) {
		case FLOW_ACTION_MANGLE:
		if (act->mangle.htype == FLOW_ACT_MANGLE_HDR_TYPE_ETH)
		hnat_flow_offload_mangle_eth(act, &data.eth);
		break;
		case FLOW_ACTION_REDIRECT:
		odev = act->dev;
		break;
		case FLOW_ACTION_CSUM:
		break;
		case FLOW_ACTION_VLAN_PUSH:
		case FLOW_ACTION_VLAN_POP:
		case FLOW_ACTION_PPPOE_PUSH:
		case FLOW_ACTION_IPSEC_PUSH:
		case FLOW_ACTION_NPU_ENCAP:
		/* Not implemented in this minimal flowtable front-end. */
		return -EOPNOTSUPP;
		default:
		return -EOPNOTSUPP;
	}
}

if (!is_valid_ether_addr(data.eth.h_source) ||
!is_valid_ether_addr(data.eth.h_dest))
return -EINVAL;

err = hnat_flow_offload_prepare(&foe, offload_type, l4proto,
data.eth.h_source, data.eth.h_dest);
if (err)
return err;

if (flow_rule_match_key(rule, FLOW_DISSECTOR_KEY_PORTS)) {
	struct flow_match_ports match;

	flow_rule_match_ports(rule, &match);
	data.src_port = match.key->src;
	data.dst_port = match.key->dst;
} else {
return -EOPNOTSUPP;
}

if (addr_type == FLOW_DISSECTOR_KEY_IPV4_ADDRS) {
	struct flow_match_ipv4_addrs addrs;

	flow_rule_match_ipv4_addrs(rule, &addrs);
	data.v4.src_addr = addrs.key->src;
	data.v4.dst_addr = addrs.key->dst;
	hnat_flow_set_ipv4_tuple(&foe, false, data.v4.src_addr,
	data.src_port, data.v4.dst_addr,
	data.dst_port);
} else {
struct flow_match_ipv6_addrs addrs;

flow_rule_match_ipv6_addrs(rule, &addrs);
data.v6.src_addr = addrs.key->src;
data.v6.dst_addr = addrs.key->dst;
hnat_flow_set_ipv6_tuple(&foe, data.v6.src_addr.s6_addr32,
data.src_port,
data.v6.dst_addr.s6_addr32,
data.dst_port);
}

flow_action_for_each(i, act, &rule->action) {
	if (act->id != FLOW_ACTION_MANGLE)
	continue;

	switch (act->mangle.htype) {
		case FLOW_ACT_MANGLE_HDR_TYPE_TCP:
		case FLOW_ACT_MANGLE_HDR_TYPE_UDP:
		err = hnat_flow_mangle_ports(act, &data);
		break;
		case FLOW_ACT_MANGLE_HDR_TYPE_IP4:
		err = hnat_flow_mangle_ipv4(act, &data);
		break;
		case FLOW_ACT_MANGLE_HDR_TYPE_ETH:
		break;
		default:
		return -EOPNOTSUPP;
	}

if (err)
return err;
}

if (addr_type == FLOW_DISSECTOR_KEY_IPV4_ADDRS)
hnat_flow_set_ipv4_tuple(&foe, true, data.v4.src_addr,
data.src_port, data.v4.dst_addr,
data.dst_port);

if (IS_IPV4_GRP(&foe))
foe.ipv4_hnapt.iblk2.dscp = data.dscp;
else
foe.ipv6_5t_route.iblk2.dscp = data.dscp;

err = hnat_flow_offload_set_output(eth, odev, &foe);
if (err)
return err;

spin_lock_bh(&hnat_flow_offload_lock);
old = hnat_flow_offload_find_by_cookie_locked(cookie);
if (old) {
	list_del(&old->list);
	spin_unlock_bh(&hnat_flow_offload_lock);
	kfree(old);
	spin_lock_bh(&hnat_flow_offload_lock);
}

fe = kzalloc(sizeof(*fe), GFP_ATOMIC);
if (!fe) {
	spin_unlock_bh(&hnat_flow_offload_lock);
	return -ENOMEM;
}

fe->cookie = cookie;
fe->ppe_index = ppe_index;
fe->hash = hnat_get_ppe_hash(&foe);
memcpy(&fe->data, &foe, sizeof(fe->data));
list_add_tail(&fe->list, &hnat_flow_offload_list);
spin_unlock_bh(&hnat_flow_offload_lock);

return 0;
}

static int hnat_flow_offload_destroy(struct flow_cls_offload *f)
{
	struct hnat_flow_offload_entry *fe;
	int err = -ENOENT;

	spin_lock_bh(&hnat_flow_offload_lock);
	fe = hnat_flow_offload_find_by_cookie_locked(f->cookie);
	if (!fe)
	goto out;

	if (fe->bound) {
		u32 ppe = fe->ppe_index;
		u32 hash = fe->hash;

		spin_unlock_bh(&hnat_flow_offload_lock);
		mtk_hnat_delete_entry_by_index(ppe, hash);
		spin_lock_bh(&hnat_flow_offload_lock);
		fe = hnat_flow_offload_find_by_cookie_locked(f->cookie);
		if (!fe)
		goto out;
	}

list_del(&fe->list);
kfree(fe);
err = 0;
out:
spin_unlock_bh(&hnat_flow_offload_lock);

return err;
}

static int hnat_flow_offload_stats(struct mtk_eth *eth,
struct flow_cls_offload *f)
{
	struct hnat_flow_offload_entry *fe;
	unsigned long long pkt = 0, bytes = 0;
	int ret = 0;

	spin_lock_bh(&hnat_flow_offload_lock);
	fe = hnat_flow_offload_find_by_cookie_locked(f->cookie);
	if (!fe) {
		ret = -ENOENT;
		goto out;
	}

if (fe->bound)
mtk_hnat_get_mib_count_by_index(fe->ppe_index, fe->hash,
&pkt, &bytes);

f->stats.pkts += pkt;
f->stats.bytes += bytes;
f->stats.lastused = jiffies;
out:
spin_unlock_bh(&hnat_flow_offload_lock);

return ret;
}

int mtk_hnat_flow_offload_cmd(struct mtk_eth *eth,
struct flow_cls_offload *cls)
{
	switch (cls->command) {
		case FLOW_CLS_REPLACE:
		return hnat_flow_offload_replace(eth, cls);
		case FLOW_CLS_DESTROY:
		return hnat_flow_offload_destroy(cls);
		case FLOW_CLS_STATS:
		return hnat_flow_offload_stats(eth, cls);
		default:
		return -EOPNOTSUPP;
	}
}

int hnat_flow_offload_init(void)
{
	mtk_flow_offload_hnat_cmd = mtk_hnat_flow_offload_cmd;
	return 0;
}

void hnat_flow_offload_deinit(void)
{
	struct hnat_flow_offload_entry *fe, *tmp;

	mtk_flow_offload_hnat_cmd = NULL;

	spin_lock_bh(&hnat_flow_offload_lock);
	list_for_each_entry_safe(fe, tmp, &hnat_flow_offload_list, list) {
		list_del(&fe->list);
		kfree(fe);
	}
spin_unlock_bh(&hnat_flow_offload_lock);
}
