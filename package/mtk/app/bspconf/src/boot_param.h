/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2026 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 *
 * MediaTek kernel boot parameter helper
 */

#ifndef _BOOTPARAM_H_
#define _BOOTPARAM_H_

#include <stddef.h>

#define BOOT_PARAM_STR_MAX_LEN			256

int read_boot_param_string(const char *name, char *val, size_t maxsize);
char *blockdev_parse(const char *name);

#endif /* _BOOTPARAM_H_ */
