// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2026 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 *
 * MediaTek kernel boot parameter helper
 */

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <glob.h>
#include <sys/stat.h>
#include <blkid/blkid.h>
#include "boot_param.h"

#define ARRAY_SIZE(a)	(sizeof(a) / sizeof(a[0]))

int read_boot_param_string(const char *name, char *val, size_t maxsize)
{
	char path[BOOT_PARAM_STR_MAX_LEN];
	size_t len;
	FILE *f;

	snprintf(path, sizeof(path),
		 "/sys/firmware/devicetree/base/mediatek,%s", name);

	f = fopen(path, "rb");
	if (!f) {
		val[0] = 0;
		return -1;
	}

	len = fread(val, 1, maxsize - 1, f);
	fclose(f);

	val[len] = 0;

	while (len > 0) {
		if (val[len - 1] != '\n' && val[len - 1] != '\r')
			break;

		val[--len] = 0;
	}

	return len;
}

static char *lookup_block_dev(const char *path, const char *key, bool is_uuid)
{
	int gl_flags = GLOB_NOESCAPE | GLOB_MARK;
	const char *type, *value;
	char *result = NULL;
	size_t i, len;
	glob_t gl;

	if (glob(path, gl_flags, NULL, &gl) < 0)
		return NULL;

	type = is_uuid ? "PART_ENTRY_UUID" : "PART_ENTRY_NAME";

	for (i = 0; i < gl.gl_pathc; i++) {
		blkid_probe pr = blkid_new_probe_from_filename(gl.gl_pathv[i]);
		if (!pr)
			continue;

		blkid_probe_enable_partitions(pr, 1);
		blkid_probe_set_partitions_flags(pr, BLKID_PARTS_ENTRY_DETAILS);

		if (!blkid_do_safeprobe(pr)) {
			if (!blkid_probe_lookup_value(pr, type, &value, &len)) {
				if (!strcmp(value, key))
					result = strdup(gl.gl_pathv[i]);
			}
		}

		blkid_free_probe(pr);

		if (result)
			break;
	}

	globfree(&gl);

	return result;
}

static char *find_block_dev(const char *key, bool is_uuid)
{
	char *devpath;
	uint32_t i;

	static const char *block_pats[] = {
		"/dev/loop*",
		"/dev/mmcblk*",
		"/dev/sd*",
		"/dev/hd*",
		"/dev/md*",
		"/dev/nvme*",
		"/dev/vd*",
		"/dev/xvd*",
		"/dev/dm-*",
		"/dev/fit*",
	};

	for (i = 0; i < ARRAY_SIZE(block_pats); i++) {
		devpath = lookup_block_dev(block_pats[i], key, is_uuid);
		if (devpath)
			return devpath;
	}

	return NULL;
}

char *blockdev_parse(const char *name)
{
	char *e, *part_dev_path;
	struct stat st;

	if (!name)
		return NULL;

	e = strchr(name, '=');
	if (e) {
		*e = 0;
		e++;
	}

	if (!e) {
		if (stat(name, &st))
			return NULL;

		if (!S_ISBLK(st.st_mode))
			return NULL;

		part_dev_path = strdup(name);
	} else if (!strcmp(name, "PARTLABEL")) {
		part_dev_path = find_block_dev(e, false);
	} else if (!strcmp(name, "PARTUUID")) {
		if (strlen(e) != 36)
			return NULL;
		part_dev_path = find_block_dev(e, true);
	} else {
		return NULL;
	}

	return part_dev_path;
}
