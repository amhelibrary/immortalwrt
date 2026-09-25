// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2026 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 *
 * bspconf UBI implementation
 */

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <libubi-tiny.h>
#include "boot_param.h"
#include "bspconf.h"
#include "ubi.h"

/* UBI device info */
static char bspconf1_name[BOOT_PARAM_STR_MAX_LEN];
static char bspconf2_name[BOOT_PARAM_STR_MAX_LEN];
static char bspconf1_vol[64];
static char bspconf2_vol[64];

/* For now use fixed ubi0 device */
static const int ubi_dev_num = 0;

void show_bspconf_info_ubi(void)
{
	printf("Type: UBI\n");

	if (part_state[0] == PART_OK) {
		printf("%s device: %s\n", bspconf1_name, bspconf1_vol);
	} else if (part_state[0] == PART_MISSING) {
		printf("%s device: %s\n", bspconf1_name, "(missing)");
	} else if (part_state[0] == PART_CORRUPTED) {
		printf("%s device: %s (corrupted)\n", bspconf1_name,
		       bspconf1_vol);
	}

	if (part_state[1] == PART_OK) {
		printf("%s device: %s\n", bspconf2_name, bspconf2_vol);
	} else if (part_state[1] == PART_MISSING) {
		printf("%s device: %s\n", bspconf2_name, "(missing)");
	} else if (part_state[1] == PART_CORRUPTED) {
		printf("%s device: %s (corrupted)\n", bspconf2_name,
		       bspconf2_vol);
	}
}

static int find_ubi_vol(libubi_t libubi, char *vol, int *dev_num, int *vol_id,
			bool *corrupted)
{
	struct ubi_vol_info vol_info;
	int ret;

	if (!ubi_dev_present(libubi, ubi_dev_num)) {
		error("ubi%u not exist\n", ubi_dev_num);
		return -ENODEV;
	}

	ret = ubi_get_vol_info1_nm(libubi, ubi_dev_num, vol, &vol_info);
	if (ret) {
		error("No volume named '%s' exist in ubi%u\n", vol,
		      ubi_dev_num);
		return -ENOENT;
	}

	*dev_num = ubi_dev_num;
	*vol_id = vol_info.vol_id;
	*corrupted = vol_info.corrupted;

	return 0;
}

int init_bspconf_ubi(void)
{
	char volbase[BOOT_PARAM_STR_MAX_LEN];
	int dev_num, vol_id, ret;
	libubi_t libubi;
	bool corrupted;

	ret = read_boot_param_string("bspconf-part-vol", volbase,
				     sizeof(volbase));
	if (ret < 0) {
		error("bspconf volume base name not present\n");
		return -1;
	}

	libubi = libubi_open();
	if (!libubi) {
		error("cannot open libubi\n");
		return -1;
	}

	(void)snprintf(bspconf1_name, sizeof(bspconf1_name), "%s1", volbase);
	debug("First bspconf volume name: %s\n", bspconf1_name);

	ret = find_ubi_vol(libubi, bspconf1_name, &dev_num, &vol_id,
			   &corrupted);
	if (ret) {
		if (ret != -ENOENT)
			return ret;

		error("bspconf volume '%s' not exist\n", bspconf1_name);

		part_state[0] = PART_MISSING;
	} else if (!ret) {
		(void)snprintf(bspconf1_vol, sizeof(bspconf1_vol),
			       "/dev/ubi%d_%d", dev_num, vol_id);

		debug("bspconf volume '%s' device: %s\n", bspconf1_name,
		      bspconf1_vol);

		if (corrupted) {
			error("bspconf volume '%s' is corrupted\n",
			      bspconf1_name);

			part_state[0] = PART_CORRUPTED;
		}
	}

	(void)snprintf(bspconf2_name, sizeof(bspconf2_name), "%s2", volbase);
	debug("Second bspconf volume name: %s\n", bspconf2_name);

	ret = find_ubi_vol(libubi, bspconf2_name, &dev_num, &vol_id,
			   &corrupted);
	if (ret) {
		if (ret != -ENOENT)
			return ret;

		error("bspconf volume '%s' not exist\n", bspconf2_name);

		part_state[1] = PART_MISSING;
	} else if (!ret) {
		(void)snprintf(bspconf2_vol, sizeof(bspconf2_vol),
			       "/dev/ubi%d_%d", dev_num, vol_id);

		debug("bspconf volume '%s' device: %s\n", bspconf2_name,
		      bspconf2_vol);

		if (corrupted) {
			error("bspconf volume '%s' is corrupted\n",
			      bspconf2_name);

			part_state[1] = PART_CORRUPTED;
		}
	}

	return 0;
}

