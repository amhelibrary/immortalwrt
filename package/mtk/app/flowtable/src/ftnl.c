// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2023 MediaTek Inc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include <netinet/ether.h>

#include "netfilter_flowtable.h"

enum {
	OPT_SIP		= (1 << 0),
	OPT_DIP		= (1 << 1),
	OPT_PROTO	= (1 << 2),
	OPT_SPORT	= (1 << 3),
	OPT_DPORT	= (1 << 4),
	OPT_MAC		= (1 << 5),
};
#define OPT_TUPLE	(OPT_SIP | OPT_DIP | OPT_PROTO | OPT_SPORT | OPT_DPORT)

void usage(void)
{
	printf("#########flush flow table\n");
	printf("ftnl -F\n");
	printf("#########del flow from offload table\n");
	printf("ftnl -D [sip] [dip] [proto] [sport] [dport]\n");
	printf("#########del flows sent to or from a MAC\n");
	printf("ftnl -D --mac xx:xx:xx:xx:xx:xx\n");
}

int main(int argc, char *argv[])
{
	struct ftnl_handle *h;
	struct flow_tuple tuple = {0};
	struct ether_addr mac;
	unsigned int options = 0;
	int msg = -1;
	int c;
	int ret = -1;
	const char *optstring = "FD";
	struct option opts[] = {
		{"sip", required_argument, NULL, 's'},
		{"dip", required_argument, NULL, 'd'},
		{"proto", required_argument, NULL, 'p'},
		{"sport", required_argument, NULL, 'm'},
		{"dport", required_argument, NULL, 'n'},
		{"mac", required_argument, NULL, 'a'},
		{0, 0, 0, 0}
	};

	/* open netlink socket */
	h = ftnl_open();
	if (!h)
		return ret;

	/* parse arg */
	while ((c = getopt_long(argc, argv, optstring, opts, NULL)) != -1) {
		switch (c) {
		case 'F':
			msg = FT_MSG_FLUSH;
			break;
		case 'D':
			msg = FT_MSG_DEL;
			break;
		case 's':
			inet_aton(optarg, &tuple.sip4);
			options |= OPT_SIP;
			break;
		case 'd':
			inet_aton(optarg, &tuple.dip4);
			options |= OPT_DIP;
			break;
		case 'p':
			options |= OPT_PROTO;
			if (!strcmp(optarg, "tcp"))
				tuple.proto = IPPROTO_TCP;
			else if (!strcmp(optarg, "udp"))
				tuple.proto = IPPROTO_UDP;
			else {
				printf("proto bad value...\n");
				printf("pls set proto to udp or tcp arg : %s\n",
				       optarg);
				goto out;
			}
			break;
		case 'm':
			tuple.sport = htons(atoi(optarg));
			options |= OPT_SPORT;
			break;
		case 'n':
			tuple.dport = htons(atoi(optarg));
			options |= OPT_DPORT;
			break;
		case 'a':
			if (!ether_aton_r(optarg, &mac)) {
				printf("mac bad value : %s\n", optarg);
				goto out;
			}
			options |= OPT_MAC;
			break;
		default:
			usage();
			goto out;
		}
	}

	if ((options & OPT_MAC) && (options & OPT_TUPLE)) {
		printf("--mac cannot be used with the 5-tuple options\n");
		usage();
		goto out;
	}

	if (msg == FT_MSG_DEL && !(options & OPT_MAC) &&
	    (options & OPT_TUPLE) != OPT_TUPLE) {
		printf("-D needs --mac or all of the 5-tuple options\n");
		usage();
		goto out;
	}

	switch (msg) {
	case FT_MSG_FLUSH:
		ftnl_flush_table(h);
		break;
	case FT_MSG_DEL:
		if (options & OPT_MAC)
			ftnl_del_mac(h, mac.ether_addr_octet);
		else
			ftnl_del_flow(h, &tuple);
		break;
	default:
		break;
	}

out:
	ftnl_close(h);
	return ret;
}
