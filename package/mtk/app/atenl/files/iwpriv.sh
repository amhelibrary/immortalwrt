#!/bin/ash
# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright (C) 2026 MediaTek Inc.
#

. "/lib/atenl/utils.sh"

change_band_idx() {
	local new_idx="$1"
	local new_phy_idx="${phy_idx}"
	local old_idx="${ATECTRLBANDIDX}"

	[ "${connac_ver}" != "2" ] && return

	case "${interface_ori}" in
		"ra"*)
			if [ -z "${old_idx}" ] || [ "${old_idx}" != "${new_idx}" ]; then
				case "${new_idx}:${interface_ori}" in
					"0:rai"*)
						# raix0 & rai0 becomes rai0
						new_phy_idx="0"
						;;
					"0:ra"*)
						# rax0 & ra0 becomes ra0
						get_soc_phy_idx "new_phy_idx" "0"
						;;
					"1:rai"*)
						# raix0 & rai0 becomes raix0
						# For AX8400, primary chip bands is 1,
						# so keep the original phy_idx
						[ "${pri_chip_bands}" -gt "1" ] && new_phy_idx="1"
						;;
					"1:ra"*)
						# rax0 & ra0 becomes rax0
						get_soc_phy_idx "new_phy_idx" "1"
						;;
				esac
			fi

			[ "${new_phy_idx}" != "${phy_idx}" ] && {
				ate_stop
				phy_idx="${new_phy_idx}"
				interface="phy${phy_idx}"
				ate_start
			}
			;;
	esac

	record_config "ATECTRLBANDIDX" "${new_idx}"
}

simple_convert() {
	local target_var="$1"
	local res=""

	case "${cmd}" in
		"ATETXCNT")
			res="tx_count"
			;;
		"ATETXLEN")
			res="tx_length"
			;;
		"ATETXMCS")
			res="tx_rate_idx"
			;;
		"ATEVHTNSS"|"ATETXNSS")
			res="tx_rate_nss"
			;;
		"ATETXLDPC")
			res="tx_rate_ldpc"
			;;
		"ATETXSTBC")
			res="tx_rate_stbc"
			;;
		"ATEPKTTXTIME")
			res="tx_time"
			;;
		"ATEIPG")
			res="tx_ipg"
			;;
		"ATEDUTYCYCLE")
			res="tx_duty_cycle"
			;;
		"ATETXFREQOFFSET")
			res="freq_offset"
			;;
		*)
			print_err "Invalid cmd ${cmd} for simple convert"
			;;
	esac

	eval "${target_var}=\"${res}\""
}

convert_tx_mode() {
	local __target_var="$1"
	local __tx_mode="$2"
	local __ebf="$3"
	local __res=""

	# if the second argument is not passed, then get the val of the __target_var
	# In this case, the __target_var should not be empty
	[ -z "${__tx_mode}" ] && eval "__tx_mode=\"\${${__target_var}}\""

	remove_leading_zeros "__tx_mode"
	case "${__tx_mode}" in
		0)
			__res="cck"
			;;
		1)
			__res="ofdm"
			;;
		2)
			__res="ht"
			;;
		4)
			__res="vht"
			;;
		8)
			__res="he_su"
			;;
		9)
			__res="he_er"
			;;
		10)
			__res="he_tb"
			;;
		11)
			__res="he_mu"
			;;
		13)
			__res="eht_su"
			;;
		15)
			__res="eht_mu"
			[ -n "${__ebf}" ] && __res="eht_su"
			;;
		16)
			__res="eht_tb"
			[ -n "${__ebf}" ] && __res="uhr_su"
			;;
		17)
			__res="uhr_su"
			;;
		18)
			__res="uhr_mu"
			;;
		19)
			__res="uhr_tb"
			;;
		*)
			print_err "unknown tx mode: ${__tx_mode}"
			;;
	esac

	eval "${__target_var}=\"${__res}\""
}

convert_gi() {
	local tx_mode="$1"
	local val="$2"
	local sgi="0"
	local he_ltf="0"

	case "${tx_mode}" in
		"ht"|"vht")
			sgi="${val}"
			;;
		"he_su"|"he_er")
			case "${val}" in
				"0")
					;;
				"1")
					he_ltf="1"
					;;
				"2")
					sgi="1"
					he_ltf="1"
					;;
				"3")
					sgi="2"
					he_ltf="2"
					;;
				"4")
					he_ltf="2"
					;;
				*)
					print_err "unknown gi: ${val}"
					;;
			esac
			;;
		"he_mu")
			case "${val}" in
				"0")
					he_ltf="2"
					;;
				"1")
					he_ltf="1"
					;;
				"2")
					sgi="1"
					he_ltf="1"
					;;
				"3")
					sgi="2"
					he_ltf="2"
					;;
				*)
					print_err "unknown gi: ${val}"
					;;
			esac
			;;
		"he_tb")
			case "${val}" in
				"0")
					sgi="1"
					;;
				"1")
					sgi="1"
					he_ltf="1"
					;;
				"2")
					sgi="2"
					he_ltf="2"
					;;
				*)
					print_err "unknown gi: ${val}"
					;;
			esac
			;;
		*)
			print_err "legacy mode no need gi"
			;;
	esac

	mt76_set_cmd "tx_rate_sgi=${sgi} tx_ltf=${he_ltf}"
}

