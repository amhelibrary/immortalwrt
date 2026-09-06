# SPDX-License-Identifier: GPL-2.0
#
# Copyright (C) 2026 MediaTek Inc.
#
# Author: Weijie Gao <weijie.gao@mediatek.com>
#

cmake_minimum_required(VERSION 3.30)

project(bspconf)

add_executable(bspconf
	bspconf.c bspconf.h
	crc32.c crc32.h
	boot_param.c boot_param.h
	ubi.c ubi.h
	blk.c blk.h
)

set_target_properties(bspconf PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
target_compile_options(bspconf PRIVATE -Wall -Werror -ffunction-sections -fdata-sections)
target_link_options(bspconf PRIVATE -Wl,--gc-sections)
target_link_libraries(bspconf PRIVATE ubi-utils blkid)

install(TARGETS bspconf bspconf
	RUNTIME DESTINATION sbin
)
