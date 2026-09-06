// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2026 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 *
 * Tool for editing bspconf data
 */

#include <limits.h>
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include "boot_param.h"
#include "bspconf.h"
#include "crc32.h"
#include "ubi.h"
#include "blk.h"

#define __noreturn	__attribute__((noreturn))

enum cmd_type {
	CMD_PROBE,
	CMD_SHOW,
	CMD_GET,
	CMD_SET,
	CMD_UPD,
};

enum part_type {
	PART_UBI,
	PART_BLK,
};

struct bspconf_editable_item {
	const char *name;
	uintptr_t offset;
	size_t size;
};

#define member_size_of(_type, _member)		(sizeof(((_type *)0)->_member))

#define _KEY(_name, _field)	\
	{ .name = (_name), \
	  .offset = offsetof(struct mtk_bsp_conf_data, _field), \
	  .size = member_size_of(struct mtk_bsp_conf_data, _field), }

#define KEY(_field)	\
	_KEY(#_field, _field)

static int verbose, ashex, hexprefix;
static enum part_type part_type;
static enum cmd_type cmd;
static const struct bspconf_editable_item *key;
static char *key_val;
static struct mtk_image_info *image_info;
static uint32_t *image_curr_slot;
static char *image_file;
static uint32_t image_slot;

enum part_state part_state[2];
static bool data_valid[2];

static struct mtk_bsp_conf_data curr_bspconf, orig_bspconf;
static uint32_t curr_bspconf_index;

static const struct bspconf_editable_item key_items[] = {
	KEY(current_fip_slot),
	KEY(fip[0].invalid),
	KEY(fip[0].size),
	KEY(fip[0].crc32),
	KEY(fip[0].upd_cnt),
	KEY(fip[1].invalid),
	KEY(fip[1].size),
	KEY(fip[1].crc32),
	KEY(fip[1].upd_cnt),

	KEY(current_image_slot),
	KEY(image[0].invalid),
	KEY(image[0].size),
	KEY(image[0].crc32),
	KEY(image[0].upd_cnt),
	KEY(image[1].invalid),
	KEY(image[1].size),
	KEY(image[1].crc32),
	KEY(image[1].upd_cnt),
};

void error(const char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	(void)vfprintf(stderr, fmt, va);
	va_end(va);

	(void)fflush(stderr);
}

void debug(const char *fmt, ...)
{
	va_list va;

	if (!verbose)
		return;

	va_start(va, fmt);
	(void)vprintf(fmt, va);
	va_end(va);

	(void)fflush(stdout);
}

static void show_bspconf_info(void)
{
	switch (part_type) {
	case PART_UBI:
		show_bspconf_info_ubi();
		break;

	case PART_BLK:
		show_bspconf_info_blk();
		break;

	default:
		break;
	}
}

static int read_bspconf(uint32_t index, struct mtk_bsp_conf_data *buf)
{
	switch (part_type) {
	case PART_UBI:
		return read_bspconf_ubi(index, buf);

	case PART_BLK:
		return read_bspconf_blk(index, buf);

	default:
		return -1;
	}
}

static int write_bspconf(uint32_t index, const struct mtk_bsp_conf_data *buf)
{
	switch (part_type) {
	case PART_UBI:
		return write_bspconf_ubi(index, buf);

	case PART_BLK:
		return write_bspconf_blk(index, buf);

	default:
		return -1;
	}
}

static int parse_fdt_args(void)
{
	char type[BOOT_PARAM_STR_MAX_LEN];
	int ret;

	ret = read_boot_param_string("bspconf-part-type", type, sizeof(type));
	if (ret < 0) {
		error("bspconf configuration not present\n");
		return -1;
	}

	if (!strcmp(type, "ubi")) {
		ret = init_bspconf_ubi();
		if (!ret)
			part_type = PART_UBI;

		return ret;
	}

	if (!strcmp(type, "blk")) {
		ret = init_bspconf_blk();
		if (!ret)
			part_type = PART_BLK;

		return ret;
	}

	error("Unknown bspconf configuration type '%s'\n", type);

	return -1;
}

static void __noreturn usage(const char *prog)
{
	const char *progname = strrchr(prog, '/');
	uint32_t i;

	if (progname)
		progname++;
	else
		progname = prog;

	printf("Usage:\n");
	printf("    %s [options] [command [args...]]\n", progname);
	printf("\n");
	printf("Command:\n");
	printf("    probe - Detect if bspconf is available\n");
	printf("    show  - Detect if bspconf is available\n");
	printf("    get   - Get value of key\n");
	printf("            Args: <key>\n");
	printf("    set   - Set key with value\n");
	printf("            Args: <key> <val>\n");
	printf("    upd   - Update and set new active slot info\n");
	printf("            Args: <type> <slot> [file]\n");
	printf("                - type: image or fip\n");
	printf("                - slot: 0 or 1\n");
	printf("\n");
	printf("Options:\n");
	printf("    -v Show verbose information\n");
	printf("    -x Display key value as hex\n");
	printf("    -p Add 0x prefix when displaying as hex\n");
	printf("    -h Display this usage\n");
	printf("\n");
	printf("Available keys:\n");

	for (i = 0; i < sizeof(key_items) / sizeof(key_items[0]); i++)
		printf("    %s\n", key_items[i].name);

	exit(0);
}

static const struct bspconf_editable_item *find_key(const char *name)
{
	uint32_t i;

	for (i = 0; i < sizeof(key_items) / sizeof(key_items[0]); i++) {
		if (!strcmp(name, key_items[i].name))
			return &key_items[i];
	}

	return NULL;
}

static int parse_args(int argc, char *argv[])
{
	uint32_t slot;
	char *end;
	int opt;

	while ((opt = getopt(argc, argv, "vxph")) != -1) {
		switch (opt) {
		case 'v':
			verbose = 1;
			break;

		case 'x':
			ashex = 1;
			break;

		case 'p':
			hexprefix = 1;
			break;

		case 'h':
			usage(argv[0]);
			break;

		default:
			error("Invalid argument '%c'\n", opt);
			error("Use -h argument for detailed usage\n");
			return -1;
		}
	}

	if (optind >= argc) {
		error("No command specified\n");
		error("Use -h argument for all available commands\n");
		return -1;
	}

	if (!strcmp(argv[optind], "probe")) {
		cmd = CMD_PROBE;
		return 0;
	}

	if (!strcmp(argv[optind], "show")) {
		cmd = CMD_SHOW;
		return 0;
	}

	if (!strcmp(argv[optind], "get")) {
		if (argc - optind < 2) {
			error("Key name not specified\n");
			return -1;
		}

		key = find_key(argv[optind + 1]);
		cmd = CMD_GET;

		if (!key) {
			error("Unsupported key name\n");
			error("Use -h argument for all available keys\n");
			return -1;
		}

		return 0;
	}

	if (!strcmp(argv[optind], "set")) {
		if (argc - optind < 3) {
			if (argc - optind < 2)
				error("Key name not specified\n");
			else
				error("Key value not specified\n");

			return -1;
		}

		key = find_key(argv[optind + 1]);
		key_val = argv[optind + 2];
		cmd = CMD_SET;

		if (!key) {
			error("Unsupported key name\n");
			error("Use -h argument for all available keys\n");
			return -1;
		}

		return 0;
	}

	if (!strcmp(argv[optind], "upd")) {
		if (argc - optind < 3) {
			if (argc - optind < 2)
				error("Image type not specified\n");
			else
				error("Active slot not specified\n");

			return -1;
		}

		if (!strcmp(argv[optind + 1], "fip")) {
			slot = strtoul(argv[optind + 2], &end, 10);
			if ((slot == ULONG_MAX && errno == ERANGE) || *end ||
			    end == argv[optind + 1] || slot >= FIP_NUM) {
				error("Invalid FIP slot number\n");
				return -1;
			}

			image_slot = slot;
			image_info = &curr_bspconf.fip[slot];
			image_curr_slot = &curr_bspconf.current_fip_slot;
		} else if (!strcmp(argv[optind + 1], "image")) {
			slot = strtoul(argv[optind + 2], &end, 10);
			if ((slot == ULONG_MAX && errno == ERANGE) || *end ||
			    end == argv[optind + 1] || slot >= IMAGE_NUM) {
				error("Invalid image slot number\n");
				return -1;
			}

			image_slot = slot;
			image_info = &curr_bspconf.image[slot];
			image_curr_slot = &curr_bspconf.current_image_slot;
		} else {
			error("Unsupported image type\n");
			return -1;
		}

		if (argc - optind > 3)
			image_file = argv[optind + 3];

		cmd = CMD_UPD;

		return 0;
	}

	error("Unsupported command '%s'\n", argv[optind]);
	error("Use -h argument for all available commands\n");

	return -1;
}

static void __update_bsp_conf(struct mtk_bsp_conf_data *bspconf)
{
	const uint32_t old_crc = bspconf->crc;

	bspconf->ver = MTK_BSP_CONF_VER;
	bspconf->len = sizeof(*bspconf);
	bspconf->crc = crc32(0, (const uint8_t *)&bspconf->len,
			     bspconf->len - sizeof(bspconf->crc));

	if (old_crc != bspconf->crc) {
		debug("bspconf data changed, crc32: %08x -> %08x\n",
		      old_crc, bspconf->crc);
	}
}

static int check_bsp_conf(const struct mtk_bsp_conf_data *bspconf,
			  uint32_t index)
{
	uint32_t crc;

	if (bspconf->len < offsetof(struct mtk_bsp_conf_data, len) +
			   sizeof(bspconf->len)) {
		error("BSP configuration %u is invalid\n", index + 1);
		return -EBADMSG;
	}

	if (bspconf->len > sizeof(*bspconf)) {
		error("BSP configuration %u length (%u) unsupported\n",
		       index + 1, bspconf->len);
		return -EBADMSG;
	}

	crc = crc32(0, (const uint8_t *)&bspconf->len,
		    bspconf->len - sizeof(bspconf->crc));
	if (crc != bspconf->crc) {
		error("BSP configuration %u is corrupted\n", index + 1);
		return -EBADMSG;
	}

	if (bspconf->ver > MTK_BSP_CONF_VER) {
		error("BSP configuration %u version (%u) unsupported\n",
		       index + 1, bspconf->ver);
		return -EBADMSG;
	}

	if (bspconf->ver == MTK_BSP_CONF_VER &&
	    bspconf->len < sizeof(*bspconf)) {
		error("BSP configuration %u size (%u) mismatch\n",
		       index + 1, bspconf->len);
		return -EBADMSG;
	}

	debug("BSP configuration %u is OK\n", index + 1);

	return 0;
}

static void import_bsp_conf(void)
{
	struct mtk_bsp_conf_data bspconf1, bspconf2;
	const struct mtk_bsp_conf_data *bc;
	int ret;

	if (part_state[0] == PART_OK) {
		ret = read_bspconf(0, &bspconf1);
		if (!ret) {
			ret = check_bsp_conf(&bspconf1, 0);
			if (!ret)
				data_valid[0] = true;
		}
	}

	if (part_state[1] == PART_OK) {
		ret = read_bspconf(1, &bspconf2);
		if (!ret) {
			ret = check_bsp_conf(&bspconf2, 1);
			if (!ret)
				data_valid[1] = true;
		}
	}

	if (!data_valid[0] && !data_valid[1]) {
		debug("No usable BSP configuration\n");

		if (part_state[0] != PART_OK && part_state[1] == PART_OK)
			curr_bspconf_index = 1;

		return;
	}

	if (data_valid[0]) {
		debug("BSP configuration 1 has update count %u\n",
		      bspconf1.upd_cnt);
	}

	if (data_valid[1]) {
		debug("BSP configuration 2 has update count %u\n",
		      bspconf2.upd_cnt);
	}

	if (data_valid[0] && !data_valid[1]) {
		bc = &bspconf1;
	} else if (!data_valid[0] && data_valid[1]) {
		bc = &bspconf2;
	} else {
		if (bspconf1.upd_cnt == UINT32_MAX && bspconf2.upd_cnt == 0)
			bc = &bspconf2;
		else if (bspconf2.upd_cnt == UINT32_MAX &&
			 bspconf1.upd_cnt == 0)
			bc = &bspconf1;
		else if (bspconf1.upd_cnt > bspconf2.upd_cnt)
			bc = &bspconf1;
		else if (bspconf2.upd_cnt > bspconf1.upd_cnt)
			bc = &bspconf2;
		else
			bc = &bspconf1;
	}

	curr_bspconf_index = bc == &bspconf1 ? 0 : 1;

	debug("Using BSP configuration %u\n", curr_bspconf_index + 1);

	memcpy(&curr_bspconf, bc, sizeof(curr_bspconf));

	if (curr_bspconf.len < sizeof(curr_bspconf)) {
		memset(((uint8_t *)&curr_bspconf) + curr_bspconf.len, 0,
		       sizeof(curr_bspconf) - curr_bspconf.len);
	}

	memcpy(&orig_bspconf, &curr_bspconf, sizeof(curr_bspconf));
}

static bool bsp_conf_changed(void)
{
	__update_bsp_conf(&curr_bspconf);

	if (!memcmp(&curr_bspconf, &orig_bspconf, sizeof(curr_bspconf)))
		return false;

	return true;
}

static int __save_bsp_conf(uint32_t index)
{
	int ret;

	curr_bspconf.upd_cnt++;

	__update_bsp_conf(&curr_bspconf);

	ret = write_bspconf(index, &curr_bspconf);
	if (ret) {
		error("Error: failed to update BSP configuration %u\n",
		      index + 1);
		return ret;
	}

	debug("BSP configuration %u has been updated\n", index + 1);

	memcpy(&orig_bspconf, &curr_bspconf, sizeof(curr_bspconf));

	curr_bspconf_index = index;
	data_valid[index] = true;

	return 0;
}

static int save_bsp_conf(void)
{
	uint32_t prev_index = curr_bspconf_index;
	uint32_t next_index = prev_index ? 0 : 1;
	int ret;

	if (data_valid[0] && data_valid[1] && !bsp_conf_changed()) {
		debug("BSP configuration unchanged\n");
		return 0;
	}

	ret = __save_bsp_conf(next_index);
	if (ret)
		return ret;

	if (!data_valid[prev_index] &&
	    (part_type == PART_UBI || part_state[0] == PART_OK))
		return __save_bsp_conf(prev_index);

	return 0;
}

static void print_image_slot(const struct mtk_image_info *info, uint32_t slot)
{
	printf("=> Slot %u:       %s\n", slot,
					 info->invalid ? "invalid" : "valid");
	printf("   Data size:    %u (0x%x)\n", info->size, info->size);
	printf("   CRC32:        %08x\n", info->crc32);
	printf("   Update count: %u\n", info->upd_cnt);
}

static void do_bspconf_show(void)
{
	printf("==== BSP Configuration %u ====\n", curr_bspconf_index);
	printf("CRC32:        %08x\n", curr_bspconf.crc);
	printf("Length:       %u (0x%x)\n", curr_bspconf.len, curr_bspconf.len);
	printf("Version:      %u\n", curr_bspconf.ver);
	printf("Update count: %u\n", curr_bspconf.upd_cnt);
	printf("\n");

	printf("==== Dual FIP ====\n");
	printf("Current slot: %u\n", curr_bspconf.current_fip_slot);
	print_image_slot(&curr_bspconf.fip[0], 0);
	print_image_slot(&curr_bspconf.fip[1], 1);
	printf("\n");

	printf("==== Dual Image ====\n");
	printf("Current slot: %u\n", curr_bspconf.current_image_slot);
	print_image_slot(&curr_bspconf.image[0], 0);
	print_image_slot(&curr_bspconf.image[1], 1);
}

static int do_bspconf_get(void)
{
	const void *p;
	uint64_t val;

	p = (const void *)((uintptr_t)&curr_bspconf + key->offset);

	switch (key->size) {
	case 1:
		val = *(const uint8_t *)p;
		break;

	case 2:
		val = *(const uint16_t *)p;
		break;

	case 4:
		val = *(const uint32_t *)p;
		break;

	case 8:
		val = *(const uint64_t *)p;
		break;

	default:
		error("Unsupported key: %s\n", key->name);
		return -1;
	}

	if (ashex) {
		if (hexprefix)
			printf("0x%" PRIx64 "\n", val);
		else
			printf("%" PRIx64 "\n", val);
	} else {
		printf("%" PRIu64 "\n", val);
	}

	return 0;
}

static int do_bspconf_set(void)
{
	uint64_t val, max_val;
	char *end;
	void *p;

	val = strtoull(key_val, &end, 0);
	if ((val == ULLONG_MAX && errno == ERANGE) || *end || end == key_val) {
		error("Invalid key value\n");
		return -1;
	}

	switch (key->size) {
	case 1:
		max_val = UINT8_MAX;
		break;

	case 2:
		max_val = UINT16_MAX;
		break;

	case 4:
		max_val = UINT32_MAX;
		break;

	case 8:
		max_val = UINT64_MAX;
		break;

	default:
		error("Unsupported key: %s\n", key->name);
		return -1;
	}

	if (!strcmp(key->name, "current_fip_slot"))
		max_val = FIP_NUM - 1;
	else if (!strcmp(key->name, "current_image_slot"))
		max_val = IMAGE_NUM - 1;

	if (val > max_val) {
		error("Key value out of range. Max value is %" PRIu64 "\n",
		      max_val);
		return -1;
	}

	p = (void *)((uintptr_t)&curr_bspconf + key->offset);

	switch (key->size) {
	case 1:
		*(uint8_t *)p = (uint8_t)val;
		break;

	case 2:
		*(uint16_t *)p = (uint16_t)val;
		break;

	case 4:
		*(uint32_t *)p = (uint32_t)val;
		break;

	case 8:
		*(uint64_t *)p = (uint64_t)val;
		break;
	}

	return save_bsp_conf();
}

static int do_bspconf_upd(void)
{
	uint32_t crc = 0, len = 0;
	uint8_t buf[4096];
	int fd, ret;

	if (image_file) {
		fd = open(image_file, O_RDONLY);
		if (fd < 0) {
			error("Failed to open file '%s': %s\n", image_file,
			      strerror(errno));
			return -1;
		}

		do {
			ret = read(fd, buf, sizeof(buf));
			if (ret < 0) {
				if (errno == EINTR)
					continue;

				error("Error reading file: %s\n",
				      strerror(errno));
				close(fd);

				return -1;
			}

			crc = crc32(crc, buf, ret);
			len += (uint32_t)ret;
		} while (ret > 0);

		close(fd);

		debug("Image file size is %u, crc32 is %08x\n", len, crc);

		image_info->crc32 = crc;
		image_info->size = len;
	} else {
		image_info->crc32 = 0;
		image_info->size = 0;
	}

	image_info->invalid = 0;
	image_info->upd_cnt++;

	*image_curr_slot = image_slot;

	return save_bsp_conf();
}

int main(int argc, char *argv[])
{
	int ret;

	if (argc == 1)
		usage(argv[0]);

	ret = parse_args(argc, argv);
	if (ret)
		return 1;

	ret = parse_fdt_args();
	if (ret)
		return 1;

	import_bsp_conf();

	switch (cmd) {
	case CMD_PROBE:
		show_bspconf_info();
		break;

	case CMD_SHOW:
		do_bspconf_show();
		break;

	case CMD_GET:
		ret = do_bspconf_get();
		if (ret)
			return 1;
		break;

	case CMD_SET:
		ret = do_bspconf_set();
		if (ret)
			return 1;
		break;

	case CMD_UPD:
		ret = do_bspconf_upd();
		if (ret)
			return 1;
		break;

	default:
		break;
	}

	return 0;
}