convert_tm_cbw_to_nl() {
	local __target_var="$1"
	local __cbw="$2"
	local __res

	# if the second argument is not passed, then get the val of the __target_var
	[ -z "${__cbw}" ] && eval "__cbw=\"\${${__target_var}}\""
	# if the original value of __target_var is "", then simply return
	[ -z "${__cbw}" ] && return

	case "${__cbw}" in
		# TM_CBW_20MHZ
		"0")
			__res="20"
			;;
		# TM_CBW_40MHZ
		"1")
			__res="40"
			;;
		# TM_CBW_80MHZ
		"2")
			__res="80"
			;;
		# TM_CBW_160MHZ
		"5")
			__res="160"
			;;
		# TM_CBW_320MHZ
		"12")
			__res="320"
			;;
		*)
			print_err "Invalid bandwidth value ${__cbw}"
			;;
	esac

	eval "${__target_var}=\"${__res}\""
}

convert_bw() {
	local system_bw data_bw bw tx_pkt_bw

	split "$1" ":" "system_bw" "data_bw"
	# Convert TM_CBW to NL80211_CHAN_WIDTH
	convert_tm_cbw_to_nl "bw" "${system_bw}"
	# data_bw could be empty
	convert_tm_cbw_to_nl "tx_pkt_bw" "${data_bw}"

	record_config "ATETXBW" "${bw}"

	# apply per-packet bw
	[ -n "${data_bw}" ] && mt76_set_cmd "tx_pkt_bw=${tx_pkt_bw}"
}

handle_ctrl_band_idx() {
	local ctrl_band_idx="${ATECTRLBANDIDX}"

	[ -z "${ctrl_band_idx}" ] && return

	if [ "${ctrl_band_idx}" = "1" ] && [ "${band}" = "0" ]; then
		local file="/etc/config/wireless"
		local count=0
		local line tmp

		[ ! -f "${file}" ] && {
			do_cmd "wifi config"
			[ ! -f "${file}" ] && print_err "Cannot find ${file}"
		}

		while read -r line; do
			case "${line}" in
				*"option band"*)
					count=$((count+1))
					[ "${count}" != "$((phy_idx+1))" ] && continue

					tmp="${line#*\'}"
					tmp="${tmp%\'*}"
					break
					;;
			esac
		done < "${file}"

		# get the channel band of the ${phy_idx}-th wiphy
		if [ "${tmp}" = "2g" ]; then
			band="0"
		elif [ "${tmp}" = "5g" ]; then
			band="1"
		elif [ "${tmp}" = "6g" ]; then
			band="2"
		else
			print_err "wrapper band translation error!"
		fi
	else
		[ "${pri_chip_bands}" -gt "1" -o "${phy_idx}" != "0" ] &&
			band=$((ctrl_band_idx * band))
	fi
}

convert_channel() {
	local center_ch band pri_sel center_ch2 fast_cal
	local base_freq control_freq center_freq control_ch
	local base_chan=1
	local fast_cal_type="none"
	local bw="${ATETXBW}"

	[ -z "${bw}" ] && bw="20"

	split "$1" ":" "center_ch" "band" "pri_sel" "center_ch2" "fast_cal"
	remove_leading_zeros "fast_cal"

	handle_ctrl_band_idx

	[ -n "${pri_sel}" ] && {
		[ "${pri_sel}" -gt $(((bw / 20) - 1)) ] &&
			print_err "Invalid primary selection for ${bw} MHz: ${pri_sel}"

		mt76_set_cmd "tx_pri_sel=${pri_sel}"
	}

	[ -n "${fast_cal}" ] && {
		case "${fast_cal}" in
			"200000")
				fast_cal_type="rx_verify"
				;;
			"400000")
				fast_cal_type="power_cal"
				;;
		esac
		mt76_set_cmd "fast_cal=${fast_cal_type}"
	}

	if [ -n "${band}" ] && [ "${band}" -gt 0 ]; then
		if [ "${band}" = "1" ]; then
			base_freq=5180
			base_chan=36
		else
			base_freq=5955
		fi
	else
		[ "${bw}" = "40" ] && [ "${center_ch}" -lt "3" -o "${center_ch}" -gt "12" ] &&
			print_err "Invalid center_ch ${center_ch} for 2G 40MHz"
		base_freq=2412
	fi

	center_freq=$(((center_ch - base_chan) * 5 + base_freq))
	control_ch=$((center_ch - bw / 10 + 2))
	[ -n "${pri_sel}" ] && control_ch=$((control_ch + pri_sel * 4))
	control_freq=$(((control_ch - base_chan) * 5 + base_freq))

	ch_cmd="iw dev mon${phy_idx} set freq ${control_freq} ${bw}"
	[ "${center_freq}" != "${control_freq}" ] && ch_cmd="${ch_cmd} ${center_freq}"
	do_cmd "${ch_cmd}"
}

