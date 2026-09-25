// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2026 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 *
 * bspconf block device implementation
 */

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/fs.h>
#include "boot_param.h"
#include "bspconf.h"
#include "blk.h"

#define BLK_SIZE	512

#define BSPCONF_ALIGNED_SIZE	\
	((sizeof(struct mtk_bsp_conf_data) + BLK_SIZE - 1) & ~(BLK_SIZE - 1))

/* block device info */
static char bspconf_part_id[BOOT_PARAM_STR_MAX_LEN];
static char *bspconf_part_dev;	/* No need to free this */
static uint64_t bspconf_part_size;
static uint64_t bspconf_part_redund_offs;

void show_bspconf_info_blk(void)
{
	printf("Type: Block device\n");
	printf("Part identifier: %s\n", bspconf_part_id);
	printf("Part device: %s\n", bspconf_part_dev);
	printf("Part size: %" PRIu64 " (0x%" PRIx64 ")\n", bspconf_part_size,
	       bspconf_part_size);
	printf("Redundancy offset: 0x%" PRIx64 "\n", bspconf_part_redund_offs);
}

int init_bspconf_blk(void)
{
	int ret, fd;

	ret = read_boot_param_string("bspconf-part", bspconf_part_id,
				     sizeof(bspconf_part_id));
	if (ret < 0) {
		error("bspconf block identifier not present\n");
		return -1;
	}

	debug("bspconf block identifier: %s\n", bspconf_part_id);

	bspconf_part_dev = blockdev_parse(bspconf_part_id);
	if (!bspconf_part_dev) {
		error("bspconf block device not exist\n");
		return -1;
	}

	debug("bspconf block device: %s\n", bspconf_part_dev);

	fd = open(bspconf_part_dev, O_RDONLY);
	if (fd < 0) {
		error("Failed to open block device '%s'\n", bspconf_part_dev);
		return -1;
	}

	ret = ioctl(fd, BLKGETSIZE64, &bspconf_part_size);
	close(fd);

	if (ret < 0) {
		error("Failed to get block device size: %s\n", strerror(errno));
		return -1;
	}

	debug("bspconf block size: %" PRIu64 " (0x%" PRIx64 ")\n",
	      bspconf_part_size, bspconf_part_size);

	if (bspconf_part_size < 2 * BSPCONF_ALIGNED_SIZE) {
		error("Warning: bspconf partition is too small for redundancy\n");
		part_state[1] = PART_MISSING;
	} else {
		bspconf_part_redund_offs = bspconf_part_size - BSPCONF_ALIGNED_SIZE;

		debug("bspconf block redundancy offset: 0x%" PRIx64 "\n",
		      bspconf_part_redund_offs);
	}

	return 0;
}

int read_bspconf_blk(uint32_t index, struct mtk_bsp_conf_data *buf)
{
	size_t p = 0, size = sizeof(*buf);
	void *data = buf;
	uint64_t offset;
	ssize_t ret;
	int fd;

	if (index > 1)
		index = 1;

	if (part_state[index] == PART_MISSING)
		return -ENODEV;

	offset = index ? bspconf_part_redund_offs : 0;

	fd = open(bspconf_part_dev, O_RDONLY);
	if (fd < 0) {
		error("Failed to open block device '%s'\n", bspconf_part_dev);
		return -1;
	}

	ret = lseek(fd, (off_t)offset, SEEK_SET);
	if (ret < 0) {
		error("lseek() on '%s' failed: %s\n", bspconf_part_dev,
		      strerror(errno));
		close(fd);
		return -1;
	}

	do {
		ret = read(fd, data + p, size - p);
		if (ret < 0) {
			if (errno == EINTR)
				continue;

			error("Error reading %s: %s\n", bspconf_part_dev,
			      strerror(errno));
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

	debug("%zu bytes read from %s at 0x%" PRIx64 "\n", size,
	      bspconf_part_dev, offset);

	return 0;
}

int write_bspconf_blk(uint32_t index, const struct mtk_bsp_conf_data *buf)
{
	uint8_t aligned_buf[BSPCONF_ALIGNED_SIZE];
	size_t p = 0, size = BSPCONF_ALIGNED_SIZE;
	const void *data = aligned_buf;
	uint64_t offset;
	ssize_t ret;
	int fd;

	if (index > 1)
		index = 1;

	if (part_state[index] == PART_MISSING)
		return -ENODEV;

	memset(aligned_buf, 0, sizeof(aligned_buf));
	memcpy(aligned_buf, buf, sizeof(*buf));

	offset = index ? bspconf_part_redund_offs : 0;

	fd = open(bspconf_part_dev, O_WRONLY | O_SYNC);
	if (fd < 0) {
		error("Failed to open block device '%s'\n", bspconf_part_dev);
		return -ENODEV;
	}

	ret = lseek(fd, (off_t)offset, SEEK_SET);
	if (ret < 0) {
		error("lseek() on '%s' failed: %s\n", bspconf_part_dev,
		      strerror(errno));
		close(fd);
		return -EIO;
	}

	do {
		ret = write(fd, data + p, size - p);
		if (ret < 0) {
			if (errno == EINTR)
				continue;

			error("Error writing %s: %s\n", bspconf_part_dev,
			      strerror(errno));
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

	debug("%zu bytes written to %s at 0x%" PRIx64 "\n", size,
	      bspconf_part_dev, offset);

	return 0;
}
