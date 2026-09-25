#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright (C) 2025 MediaTek Inc.
#

usage() {
	cat << EOF
Usage: $0 [options]
  -r    dump rings
  -h    show this help
EOF
	exit 1
}

# options
FLAG_R=0

while getopts "rh" OPT; do
	case "$OPT" in
		r) FLAG_R=1	;;
		h) usage	;;
		?) usage	;;
	esac
done

shift $(( OPTIND - 1 ))

# helper functions
to_hex() { printf "0x%x" "$(( $1 ))"; }

# default values
DMAD_SZ_ADMA_RX=32
DMAD_SZ_WDMA_TX=32
DMAD_SZ_WDMA_RX=32
DMAD_SZ_TDMA_TX=32
DMAD_SZ_TDMA_RX=32

SOC=`cat /proc/device-tree/ethernet*/compatible | cut -c 10-15`
if [ -z "$SOC" ]; then
	SOC=`cat /proc/device-tree/soc/ethernet*/compatible | cut -c 10-15`
fi
if [ -z "$SOC" ]; then
	SOC=`cat /proc/device-tree/soc_netsys/ethernet*/compatible | cut -c 10-15`
fi
init()
{
	case $SOC in
	*7988)
		BASE_FE=15100000
		BASE_SGMII=10060000
		OFFSET_PSE="0 100 200 300 400 500 600 c00 d00 e00 f00
			    1400 1500 1600 2800"
		OFFSET_PPE="2200 2300 2600 2700 2e00 2f00"
		OFFSET_ADMA="6800 6900 6a00"
		OFFSET_RING_ADMA_RX="6900 6910 6920 6930 6940 6950 6960 6970"
		OFFSET_QDMA="4400 4500 4600 4700"
		OFFSET_WDMA="4800 4900 4a00 4b00"
		OFFSET_WDMA1="4c00 4d00 4e00 4f00"
		OFFSET_WDMA2="5000 5100 5200 5300"
		OFFSET_RING_WDMA_TX="4800 4810 4820 4830 4c00 4c10 4c20 4c30 5000 5010 5020 5030"
		OFFSET_RING_WDMA_RX="4900 4910 4d00 4d10 5100 5110"
		OFFSET_TDMA="6000 6100 6200 6300"
		OFFSET_RING_TDMA_TX="6000"
		OFFSET_RING_TDMA_RX="6100 6110 6120 6130"
		OFFSET_GMAC="10000 10100 10200"
		OFFSET_XFI="12000 12100 12200 13000 13100 13200"
		OFFSET_LRO="6c00 6d00"
		OFFSET_RSS="7000"
		OFFSET_SGMII="0 100 10000 10100"
		;;
	*7987)
		BASE_FE=15100000
		BASE_SGMII=10060000
		OFFSET_PSE="0 100 200 300 400 500 600 c00 d00 e00 f00
			    1400 1500 1600"
		OFFSET_PPE="2200 2300 2600 2700"
		OFFSET_ADMA="6800 6900 6a00"
		OFFSET_RING_ADMA_RX="6900 6910 6920 6930 6940 6950 6960 6970"
		OFFSET_QDMA="4400 4500 4600 4700"
		OFFSET_WDMA="4800 4900 4a00 4b00"
		OFFSET_RING_WDMA_TX="4800 4810 4820 4830"
		OFFSET_RING_WDMA_RX="4900 4910"
		OFFSET_GMAC="10000 10100 10200"
		OFFSET_XFI="12000 12100 12200"
		OFFSET_LRO="6c00 6d00"
		OFFSET_RSS="7000"
		OFFSET_SGMII="0 100 10000 10100"
		;;
	*798*)
		BASE_FE=15100000
		BASE_SGMII=10060000
		OFFSET_PSE="0 100 200 300 400 500 1400 1500 1600 2800"
		OFFSET_PPE="2200 2300"
		OFFSET_ADMA="4000 4100 4200 4300"
		OFFSET_RING_ADMA_RX="4100 4110 4120 4130"
		DMAD_SZ_ADMA_RX=16
		OFFSET_QDMA="4400 4500 4600 4700"
		OFFSET_WDMA="4800 4900 4a00 4b00 4c00 4d00 4e00 4f00"
		OFFSET_RING_WDMA_TX="4800 4810 4820 4830"
		OFFSET_RING_WDMA_RX="4900 4910"
		OFFSET_GMAC="10000 10100 10200"
		OFFSET_LRO="6400 6500"
		OFFSET_RSS="3000 6800"
		OFFSET_SGMII="0 100 10000 10100"
		;;
	*762*)
		BASE_FE=1b100000
		BASE_SGMII=1b128000
		OFFSET_PSE="0 100 200 300 400 500 1400 1500 1600"
		OFFSET_PPE="e00 f00 1000"
		OFFSET_ADMA="800 900 a00"
		OFFSET_QDMA="1800 1900 1a00 1b00"
		OFFSET_WDMA="2800 2900 2a00 2b00 2c00 2d00 2e00 2f00"
		OFFSET_GMAC="10000 10100 10200"
		OFFSET_LRO="b00"
		OFFSET_RSS="3000"
		OFFSET_SGMII="0 100"
		;;
	esac
	echo "============== SoC Info =============="
	echo "SoC: $SOC"
	echo "BASE_FE: 0x$BASE_FE"
	echo "BASE_SGMII: 0x$BASE_SGMII"
	echo
}
dump_basic()
{
	echo "============== Basic Information =============="
	cat /proc/version
	cat /proc/interrupts
	cat /proc/mtketh/dbg_regs
	cat /proc/mtketh/esw_cnt
	switch vlan dump
	switch reg r 1fc0
	switch reg r 3600
}
dump_ring()
{
	local NAME=$1
	local BASE=$2
	local OFFSET_RING=$3
	local DMAD_SZ=$4
	if [ -z "$BASE" ] || [ "$BASE" = "0" ]; then
		return
	fi

	local ADDR
	local END_ADDR
	local BASE_PTR
	local MAX_CNT
	local CTX_IDX
	local DTX_IDX
	local SZ_PER_DUMP=0x100
	local idx=0
	local val
	for val in $OFFSET_RING
	do
		echo "============== $NAME: RING $idx =============="
		ADDR=$(to_hex "0x$BASE + 0x$val")
		BASE_PTR=$(regs r $(to_hex "$ADDR + 0x0") | awk '/Value at/ { print $NF }')
		MAX_CNT=$(regs r $(to_hex "$ADDR + 0x4") | awk '/Value at/ { print $NF }')
		CTX_IDX=$(regs r $(to_hex "$ADDR + 0x8") | awk '/Value at/ { print $NF }')
		DTX_IDX=$(regs r $(to_hex "$ADDR + 0xc") | awk '/Value at/ { print $NF }')
		echo "BASE_PTR=$BASE_PTR MAX_CNT=$MAX_CNT CTX_IDX=$CTX_IDX DTX_IDX=$DTX_IDX"
		ADDR=$(( BASE_PTR ))
		END_ADDR=$(( BASE_PTR + DMAD_SZ * MAX_CNT ))
		while [ "$(( ADDR ))" -lt "$(( END_ADDR ))" ]; do
			printf "[$NAME: RING $idx DMAD %04d/%04d]\n" \
				"$(( (ADDR - BASE_PTR) / DMAD_SZ ))" \
				"$((MAX_CNT))"
			regs d "$(to_hex $ADDR)"
			ADDR=$(( ADDR + SZ_PER_DUMP ))
		done
		echo
		idx=$((idx + 1))
	done
}
dump_netsys()
{
	local NAME=$1
	local BASE=$2
	local OFFSET=$3
	if [ $BASE == 0 ]; then
		return
	fi
	echo "============== $NAME =============="
	for i in $OFFSET
	do
		ADDR=`printf '%x' $((0x$BASE + 0x$i))`
		regs d $ADDR
		echo
	done
}
init
dump_basic
dump_netsys	"PSE"		$BASE_FE	"$OFFSET_PSE"
dump_netsys	"PPE"		$BASE_FE	"$OFFSET_PPE"
dump_netsys	"ADMA"		$BASE_FE	"$OFFSET_ADMA"
if [ "$FLAG_R" -eq 1 ]; then
	dump_ring	"ADMA_RX"	$BASE_FE	"$OFFSET_RING_ADMA_RX"	"$DMAD_SZ_ADMA_RX"