convert_rxstat() {
	local mdrdy fcs_error rx_ok rssi rcpi ib_rssi wb_rssi
	local res formatted_res

	query_cmd "res" "mt76-test ${interface} dump stats"
	formatted_res=$(echo "${res}" | awk -F'=' '
		$1 ~ /rx_packets/	{ mdrdy = $2 }
		$1 ~ /rx_fcs_error/	{ fcs_error = $2 }
		$1 ~ /rx_success/	{ rx_ok = $2 }
		$1 ~ /last_rssi/	{ gsub(/,/, " ", $2); rssi = $2 }
		$1 ~ /last_rcpi/	{ gsub(/,/, " ", $2); rcpi = $2 }
		$1 ~ /last_ib_rssi/	{ gsub(/,/, " ", $2); ib_rssi = $2 }
		$1 ~ /last_wb_rssi/	{ gsub(/,/, " ", $2); wb_rssi = $2 }
		END {
			print mdrdy ":" fcs_error ":" rx_ok ":" rssi ":" \
			      rcpi ":" ib_rssi ":" wb_rssi
		}
	')

	split "${formatted_res}" ":" "mdrdy" "fcs_error" "rx_ok" "rssi" "rcpi" "ib_rssi" "wb_rssi"

	write_dmesg "rssi: ${rssi}"
	write_dmesg "rcpi: ${rcpi}"
	write_dmesg "fagc rssi ib: ${ib_rssi}"
	write_dmesg "fagc rssi wb: ${wb_rssi}"
	write_dmesg "all_mac_rx_mdrdy_cnt: ${mdrdy}"
	write_dmesg "all_mac_rx_fcs_err_cnt: ${fcs_error}"
	write_dmesg "all_mac_rx_ok_cnt : ${rx_ok}"
}

set_mac_addr() {
	local addr1 addr2 addr3
	record_config "${cmd}" "${param}"
	get_config "${SETTING_FILE}"
	addr1="${ATEDA}"
	addr2="${ATESA}"
	addr3="${ATEBSSID}"

	[ -z "${addr1}" ] && addr1="00:11:22:33:44:55"
	[ -z "${addr2}" ] && addr2="00:11:22:33:44:55"
	[ -z "${addr3}" ] && addr3="00:11:22:33:44:55"

	mt76_set_cmd "mac_addrs=${addr1},${addr2},${addr3}"
}

convert_bf() {
	local new_param="${param//:/,}"
	local new_cmd

	case "${cmd}" in
		"ATETxBfInit")
			new_cmd="init"
			new_param="1"
			mt76_set_cmd "state=idle"
			;;
		"ATETxBfGdInit")
			new_cmd="golden_init"
			new_param="1"
			mt76_set_cmd "state=idle"
			;;
		"ATEIBFPhaseComp")
			new_cmd="phase_comp"
			;;
		"ATEEBfProfileConfig")
			new_cmd="ebf_prof_update"
			;;
		"ATEIBfProfileConfig")
			new_cmd="ibf_prof_update"
			;;
		"ATEIBfInstCal")
			new_cmd="phase_cal"
			;;
		"ATEIBfGdCal")
			local group group_l_m_h band_idx cal_type ver
			local lna_level="0"

			new_cmd="phase_cal"
			split "${new_param}" "," "group" "group_l_m_h" "band_idx" \
			      "cal_type" "version"

			# only ibf 2.0 will set version, so add null check for
			# backward compatibility
			new_param="${group},${group_l_m_h},${band_idx}"
			new_param="${new_param},${cal_type},${lna_level},${version:-00}"
			;;
		"TxBfTxApply")
			new_cmd="apply_tx"
			;;
		"ATETxPacketWithBf")
			local bf_on wlan_idx tx_count
			local aid="1"
			local update="0"

			new_cmd="tx_prep"
			split "${new_param}" "," "bf_on" "wlan_idx" "tx_count"

			# continuous TX
			[ "${tx_count}" = "00" ] && tx_count="10000000"

			new_param="${bf_on},${aid},${wlan_idx},${update}"
			new_param="${new_param} aid=1 tx_length=1024 tx_count=${tx_count}"
			mt76_set_cmd "state=idle"
			;;
		"TxBfProfileData20MAllWrite")
			new_cmd="prof_update_all"
			;;
		"ATEIBFPhaseE2pUpdate")
			new_cmd="e2p_update"
			;;
		"ATEIBFPhaseVerify")
			local group group_l_m_h band_idx phase_cal_type
			local lna_gain_level read_from_e2p
			local phase_comp_param

			new_cmd="phase_cal"
			split "${new_param}" "," "group" "group_l_m_h" "band_idx" \
			      "phase_cal_type" "lna_gain_level" "read_from_e2p"

			phase_comp_param="txbf_param=1,${band_idx},${group},${read_from_e2p},0"
			mt76_set_cmd "txbf_act=phase_comp ${phase_comp_param}"

			new_param="${group},${group_l_m_h},${band_idx}"
			new_param="${new_param},${phase_cal_type},${lna_gain_level}"
			;;
		"TxBfProfileTagRead")
			new_cmd="pfmu_tag_read"
			;;
		"TxBfProfileTagWrite")
			new_cmd="pfmu_tag_write"
			;;
		"TxBfProfileTagInValid")
			new_cmd="set_invalid_prof"
			;;
		"StaRecBfRead")
			new_cmd="sta_rec_read"
			;;
		"TriggerSounding")
			new_cmd="trigger_sounding"
			;;
		"StopSounding")
			new_cmd="stop_sounding"
			new_param="0"
			;;
		"TxBfTxCmd")
			new_cmd="txcmd"
			;;
		"ATEConTxETxBfGdProc")
			local tx_rate_mode tx_rate_idx bw channel channel2 band

			new_cmd="ebf_golden_init"
			split "${new_param}" "," "tx_rate_mode" "tx_rate_idx" \
			      "bw" "channel" "channel2" "band"
			convert_tx_mode "tx_rate_mode" "" "1"

			remove_leading_zeros "bw"
			convert_tm_cbw_to_nl "bw"
			remove_leading_zeros "channel"
			remove_leading_zeros "channel2"
			remove_leading_zeros "tx_rate_idx"

			ate_start
			mt76_set_cmd "state=idle"

			record_config "ATETXBW" "${bw}"
			convert_channel "${channel}:${band}"

			key_val_pair "tx_rate_mode" "tx_rate_idx"
			mt76_set_cmd "${tx_rate_mode} ${tx_rate_idx} tx_rate_sgi=0"

			new_param="1,0"
			[ "${bw}" = "5" ] && new_param="1,1"
			;;
		"ATEConTxETxBfInitProc")
			local ebf_tx_vars="tx_rate_mode tx_rate_idx tx_rate_nss \
					   tx_power tx_length tx_antenna"
			local bw channel channel2 band
			local tx_stream tx_args=""

			# declare all the variables in ebf_tx_vars
			local ${ebf_tx_vars}

			new_cmd="ebf_init"
			split "${new_param}" "," "tx_rate_mode" "tx_rate_idx" \
				"bw" "tx_rate_nss" "tx_stream" "tx_power" "channel" \
				"channel2" "band" "tx_length"
			convert_tx_mode "tx_rate_mode" "" "1"

			remove_leading_zeros "bw"
			convert_tm_cbw_to_nl "bw"
			remove_leading_zeros "channel"
			remove_leading_zeros "channel2"
			remove_leading_zeros "tx_length"
			tx_antenna=$((2 ** tx_stream - 1))

			ate_start
			mt76_set_cmd "state=idle"

			record_config "ATETXBW" "${bw}"
			convert_channel "${channel}:${band}"

			tx_power="${tx_power},0,0,0"
			tx_args="tx_rate_sgi=0 tx_rate_ldpc=1 tx_count=10000000 tx_ipg=4"
			concat_vars "tx_args" "${ebf_tx_vars}"
			mt76_set_cmd "${tx_args}"
			new_param="1"
			;;
		*)
	esac

	mt76_set_cmd "txbf_act=${new_cmd} txbf_param=${new_param}"

	case "${cmd}" in
		"ATETxBfInit"|"ATETxBfGdInit")
			mt76_set_cmd "aid=1"
			;;
		"ATETxPacketWithBf")
			mt76_set_cmd "state=tx_frames"
			;;
		"ATEIBFPhaseE2pUpdate")
			do_cmd "atenl -i ${interface} -c \"eeprom ibf sync\""
			;;
		"ATEConTxETxBfInitProc")
			local wlan_id="1"

			[ "${connac_ver}" != "2" ] && wlan_id=$((phy_idx+1))
			mt76_set_cmd "aid=1"
			mt76_set_cmd "txbf_act=stop_sounding txbf_param=1"
			mt76_set_cmd "txbf_act=update_ch txbf_param=1"
			mt76_set_cmd "txbf_act=ebf_prof_update txbf_param=0,0,0"
			mt76_set_cmd "txbf_act=apply_tx txbf_param=${wlan_id},1,0,0,0"
			[ "${connac_ver}" != "2" ] &&
				mt76_set_cmd "txbf_act=txcmd txbf_param=1,1,1"
			mt76_set_cmd "txbf_act=pfmu_tag_read txbf_param=0,1"
			mt76_set_cmd "txbf_act=sta_rec_read txbf_param=${wlan_id}"
			mt76_set_cmd "txbf_act=trigger_sounding txbf_param=0,1,0,${wlan_id},0,0,0"
			mt76_set_cmd "txbf_act=trigger_sounding txbf_param=2,1,ff,${wlan_id},0,0,0"
			mt76_set_cmd "state=rx_frames"
			;;
		"ATEConTxETxBfGdProc")
			mt76_set_cmd "aid=1"
			mt76_set_cmd "state=rx_frames"
			;;
		*)
	esac
}

