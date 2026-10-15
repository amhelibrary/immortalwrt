[ ! -e /etc/fw_env.config ] && exit 0

. /lib/functions/system.sh

case "$(board_name)" in
bananapi,bpi-r2|\
bananapi,bpi-r64|\
unielec,u7623-02)
	[ -z "$(fw_printenv -n ethaddr 2>/dev/null)" ] &&
		fw_setenv ethaddr "$(cat /sys/class/net/eth0/address)"
	;;
bananapi,bpi-r3|\
bananapi,bpi-r3-mini)
	[ -z "$(fw_printenv -n ethaddr 2>/dev/null)" ] &&
		fw_setenv ethaddr "$(cat /sys/class/net/eth0/address)"
	[ -z "$(fw_printenv -n eth1addr 2>/dev/null)" ] &&
		fw_setenv eth1addr "$(macaddr_add $(cat /sys/class/net/eth0/address) 1)"
	;;
bananapi,bpi-r4*)
	is_bad_crc=0
	if fw_printenv 2>&1 | grep -qi 'bad crc'; then
		is_bad_crc=1
	fi
	cur_bootconf="$(fw_printenv -n bootconf 2>/dev/null)"
	cur_bootcmd="$(fw_printenv -n bootcmd 2>/dev/null)"

	if [ "$is_bad_crc" -eq 1 ] || [ -z "$cur_bootconf" ] || [ "$cur_bootcmd" = 'run distro_bootcmd' ]; then
		bootdev="$(grep -l "u-boot-env" /sys/block/mmcblk*/mmcblk*p*/uevent 2>/dev/null | head -n 1)"
		bootdev="${bootdev#/sys/block/}"
		bootdev="${bootdev%%/*}"
		[ -z "$bootdev" ] && bootdev="mmcblk0"
		dev_type="$(cat /sys/block/${bootdev}/device/type 2>/dev/null)"

		case "$(board_name)" in
		bananapi,bpi-r4-poe*)
			if [ "$dev_type" = "MMC" ]; then
				bconf="config-mt7988a-bananapi-bpi-r4-poe#mt7988a-bananapi-bpi-r4-poe-emmc"
			else
				bconf="config-mt7988a-bananapi-bpi-r4-poe#mt7988a-bananapi-bpi-r4-poe-sd"
			fi
			;;
		*)
			if [ "$dev_type" = "MMC" ]; then
				bconf="config-mt7988a-bananapi-bpi-r4#mt7988a-bananapi-bpi-r4-emmc"
			else
				bconf="config-mt7988a-bananapi-bpi-r4#mt7988a-bananapi-bpi-r4-sd"
			fi
			;;
		esac

		fw_setenv -s - <<-EOF 2>/dev/null
			bootconf $bconf
			bootdelay 2
			baudrate 115200
			loadaddr 0x44000000
			ipaddr 192.168.1.1
			serverip 192.168.1.2
			netmask 255.255.255.0
			bootcmd
			ipaddr2
			ipaddr3
			ipaddr5
			ipaddr6
			ipaddr7
			eth6addr
		EOF
	fi

	cur_ethaddr="$(fw_printenv -n ethaddr 2>/dev/null)"
	cur_eth1addr="$(fw_printenv -n eth1addr 2>/dev/null)"
	[ -z "$cur_ethaddr" ] || [ "$cur_ethaddr" = "02:00:11:22:33:44" ] && \
		fw_setenv ethaddr "$(cat /sys/class/net/eth0/address)"
	[ -z "$cur_eth1addr" ] && \
		fw_setenv eth1addr "$(macaddr_add $(cat /sys/class/net/eth0/address) 1)"
	;;
esac

exit 0
