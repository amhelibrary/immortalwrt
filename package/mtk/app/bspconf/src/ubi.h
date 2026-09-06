/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2026 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 *
 * bspconf UBI implementation
 */

#ifndef _BSP_CONF_UBI_H_
#define _BSP_CONF_UBI_H_

#include <stdint.h>

struct mtk_bsp_conf_data;

void show_bspconf_info_ubi(void);
int init_bspconf_ubi(void);
int read_bspconf_ubi(uint32_t index, struct mtk_bsp_conf_data *buf);
int write_bspconf_ubi(uint32_t index, const struct mtk_bsp_conf_data *buf);

#endif /* _BSP_CONF_UBI_H_ */