parsing_ruinfo() {
	local ru_vars="ru_alloc aid ru_idx tx_rate_idx tx_rate_ldpc tx_rate_nss tx_length"
	local new_param="$1" ru_args=""
	local _

	# declare all the variables in ru_vars
	local ${ru_vars}

	split "${new_param}" "-" "ru_alloc" "aid" "ru_idx" "tx_rate_idx" \
	      "tx_rate_ldpc" "tx_rate_nss" "_" "tx_length"

	ru_args="tx_rate_mode=he_mu tx_rate_sgi=0 tx_ltf=0"
	concat_vars "ru_args" "${ru_vars}"
	mt76_set_cmd "${ru_args}"
}

convert_ruinfo() {
	local new_param="${param}"
	local tuple

	mt76_set_cmd "state=idle"

	local old_ifs="$IFS"; IFS=":"; set -- ${param}; IFS="$old_ifs"

	for tuple in "$@"; do
		[ "${#tuple}" -gt 2 ] && parsing_ruinfo "${tuple}"
	done
}

convert_dfs() {
	case "${cmd}" in
		"DfsRxCtrl"|"ATEDEDICATEDRX")
			local offchan_ch offchan_bw

			split "${param}" ":" "offchan_ch" "offchan_bw"

			offchan_bw=$(((offchan_bw + 1) * 20))

			mt76_set_cmd "state=idle"
			mt76_set_cmd "offchan_ch=${offchan_ch} offchan_bw=${offchan_bw}"
			;;
		"DfsRxHist")
			local ipi_th ipi_period ipi_antenna_idx ipi_cmd

			split "${param}" ":" "ipi_th" "ipi_period" "ipi_antenna_idx"

			ipi_cmd="ipi_threshold=${ipi_th} ipi_period=${ipi_period}"
			[ -n "${ipi_antenna_idx}" ] &&
				ipi_cmd="${ipi_cmd} ipi_antenna_idx=${ipi_antenna}"
			mt76_set_cmd "${ipi_cmd}"
			;;
	*)
	esac
}

