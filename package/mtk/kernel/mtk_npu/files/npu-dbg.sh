#!/bin/bash

#1: base
#2: number of regs for every 0x100 register map
function dump_reg() {
	for i in $(seq 0 $2)
	do
		reg=$(printf '%x\n' $(($1 + 0x100 * ${i})))
		echo ${reg}
		regs d ${reg}
	done
}

echo "============ TDMA ============"
dump_reg 0x15106000 3

echo "============ TDMA Tx Desc ============"
dump_reg 0x09234580 26

echo "============ TDMA Rx Desc #0 ============"
dump_reg 0x09818000 7
echo "============ TDMA Rx Desc #1 ============"
dump_reg 0x0981b250 7
echo "============ TDMA Rx Desc #2 ============"
dump_reg 0x0981e4a0 7
echo "============ TDMA Rx Desc #3 ============"
dump_reg 0x098216f0 7

cat /proc/npu/tnl
cat /proc/npu/multicast/group
