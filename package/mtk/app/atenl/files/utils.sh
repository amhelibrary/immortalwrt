#!/bin/ash
# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright (C) 2026 MediaTek Inc.
#

DUT_CONFIG_FILE="/dev/shm/dut_config"
SETTING_FILE="/dev/shm/setting"

# primary chip band number
# For PCIE + SOC sku, PCIE chip will be the primary one
pri_chip_bands=""
sec_chip_bands=""
connac_ver=""
# RUN/PRINT/DEBUG
work_mode="RUN"

ATECTRLBANDIDX=""
ATETXBW=""
ATEDA=""
ATESA=""
ATEBSSID=""
ATETXMODE=""

# <iwpriv/mwctl> <interface> <cmd_type> <full_cmd>
interface=""			# phy0/phy1/ra0
cmd_type=""			# set/show/e2p/mac/dump/switch
full_cmd=""
interface_ori=""

# Split "${full_cmd}" by "=": "cmd" receives the key and "param" receives the value.
# If "=" is missing, "cmd" defaults to "${full_cmd}" and "param" is set to ""
cmd=""
param=""

phy_idx=""
main_phy_idx=""

print_debug() {
	[ "${work_mode}" = "DEBUG" ] && echo "[DEBUG] $1"
}

print_err() {
	echo "[ERROR] $1"
	exit 1
}

write_dmesg() {
	echo "$1" > /dev/kmsg
}

do_cmd() {
	case "${work_mode}" in
		"RUN")
			eval "$1"
			;;
		"PRINT")
			echo "$1" >&2
			;;
		"DEBUG")
			echo "$1" >&2
			eval "$1"
			;;
	esac
}

query_cmd() {
	local __target_var="$1"
	local __cmd="$2"

	case "${work_mode}" in
		"RUN")
			eval "${__target_var}=\"\$(${__cmd})\""
			;;
		"PRINT")
			echo "${__cmd}" >&2
			eval "${__target_var}=\"\""
			;;
		"DEBUG")
			echo "${__cmd}" >&2
			eval "${__target_var}=\"\$(${__cmd})\""
			;;
	esac
}

mt76_set_cmd() {
	local __cmd="mt76-test ${interface} set $1"

	[ -z "$1" ] && print_err "Empty command"

	do_cmd "${__cmd}"
}

key_val_pair() {
	# ==========================================
	# Usage:
	#	key_val_pair "var1" "var2" "var3" ...
	#
	# Description:
	#	varx will become "varx=${varx}"
	# ==========================================
	local __key __val

	for __key in "$@"; do
		eval "__val=\"\${${__key}}\""
		[ -z "${__val}" ] && continue

		eval "${__key}=\"${__key}=\${__val}\""
	done
}

concat_vars() {
	# ==========================================
	# Usage:
	#	concat_vars "${vars}" "target"
	#
	# Description:
	#	Convert variables to key-value pairs and append them to the target variable
	#
	# Example:
	#	__vars="aa bb cc"
	#	__target_var="dd"
	#	final result dd="${dd} aa=${aa} bb=${bb} cc=${cc}"
	# ==========================================
	local __target_var="$1"
	local __vars="$2"
	local __v

	key_val_pair ${__vars}

	for __v in ${__vars}; do
		eval "local __val=\"\${$__v}\""
		[ -n "${__val}" ] && eval "${__target_var}=\"\${${__target_var}} \${__val}\""
	done
}

remove_leading_zeros() {
	# ==========================================
	# Usage:
	#	1. remove_leading_zeros "var"
	# 	2. remove_leading_zeros "var" ${val}
	#
	# Description:
	#	1. In-place removing leading zeros in ${var}
	#	2. Removing leading zeros in ${val}, and assign to ${var}
	# ==========================================
	local __target_var="$1"
	local __val="$2"
	local __res=""

	# if the second argument is not passed, then get the val of the __target_var
	[ -z "${__val}" ] && eval "__val=\"\${${__target_var}}\""
	# if the original value of __target_var is "", then simply return
	[ -z "${__val}" ] && return

	__res="${__val#"${__val%%[!0]*}"}"
	eval "${__target_var}=\"${__res:-0}\""
}