ate_start() {
	if ifconfig "mon${phy_idx}" >/dev/null 2>&1; then
		print_debug "ATE already starts"
		return
	fi

	do_cmd "mt76-test ${interface} add mon${phy_idx}"
}

ate_stop() {
	local file="${SETTING_FILE}"

	if ! ifconfig "mon${phy_idx}" >/dev/null 2>&1; then
		print_debug "ATE does not start"
		return
	fi

	do_cmd "mt76-test ${interface} del mon${phy_idx}"
	config_handler
	rm -f "${file}"
}

do_ate_work() {
	case "${param}" in
		"ATESTART")
			ate_start
			;;
		"ATESTOP")
			ate_stop
			;;
		"TXCOMMIT")
			mt76_set_cmd "aid=1"
			;;
		"TXFRAME")
			mt76_set_cmd "state=tx_frames"
			;;
		"TXSTOP"|"RXSTOP"|"TXCONTSTOP")
			mt76_set_cmd "state=idle"
			;;
		"TXREVERT")
			mt76_set_cmd "aid=0"
			;;
		"RXFRAME")
			mt76_set_cmd "state=rx_frames"
			;;
		"TXCONT")
			mt76_set_cmd "state=tx_cont"
			;;
		"GROUPREK")
			mt76_set_cmd "state=group_prek"
			;;
		"GROUPREKDump")
			mt76_set_cmd "state=group_prek_dump"
			;;
		"GROUPREKClean")
			mt76_set_cmd "state=group_prek_clean"
			;;
		"DPD2G")
			mt76_set_cmd "state=dpd_2g"
			;;
		"DPD5G")
			mt76_set_cmd "state=dpd_5g"
			;;
		"DPD6G")
			mt76_set_cmd "state=dpd_6g"
			;;
		"DPDDump")
			mt76_set_cmd "state=dpd_dump"
			;;
		"DPDClean")
			mt76_set_cmd "state=dpd_clean"
			;;
		"RXGAINCAL")
			mt76_set_cmd "state=rx_gain_cal"
			do_cmd "atenl -i ${interface} -c \"eeprom rx gain sync\""
			;;
		"RXGAINRESULT")
			mt76_set_cmd "state=rx_gain_dump"
			do_cmd "atenl -i ${interface} -c \"eeprom rx gain sync\""
			;;
		*)
			print_err "Unknown cmd: ${ate_cmd}"
			;;
	esac
}

convert_listmode() {
	local tag _

	split "${param}" "," "tag"

	case "${tag}" in
		"16")
			local tx_vars="tx_length tx_rate_stbc lm_seg_idx tx_antenna \
				       lm_center_ch lm_cbw tx_pkt_bw tx_pri_sel tx_count tx_power \
				       tx_rate_mode tx_rate_idx tx_rate_ldpc tx_ipg tx_rate_sgi \
				       tx_rate_nss lm_seg_timeout"
			local da sa bssid
			local new_param=""

			# declare all the variables in tx_vars
			local ${tx_vars}

			split "${param}" "," \
			      "tag" "_" "_" "_" "tx_length" \
			      "da" "sa" "bssid" "tx_rate_stbc" "_" \
			      "_" "lm_seg_idx" "_" "_" "tx_antenna" \
			      "_" "lm_center_ch" "_" "lm_cbw" "tx_pkt_bw" \
			      "tx_pri_sel" "_" "_" "tx_count" "tx_power" \
			      "tx_rate_mode" "tx_rate_idx" "tx_rate_ldpc" "tx_ipg" "tx_rate_sgi" \
			      "tx_rate_nss" "_" "_" "lm_seg_timeout"

			convert_tm_cbw_to_nl "lm_cbw"
			convert_tm_cbw_to_nl "tx_pkt_bw"
			convert_tx_mode "tx_rate_mode"

			new_param="mac_addrs=${da},${sa},${bssid}"
			concat_vars "new_param" "${tx_vars}"

			mt76_set_cmd "list_act=tx_seg ${new_param}"
			;;
		"17")
			mt76_set_cmd "list_act=tx_start"
			;;
		"19")
			mt76_set_cmd "list_act=tx_stop"
			mt76_set_cmd "list_act=clear_seg"
			;;
		"20")
			local rx_vars="mac_addrs lm_seg_idx tx_antenna lm_center_ch lm_cbw \
				       tx_pkt_bw tx_pri_sel lm_sta_idx lm_seg_timeout"
			local new_param=""

			# declare all the variables in rx_vars
			local ${rx_vars}

			split "${param}" "," \
			      "tag" "mac_addrs" "_" "_" "_" \
			      "_" "_" "tx_antenna" "_" "lm_center_ch" \
			      "_" "lm_cbw" "tx_pkt_bw" "tx_pri_sel" "lm_sta_idx" \
			      "_" "_" "lm_seg_timeout"

			convert_tm_cbw_to_nl "lm_cbw"
			convert_tm_cbw_to_nl "tx_pkt_bw"

			concat_vars "new_param" "${rx_vars}"

			mt76_set_cmd "list_act=rx_seg ${new_param}"
			;;
		"21")
			mt76_set_cmd "list_act=rx_start"
			;;
		"22")
			mt76_set_cmd "list_act=rx_stat"
			;;
		"23")
			mt76_set_cmd "list_act=rx_stop"
			mt76_set_cmd "list_act=clear_seg"
			;;
		"24")
			mt76_set_cmd "list_act=dut_stat"
			;;
		"25")
			mt76_set_cmd "list_act=switch_seg"
			;;
		*)
			print_err "Unknown tag: ${tag}"
			;;
	esac
}

