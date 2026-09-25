/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2026 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 *
 * bspconf data structure and helpers
 */

#ifndef _BSP_CONF_H_
#define _BSP_CONF_H_

#include <stdint.h>

#define MTK_BSP_CONF_VER			1

#define FIP_NUM					2
#define IMAGE_NUM				2

struct mtk_image_info {
	uint32_t invalid;
	uint32_t size;
	uint32_t crc32;
	uint32_t upd_cnt;
};

struct mtk_bsp_conf_data {
	uint32_t crc;
	uint32_t len;
	uint32_t ver;
	uint32_t upd_cnt;

	/* Dual FIP */
	uint32_t current_fip_slot;
	struct mtk_image_info fip[FIP_NUM];

	/* Dual image */
	uint32_t current_image_slot;
	struct mtk_image_info image[IMAGE_NUM];
};

void error(const char *fmt, ...);
void debug(const char *fmt, ...);

enum part_state {
	PART_OK,
	PART_MISSING,
	PART_CORRUPTED,
};

extern enum part_state part_state[2];

#endif /* _BSP_CONF_H_ */