static int ubi_read_vol(const char *voldev, void *data, size_t size)
{
	size_t p = 0;
	ssize_t ret;
	int fd;

	fd = open(voldev, O_RDONLY);
	if (fd < 0) {
		error("Error opening %s: %s\n", voldev, strerror(errno));
		return -1;
	}

	do {
		ret = read(fd, data + p, size - p);
		if (ret < 0) {
			if (errno == EINTR)
				continue;

			error("Error reading %s: %s\n", voldev, strerror(errno));
			close(fd);

			return -1;
		}

		if (!ret)
			break;

		p += ret;
	} while (p < size);

	close(fd);

	if (p < size) {
		error("No enough data to read, %zu remaining\n", size - p);
		return -1;
	}

	debug("%zu bytes read from %s\n", size, voldev);

	return 0;
}

static int ubi_update_vol(const char *voldev, const void *data, size_t size)
{
	uint64_t wrlen = sizeof(struct mtk_bsp_conf_data);
	size_t p = 0;
	ssize_t ret;
	int fd;

	fd = open(voldev, O_WRONLY | O_SYNC);
	if (fd < 0) {
		error("Error opening %s: %s\n", voldev, strerror(errno));
		return -ENODEV;
	}

	if (ioctl(fd, UBI_IOCVOLUP, &wrlen) < 0) {
		error("ioctl(UBI_IOCVOLUP) failed: %s\n", strerror(errno));
		close(fd);
		return -EIO;
	}

	do {
		ret = write(fd, data + p, size - p);
		if (ret < 0) {
			if (errno == EINTR)
				continue;

			error("Error writing %s: %s\n", voldev, strerror(errno));
			close(fd);

			return -EIO;
		}

		p += ret;
	} while (p < size);

	close(fd);

	if (p < size) {
		error("Not all data written, %zu remaining\n", size - p);
		return -EIO;
	}

	debug("%zu bytes updated to %s\n", size, voldev);

	return 0;
}

static int ubi_create_vol(const char *name, size_t size, char *devpath,
			  size_t maxpathsize)
{
	struct ubi_mkvol_request req;
	libubi_t libubi;
	char node[64];
	int ret;

	libubi = libubi_open();
	if (!libubi) {
		error("cannot open libubi\n");
		return -1;
	}

	req.vol_id = UBI_VOL_NUM_AUTO;
	req.alignment = 1;
	req.bytes = size;
	req.vol_type = UBI_STATIC_VOLUME;
	req.name = name;

	(void)snprintf(node, sizeof(node), "/dev/ubi%d", ubi_dev_num);

	ret = ubi_mkvol(libubi, node, &req);
	if (ret < 0) {
		error("Failed to create UBI volume '%s' with size %zu\n", name,
		      size);
		return -1;
	}

	(void)snprintf(devpath, maxpathsize, "/dev/ubi%d_%d", ubi_dev_num,
		       req.vol_id);

	debug("Creatd UBI static volume '%s' with size %zu as %s\n", name, size,
	      devpath);

	return 0;
}

int read_bspconf_ubi(uint32_t index, struct mtk_bsp_conf_data *buf)
{
	const char *voldev;

	if (index > 1)
		index = 1;

	voldev = index ? bspconf2_vol : bspconf1_vol;

	if (part_state[index] != PART_OK)
		return -EUCLEAN;

	return ubi_read_vol(voldev, buf, sizeof(*buf));
}

int write_bspconf_ubi(uint32_t index, const struct mtk_bsp_conf_data *buf)
{
	const char *volname;
	char *voldev;
	int ret;

	if (index > 1)
		index = 1;

	voldev = index ? bspconf2_vol : bspconf1_vol;

	if (part_state[index] == PART_MISSING) {
		volname = index ? bspconf2_name : bspconf1_name;

		ret = ubi_create_vol(volname, sizeof(*buf), voldev,
				     sizeof(bspconf1_vol));
		if (ret)
			return -ENODEV;

		part_state[index] = PART_CORRUPTED;
	}

	ret = ubi_update_vol(voldev, buf, sizeof(*buf));
	if (!ret)
		part_state[index] = PART_OK;

	return ret;
}