convert_band_to_interface() {
	local base="/sys/kernel/debug/ieee80211/phy0/netdev"
	local dev_dir line links_info
	local ifname link_id

	for dev_dir in "${base}:"*; do
		# get mld interface
		[ ! -d "${dev_dir}" -o ! -d "${dev_dir}/link-0" ] && continue

		ifname="${dev_dir##*:}"
		links_info="${base}:${ifname}/mt76_links_info"

		[ ! -f "${links_info}" ] && continue

		while read -r line; do
			case "${line}" in
				*"${interface}_link_id"*)
					link_id="${line#*=}"
					# remove white spaces & tabs
					link_id="${link_id//[ \t]/}"
					break
					;;
			esac
		done < "${links_info}"

		[ -n "${link_id}" ] && {
			interface="${ifname} -l ${link_id}"
			break
		}
	done

	[ -n "${ifname}" ] && [ -z "${link_id}" ] &&
		print_err "Cannot find any mld interface that has ${interface}"
}

handle_cert() {
	# In wifi 7 chipset, testmode & vendor command both use mwctl
	# Therefore this wrapper would translate it to either mt76-test or
	# mt76-vendor or hostapd_cli based on the attribute of the command

	case "${cmd}" in
		"csi"|"amnt"|"ap_rfeatures"|"ap_wireless"|"mu"|"set_muru_manual_config")
			if [ "${connac_ver}" != "2" ]; then
				do_cmd "hostapd_cli -i ${interface} raw ${full_cmd}"
			else
				do_cmd "mt76-vendor ${interface_ori} set ${full_cmd}"
			fi
			;;
		"txbftxsndinfo"|"TxBfTxSndInfo")
			local bf_txsnd_info

			mt76_debugfs_path "bf_txsnd_info" "band0"
			do_cmd "echo ${param} > ${bf_txsnd_info}"
			;;
		"muruDbgInfo")
			local muru_dbg

			mt76_debugfs_path "muru_dbg"
			do_cmd "echo ${param} > ${muru_dbg}"
			;;
		*)
			return
			;;
	esac

	# If it is cert command, then exit the process after translation
	exit 0
}

handle_cmd_set() {
	local cmd_new="${cmd}"
	local param_new="${param}"
	local _

	handle_cert

	case "${cmd}" in
		"ATE")
			do_ate_work
			return
		;;
		"ATETXCNT"|"ATETXLEN"|"ATETXMCS"|"ATEVHTNSS"|"ATETXNSS"|"ATETXLDPC"| \
		"ATETXSTBC"|"ATEPKTTXTIME"|"ATEIPG"|"ATEDUTYCYCLE"|"ATETXFREQOFFSET")
			simple_convert "cmd_new"
			[ "${cmd_new}" = "tx_count" ] && [ "${param}" = "0" ] &&
				param_new="0xFFFFFFFF"
			;;
		"ATETXANT"|"ATERXANT")
			case "${param}" in
				*:*)
					cmd_new="tx_spe_idx"
					split "${param}" ":" "_" "param_new"
					;;
				*)
					mt76_set_cmd "tx_spe_idx=0"
					cmd_new="tx_antenna"
					;;
			esac
			;;
		"ATETXGI")
			[ "${connac_ver}" = "2" ] && {
				local tx_mode="${ATETXMODE}"

				convert_gi "${tx_mode}" "${param}"
				return
			}
			cmd_new="tx_rate_sgi"
			;;
		"ATETXMODE")
			cmd_new="tx_rate_mode"
			convert_tx_mode "param_new" "${param}"
			record_config "ATETXMODE" "${param_new}"
			;;
		*"TXPOW"*)
			cmd_new="tx_power"
			# for iTest verification
			[ "${param}" = "127" ] && exit 0
			param_new="${param},0,0,0"
			;;
		"ATEMUAID")
			cmd_new="mu_aid"
			param_new="${param}"
			;;
		"ATETXBW")
			convert_bw "${param}"
			return
			;;
		"ATECHANNEL")
			convert_channel "${param}"
			return
			;;
		"ATERXSTAT")
			convert_rxstat
			return
			;;
		"ATECTRLBANDIDX")
			change_band_idx "${param}"
			return
			;;
		"ATEDA"|"ATESA"|"ATEBSSID")
			set_mac_addr
			return
			;;
		"Dfs"*|"ATEDEDICATEDRX")
			convert_dfs
			return
			;;
		*"Bf"*|*"BF"*|*"Sounding"*)
			convert_bf
			return
			;;
		"bufferMode"|"buffermode")
			[ "${param}" = "2" ] &&
				do_cmd "atenl -i ${interface} -c \"eeprom update buffermode\""
			return
			;;
		"ResetCounter"|"ATERXSTATRESET")
			return
			;;
		"WORKMODE")
			record_config "work_mode" "${param}"
			echo "Entering ${param} mode in iwpriv"
			return
			;;
		"ATERUINFO")
			convert_ruinfo
			return
			;;
		"ATELISTMODE")
			convert_listmode
			return
			;;
		*)
			print_err "Unknown command to set: ${cmd}"
			;;
	esac

	mt76_set_cmd "${cmd_new}=${param_new}"
}

