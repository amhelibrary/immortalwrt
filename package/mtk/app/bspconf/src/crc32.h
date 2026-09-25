/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2026 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 *
 * bspconf UBI implementation
 */

#ifndef _CRC32_H_
#define _CRC32_H_

#include <stdint.h>
#include <stddef.h>

uint32_t crc32_no_comp(uint32_t crc, const void *data, size_t size);

static inline uint32_t crc32(uint32_t crc, const void *data, size_t size)
{
	return crc32_no_comp(crc ^ 0xffffffff, data, size) ^ 0xffffffff;
}

#endif /* _CRC32_H_ */