config_handler() {
	# ==========================================
	# In connac 2, SOC and PCIE card could be in one DUT.
	# Therefore, the SOC and PCIe card configurations should be separated.
	# ==========================================
	[ "${file}" = "${DUT_CONFIG_FILE}" -o "${connac_ver}" != "2" ] && return

	[ "${phy_idx}" -ge "${pri_chip_bands}" ] && file="${file}_soc"
}

write_file() {
	local __var_name
	local __val

	for __var_name in "$@"; do
		eval "__val=\"\${${__var_name}}\""
		printf "%s='%s'\n" "${__var_name}" "${__val}"
	done > "${file}"
}

record_config() {
	# ==========================================
	# Usage:
	#	record_config "${config}" "${value}"
	#	record_config "${file}"
	#
	# Description:
	#	Record the configurations in the specified file.
	#	Instead of searching and replacing individual
	#	configurations, overwriting the entire configuration is faster.
	# ==========================================
	local config="$1"
	local val="$2"
	local file

	[ "$#" -ge 2 ] && eval "${config}=\"\${val}\""
	case "${config}" in
		"connac_ver"|"pri_chip_bands"|"sec_chip_bands"|"work_mode"|"${DUT_CONFIG_FILE}")
			file="${DUT_CONFIG_FILE}"
			write_file "connac_ver" "pri_chip_bands" "sec_chip_bands" "work_mode"
			;;
		"ATECTRLBANDIDX"|"ATETXBW"|"ATEDA"|"ATESA"|"ATEBSSID"| \
		"ATETXMODE"|"${SETTING_FILE}")
			file="${SETTING_FILE}"
			config_handler
			write_file "ATECTRLBANDIDX" "ATETXBW" "ATEDA" \
				   "ATESA" "ATEBSSID" "ATETXMODE"
			;;
		*)
			print_err "Invalid config ${config} to record"
			;;
	esac
}

get_config() {
	# ==========================================
	# Usage:
	#	get_config "${file}"
	#
	# Description:
	#	Get all the cached configurations written in specified file
	# ==========================================
	local file="$1"

	config_handler
	[ -f "${file}" ] && . "${file}"
}

split() {
	# ==========================================
	# Usage:
	#	split "${input}" "${delimiter}" "${dst1}" "${dst2}" ...
	#
	# Description:
	#	Splits the input string by the specified delimiter and
	#	stores the results into the subsequent destination variables
	# ==========================================
	local input="$1"
	local delim="$2"

	shift 2
	local var_names="$*"

	local old_ifs="$IFS"
	IFS="${delim}"
	set -- ${input}
	IFS="$old_ifs"

	local var
	local idx=1
	local part

	for var in ${var_names}; do
		# part=$1, $2, ...
		eval "part=\"\${$idx}\""

		if [ -n "${part}" ]; then
			eval "${var}=\"${part}\""
		else
			eval "${var}=\"\""
		fi

		idx=$((idx + 1))
	done
}

mt76_debugfs_path() {
	local __path="/sys/kernel/debug/ieee80211/phy${main_phy_idx}/mt76"
	local __target_var="$1"
	local __band="$2"

	[ -n "${__band}" ] && __path="${__path}/${__band}"
	__path="${__path}/$1"

	eval "${__target_var}=\"${__path}\""
}

get_soc_phy_idx() {
	local __target_var="$1"
	local __band="$2"
	local __phy_idx="${band}"

	# If secondary chip exists, then the SOC phy idx must
	# be the primary chip band number + desired band
	[ "${sec_chip_bands}" != "0" ] &&
		__phy_idx=$((__phy_idx + pri_chip_bands))

	eval "${__target_var}=\"${__phy_idx}\""
}