handle_cmd_show() {
	case "${cmd}" in
		"wtbl")
			local wlan_idx wtbl_info

			mt76_debugfs_path "wlan_idx"
			mt76_debugfs_path "wtbl_info"
			do_cmd "echo ${param} > ${wlan_idx}"
			do_cmd "cat ${wtbl_info}"
			;;
		"ATERXSTAT")
			convert_rxstat
			;;
		*)
			do_cmd "mt76-test ${interface} dump"
			do_cmd "mt76-test ${interface} dump stats"
			;;
	esac
}

handle_cmd_e2p() {
	local tuple offset val lower_val upper_val v1 v2
        # support multiple read write
	local old_ifs="$IFS"; IFS=","; set -f; set -- ${full_cmd}; set +f; IFS="$old_ifs"

	for tuple in "$@"; do
		split "${tuple}" "=" "cmd" "param"
		offset=$(printf "0x%x" "0x${cmd}")

		# e2p write
		if [ -n "${param}" ]; then
			val=$(printf "0x%x" "0x${param}")
			lower_val=$(printf "0x%x" $((val & 0xff)))
			upper_val=$(printf "0x%x" $(((val >> 8) & 0xff)))

			do_cmd "atenl -i ${interface} -c \"eeprom set ${offset}=${lower_val}\""

			offset=$(printf "0x%x" $((offset + 1)))
			do_cmd "atenl -i ${interface} -c \"eeprom set ${offset}=${upper_val}\""
		else
			query_cmd "v1" "atenl -i ${interface} -c \"eeprom read ${offset}\""
			# output: val = 0xAA (170), extract the decimal value within the parentheses
			v1="${v1#*(}"
			v1="${v1%)*}"

			offset2=$(printf "0x%x" $((offset + 1)))
			query_cmd "v2" "atenl -i ${interface} -c \"eeprom read ${offset2}\""
			v2="${v2#*(}"
			v2="${v2%)*}"

			# The offset here require to be upper case for iTest
			offset=$(printf "0x%04X" "${offset}")
			[ -n "${v1}" -o -n "${v2}" ] &&
				printf "[%s]:0x%02x%02x\n" ${offset} ${v2} ${v1}
		fi
	done
}

register_handler() {
	local regidx regval
	# input: hex string with "0x" prefix
	local offset="$1"
	local val="$2"

	mt76_debugfs_path "regidx"
	mt76_debugfs_path "regval"
	echo "${offset}" > "${regidx}"
	[ -n "${val}" ] && echo "${val}" > "${regval}"

	res=$(cat ${regval})
	printf "[%s]:%s\n" "${offset}" "${res#0x}"
}

handle_cmd_mac() {
	local offset

	if [ -z "${param}" ]; then
		local start_addr end_addr addr

		split "${full_cmd}" "-" "start_addr" "end_addr"
		[ -z "${end_addr}" ] && end_addr="${start_addr}"

		# hex string to decimal value
		start_addr=$((0x${start_addr}))
		end_addr=$((0x${end_addr}))
		addr="${start_addr}"
		while [ "${addr}" -le "${end_addr}" ]; do
			offset=$(printf "0x%x" ${addr})
			register_handler "${offset}"
			addr=$((addr + 4))
		done
	else
		local val=$(printf "0x%x" "0x${param}")
		offset=$(printf "0x%x" "0x${cmd}")
		register_handler "${offset}" "${val}"
	fi
}

handle_cmd_dump() {
        do_cmd "mt76-vendor ${interface} dump ${full_cmd}"
}

handle_cmd_switch() {
	local eeprom_tm_offs="1af"
	local testmode_enable="0"
	local chip_module=""
	local eeprom_mode

	if [ "${connac_ver}" = "3" ]; then
		chip_module="mt7996e"
	elif [ "${connac_ver}" = "5" ]; then
		chip_module="mt7999e"
	else
		return
	fi

	mt76_debugfs_path "eeprom_mode"
	eeprom_mode=$(awk 'NR==2 { $1=$1; print }' "${eeprom_mode}")

	[ "${cmd}" = "testmode" ] && {
		testmode_enable="1"
		do_cmd "wifi down"
	}

	do_cmd "uci set wireless.radio0.disabled=${testmode_enable}"
	do_cmd "uci set wireless.radio1.disabled=${testmode_enable}"
	do_cmd "uci set wireless.radio2.disabled=${testmode_enable}"
	do_cmd "uci commit"

	[ "${eeprom_mode}" = "flash mode" ] && {
		## flash mode should set eeprom testmode offset bit
		## efuse/bin file/default bin mode rely on module param only
		local tm_eeprom_cmd="\"eeprom set 0x${eeprom_tm_offs}=0x${testmode_enable}\""

		do_cmd "atenl -i ${interface} -c ${tm_eeprom_cmd}"
		do_cmd "atenl -i ${interface} -c \"sync eeprom all\""
	}

	do_cmd "rmmod ${chip_module}"
	do_cmd "rmmod mt76-connac-lib"
	do_cmd "rmmod mt76"
	do_cmd "rmmod mac80211"
	do_cmd "rmmod cfg80211"
	do_cmd "rmmod compat"
	do_cmd "insmod compat"
	do_cmd "insmod cfg80211"
	do_cmd "insmod mac80211"
	do_cmd "insmod mt76"
	do_cmd "insmod mt76-connac-lib"
	do_cmd "insmod ${chip_module} testmode_enable=${testmode_enable}"
	do_cmd "sleep 5"
	do_cmd "killall hostapd"
	do_cmd "killall netifd"
}