fi
dump_netsys	"QDMA"		$BASE_FE	"$OFFSET_QDMA"
dump_netsys	"WDMA"		$BASE_FE	"$OFFSET_WDMA"
dump_netsys	"WDMA1"		$BASE_FE	"$OFFSET_WDMA1"
dump_netsys	"WDMA2"		$BASE_FE	"$OFFSET_WDMA2"
if [ "$FLAG_R" -eq 1 ]; then
	dump_ring	"WDMA_TX"	$BASE_FE	"$OFFSET_RING_WDMA_TX"	"$DMAD_SZ_WDMA_TX"
	dump_ring	"WDMA_RX"	$BASE_FE	"$OFFSET_RING_WDMA_RX"	"$DMAD_SZ_WDMA_RX"
fi
dump_netsys	"TDMA"		$BASE_FE	"$OFFSET_TDMA"
if [ "$FLAG_R" -eq 1 ]; then
	dump_ring	"TDMA_TX"	$BASE_FE	"$OFFSET_RING_TDMA_TX"	"$DMAD_SZ_TDMA_TX"
	dump_ring	"TDMA_RX"	$BASE_FE	"$OFFSET_RING_TDMA_RX"	"$DMAD_SZ_TDMA_RX"
fi
dump_netsys	"GMAC"		$BASE_FE	"$OFFSET_GMAC"
dump_netsys	"XFI"		$BASE_FE        "$OFFSET_XFI"
dump_netsys	"LRO"		$BASE_FE	"$OFFSET_LRO"
dump_netsys	"RSS"		$BASE_FE	"$OFFSET_RSS"
dump_netsys	"SGMII"		$BASE_SGMII	"$OFFSET_SGMII"
