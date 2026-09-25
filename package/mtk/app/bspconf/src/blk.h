/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2026 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 *
 * bspconf block device implementation
 */

#ifndef _BSP_CONF_BLK_H_
#define _BSP_CONF_BLK_H_

#include <stdint.h>

struct mtk_bsp_conf_data;

void show_bspconf_info_blk(void);
int init_bspconf_blk(void);
int read_bspconf_blk(uint32_t index, struct mtk_bsp_conf_data *buf);
int write_bspconf_blk(uint32_t index, const struct mtk_bsp_conf_data *buf);

#endif /* _BSP_CONF_BLK_H_ */