handle_cmd() {
	case "${cmd_type}" in
		"set")
			handle_cmd_set
			;;
		"show")
			handle_cmd_show
			;;
		"e2p")
			handle_cmd_e2p
			;;
		"mac")
			handle_cmd_mac
			;;
		"dump")
			handle_cmd_dump
			;;
		"switch")
			handle_cmd_switch
			;;
		*)
			print_err "Unknown command type"
			;;
	esac
}

dump_usage() {
	cat << 'EOF'
Usage:
	ATE:
		<iwpriv/mwctl> <interface> set <ATE={param}|{cmd}={param}>
	Show:
		<iwpriv/mwctl> <interface> show <wtbl=<wlan_idx>|ATERXSTAT>
	Read/Write EEPROM:
		iwpriv <interface> e2p <offs>[=<val>]
		iwpriv <interface> e2p <offs>[,<offs2>,...]
		mwctl phy <interface> e2p <offs>[=<val>]
		mwctl phy <interface> e2p <offs>[,<offs2>,...]
	Read/Write CR:
		iwpriv <interface> mac <offs>[=<val>]
		iwpriv <interface> mac <start offs>-<end offs>
		mwctl phy <interface> mac <offs>[=<val>]
		mwctl phy <interface> mac <start offs>-<end offs>
	Switch FW mode:
		<iwpriv/mwctl> <interface> switch <testmode/normal>
	Cert:
		mwctl <interface> set csi ctrl=<opt1>,<opt2>,<opt3>,<opt4> (macaddr=<macaddr>)
		mwctl <interface> set csi interval=<interval (us)>
		mwctl <interface> dump csi <packet num> <filename>
		mwctl <interface> set amnt <index>(0x0~0xf) <mac addr>(xx:xx:xx:xx:xx:xx)
		mwctl <interface> dump amnt <index> (0x0~0xf or 0xff)
		mwctl <interface> set ap_rfeatures he_gi=<val>
		mwctl <interface> set ap_rfeatures he_ltf=<val>
		mwctl <interface> set ap_rfeatures trig_type=<enable>,<val> (val: 0-7)
		mwctl <interface> set ap_rfeatures ack_policy=<val> (val: 0-4)
		mwctl <interface> set ap_wireless fixed_mcs=<val>"
		mwctl <interface> set ap_wireless ofdma=<val> (0: disable, 1: DL, 2: UL)
		mwctl <interface> set ap_wireless nusers_ofdma=<val>
		mwctl <interface> set ap_wireless ppdu_type=<val> (0: SU, 1: MU, 4: LEGACY)
		mwctl <interface> set ap_wireless add_ba_req_bufsize=<val>
		mwctl <interface> set ap_wireless mimo=<val> (0: DL, 1: UL)
		mwctl <interface> set ap_wireless ampdu=<enable>
		mwctl <interface> set ap_wireless amsdu=<enable>
		mwctl <interface> set ap_wireless cert=<enable>
		mwctl <interface> set mu onoff=<val>
		(bitmap: UL MU-MIMO(bit3), DL MU-MIMO(bit2), UL OFDMA(bit1), DL OFDMA(bit0))
		mwctl <interface> dump phy_capa
EOF
}


main() {
	interface="$1"
	cmd_type="$2"
	full_cmd="$3"
	interface_ori="${interface}"
	phy_idx="0"
	main_phy_idx="0"

	parse_sku
	case "${interface}" in
		ra*)
			convert_interface
			;;
		phy)
			# handle mwctl phy phy0 e2p ... case
			interface="$2"
			cmd_type="$3"
			full_cmd="$4"
			;;
		phy[0-9])
			phy_idx="${interface#phy}"
			;;
		band[0-9])
			# mwctl bandX set ... from sigma tool
			# If mld interface exits, the interface should be convert
			# "<mld_iface> -l <link_id>"
			# Otherwise, just remain bandX since sigma will rename the
			# legacy interface name to bandX
			convert_band_to_interface
			;;
		*)
			dump_usage
			print_err "Unsupported interface type [${interface}]"
			;;
	esac

	# get main phy idx
	[ "${phy_idx}" -ge "${pri_chip_bands}" ] && main_phy_idx="${pri_chip_bands}"
	get_config "${SETTING_FILE}"

	case "${full_cmd}" in
		*=*)
			cmd="${full_cmd%%=*}"
			param="${full_cmd#*=}"
			;;
		*)
			cmd="${full_cmd}"
			param=""
			;;
	esac

	# for cert command
	[ "${interface_ori}" != "phy" ] && [ "$#" -gt 3 ] && {
		shift 2
		full_cmd="$*"
	}

	handle_cmd
}

main "$@"
