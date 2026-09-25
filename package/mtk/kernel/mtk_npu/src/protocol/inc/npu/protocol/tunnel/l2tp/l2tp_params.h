/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Frank-zj Lin <rank-zj.lin@mediatek.com>
 */

#ifndef _NPU_L2TP_H_
#define _NPU_L2TP_H_

/* L2TP header constants */
#define L2TP_HDRFLAG_T		0x8000
#define L2TP_HDRFLAG_L		0x4000
#define L2TP_HDRFLAG_S		0x0800
#define L2TP_HDRFLAG_O		0x0200
#define L2TP_HDRFLAG_P		0x0100
#define L2TP_HDRFLAG_RSVD	0x34F0

#define L2TP_HDR_VER_MASK	0x000F
#define L2TP_HDR_VER_2		0x0002
#define L2TP_HDR_VER_3		0x0003

#define UDP_L2TP_PORT		1701

struct npu_l2tp_params {
	u16 dl_tid; /* l2tp tunnel id for DL */
	u16 dl_sid; /* l2tp session id for DL */
	u16 ul_tid; /* l2tp tunnel id for UL */
	u16 ul_sid; /* l2tp session id for UL */
};

struct l2tp_flag_hdr {
	u16 flag_ver;
};

struct l2tp_len_hdr {
	u16 length;
};

struct udp_l2tp_data_hdr {
	u16 tid;
	u16 sid;
};

struct l2tp_ns_nr_hdr {
	u16 ns;
	u16 nr;
};

struct l2tp_offset_hdr {
	u16 offset_size;
};
#endif /* _NPU_L2TP_H_ */