dump_sku() {
	echo "
Interface Conversion Failed!
Please configure the sku of your board manually by the following commands
For AX3000/AX6000:
	echo connac_ver=2 >> ${DUT_CONFIG_FILE}
	echo pri_chip_bands=2 >> ${DUT_CONFIG_FILE}
	echo sec_chip_bands=0 >> ${DUT_CONFIG_FILE}
For AX7800:
	echo connac_ver=2 >> ${DUT_CONFIG_FILE}
	echo pri_chip_bands=2 >> ${DUT_CONFIG_FILE}
	echo sec_chip_bands=2 >> ${DUT_CONFIG_FILE}
For AX8400:
	echo connac_ver=2 >> ${DUT_CONFIG_FILE}
	echo pri_chip_bands=1 >> ${DUT_CONFIG_FILE}
	echo sec_chip_bands=2 >> ${DUT_CONFIG_FILE}
For MT7996:
	echo connac_ver=3 >> ${DUT_CONFIG_FILE}
	echo pri_chip_bands=3 >> ${DUT_CONFIG_FILE}
	echo sec_chip_bands=0 >> ${DUT_CONFIG_FILE}
For MT7992:
	echo connac_ver=3 >> ${DUT_CONFIG_FILE}
	echo pri_chip_bands=2 >> ${DUT_CONFIG_FILE}
	echo sec_chip_bands=0 >> ${DUT_CONFIG_FILE}
For MT7990:
	echo connac_ver=3 >> ${DUT_CONFIG_FILE}
	echo pri_chip_bands=2 >> ${DUT_CONFIG_FILE}
	echo sec_chip_bands=0 >> ${DUT_CONFIG_FILE}
For MT7999:
	echo connac_ver=5 >> ${DUT_CONFIG_FILE}
	echo pri_chip_bands=3 >> ${DUT_CONFIG_FILE}
	echo sec_chip_bands=0 >> ${DUT_CONFIG_FILE}"
}

parse_sku() {
	local sku_table="
		7916:2:2:2
		7915:2:1:2
		7981:2:2:0
		7986:2:2:0
		7990:3:3:0
		7992:3:2:0
		7993:3:2:0
		80f2:5:3:0
	"
	local item eeprom chip _

	mt76_debugfs_path "eeprom"
	get_config "${DUT_CONFIG_FILE}"

	[ -n "${pri_chip_bands}" ] && [ -n "${sec_chip_bands}" ] && [ -n "${connac_ver}" ] &&
		return
	[ ! -f "${eeprom}" ] && print_err "eeprom file not found"

	chip=$(head -c 2 "${eeprom}" | hexdump -ve '/2 "%04x"')
	for item in ${sku_table}; do
		case "${item}" in
			${chip}:*)
				split "${item}" ":" "_" "connac_ver" "pri_chip_bands" \
				      "sec_chip_bands"
				record_config "${DUT_CONFIG_FILE}"
				return 0
				;;
		esac
	done

	dump_sku
	exit 1
}

convert_interface() {
	local band_idx

	if [ "${connac_ver}" != "2" ]; then
		# Connac 3/5 chips mapping rule
		# phy0: ra0
		# phy1: rai0
		# phy2: rax0
		case "${interface}" in
			rai*)
				phy_idx="1"
				;;
			rax*)
				phy_idx="2"
				;;
			*)
				phy_idx="0"
				;;
		esac

		interface="phy${phy_idx}"
		return
	fi

	case "${interface}" in
		# Connac 2 chips mapping rule
		# raix: PCIE card band 1
		# rai: PCIE card band 0
		# rax: SOC band 1
		# ra: SOC band 0
		raix*)
			[ "${pri_chip_bands}" -gt "1" ] && phy_idx="1"
			;;
		rai*)
			phy_idx="0"
			;;
		rax*)
			get_soc_phy_idx "phy_idx" "1"
			;;
		*)
			get_soc_phy_idx "phy_idx" "0"
			;;
	esac

	# convert phy index according to band idx
	band_idx="${ATECTRLBANDIDX}"
	if [ "${band_idx}" = "0" ]; then
		case "${interface}" in
			raix*)
				phy_idx="0"
				;;
			rax*)
				get_soc_phy_idx "phy_idx" "0"
				;;
		esac
	elif [ "${band_idx}" = "1" ]; then
		case "${interface}" in
			rai*)
				# AX8400: only has one band for mt7915, so remain phy0
				# AX7800: mt7916 becomes phy1
				[ "${pri_chip_bands}" -gt "1" ] && phy_idx="1"
				;;
			ra*)
				get_soc_phy_idx "phy_idx" "1"
				;;
		esac
	fi

	interface="phy${phy_idx}"
}
