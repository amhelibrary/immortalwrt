#!/bin/ash
# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright (C) 2026 MediaTek Inc.
#
# This script wraps ated commands into atenl commands

. "/lib/atenl/utils.sh"

main() {
	# executing in background
	local background="1"
	local quoted="0"
	local i

	parse_sku

	cmd="atenl"
	for i in "$@"; do
		case "${i}" in
			"ra"*)
				interface="${i}"
				convert_interface
				cmd="${cmd} ${interface}"
				;;
			*)
				[ "${quoted}" = "1" ] && {
					cmd="${cmd} \"${i}\""
					quoted="0"
					continue
				}

				[ "${i}" = "-c" ] && {
					background="0"
					quoted="1"
				}

				cmd="${cmd} ${i}"
				;;
		esac
	done

	[ "${background}" = "1" ] && do_cmd "killall atenl > /dev/null 2>&1"
	do_cmd "${cmd}"
}

main "$@"
