'use strict';
'require view';
'require fs';
'require poll';
'require ui';
'require dom';

/*
 * MediaTek Filogic 880 (MT7988) PPE Hardware MIB Traffic Statistics View
 * Standard LuCI Client-Side Rendering (Adaptive Theme Compatible)
 * Features:
 *   - Stacked Row 1: IPv4 Address <-> IPv4 Realtime (Rx + Tx) <-> IPv4 Total (Rx + Tx)
 *   - Stacked Row 2: IPv6 Address <-> IPv6 Realtime (Rx + Tx) <-> IPv6 Total (Rx + Tx)
 * Copyright (C) 2026 MediaTek Filogic Team / ImmortalWrt
 */

return view.extend({
	load: function() {
		return fs.exec('/usr/bin/hnat_mib_stat', ['--json']);
	},

	formatBytes: function(bytes) {
		bytes = parseFloat(bytes) || 0;
		if (bytes <= 0) return '0 B';
		var units = ['B', 'KB', 'MB', 'GB', 'TB'];
		var i = Math.floor(Math.log(bytes) / Math.log(1024));
		if (i >= units.length) i = units.length - 1;
		return (bytes / Math.pow(1024, i)).toFixed(2) + ' ' + units[i];
	},

	formatRate: function(bps) {
		bps = parseFloat(bps) || 0;
		if (bps <= 0) return '0 bps';
		if (bps < 1000) return bps.toFixed(0) + ' bps';
		else if (bps < 1000000) return (bps / 1000).toFixed(2) + ' Kbps';
		else if (bps < 1000000000) return (bps / 1000000).toFixed(2) + ' Mbps';
		else return (bps / 1000000000).toFixed(2) + ' Gbps';
	},

	render: function(response) {
		var self = this;

		var viewContainer = E('div', { 'class': 'cbi-map' }, [
			E('h2', {}, _('MediaTek HNAT Hardware Traffic Statistics')),
			E('div', { 'class': 'cbi-map-descr' }, _('Based on MediaTek Filogic 880 (MT7988) PPE ASIC hardware MIB counters, providing lossless sub-second flow polling and high-precision LAN client traffic monitoring.'))
		]);

		// 1. Overview Metric Cards Container (Theme-Adaptive CSS)
		var cardGrid = E('div', {
			'style': 'display: grid; grid-template-columns: repeat(auto-fit, minmax(210px, 1fr)); gap: 15px; margin-bottom: 20px;'
		}, [
			E('div', { 'class': 'cbi-section-node', 'style': 'padding: 14px 18px; border-radius: 6px; border: 1px solid var(--border-color, rgba(128,128,128,0.2));' }, [
				E('div', { 'style': 'font-size: 13px; opacity: 0.75; margin-bottom: 4px;' }, _('Total Realtime Download Rate')),
				E('div', { 'id': 'stat-total-rx-rate', 'style': 'font-size: 22px; font-weight: 700; color: var(--primary-color, #1890ff);' }, '-')
			]),
			E('div', { 'class': 'cbi-section-node', 'style': 'padding: 14px 18px; border-radius: 6px; border: 1px solid var(--border-color, rgba(128,128,128,0.2));' }, [
				E('div', { 'style': 'font-size: 13px; opacity: 0.75; margin-bottom: 4px;' }, _('Total Realtime Upload Rate')),
				E('div', { 'id': 'stat-total-tx-rate', 'style': 'font-size: 22px; font-weight: 700; color: #52c41a;' }, '-')
			]),
			E('div', { 'class': 'cbi-section-node', 'style': 'padding: 14px 18px; border-radius: 6px; border: 1px solid var(--border-color, rgba(128,128,128,0.2));' }, [
				E('div', { 'style': 'font-size: 13px; opacity: 0.75; margin-bottom: 4px;' }, _('Total Hardware Volume')),
				E('div', { 'id': 'stat-total-volume', 'style': 'font-size: 20px; font-weight: 600;' }, '-')
			]),
			E('div', { 'class': 'cbi-section-node', 'style': 'padding: 14px 18px; border-radius: 6px; border: 1px solid var(--border-color, rgba(128,128,128,0.2));' }, [
				E('div', { 'style': 'font-size: 13px; opacity: 0.75; margin-bottom: 4px;' }, _('Active Hardware Flows (BIND)')),
				E('div', { 'id': 'stat-active-flows', 'style': 'font-size: 20px; font-weight: 600;' }, '-')
			])
		]);

		viewContainer.appendChild(E('div', { 'class': 'cbi-section' }, [
			E('h3', {}, _('Hardware Acceleration Overview')),
			cardGrid
		]));

		// 2. Hardware Engine Specifications Table
		var hwTable = E('table', { 'class': 'table' }, [
			E('tr', { 'class': 'tr' }, [
				E('td', { 'class': 'td left', 'style': 'width: 25%; font-weight: 600;' }, _('Hardware Engine')),
				E('td', { 'class': 'td left' }, 'MediaTek PPE (Packet Process Engine v3.0 / HNAT)'),
				E('td', { 'class': 'td left', 'style': 'width: 25%; font-weight: 600;' }, _('Driver Architecture')),
				E('td', { 'class': 'td left' }, 'mtkhnat (Hardware 64-bit Atomic MIB Accounting)')
			]),
			E('tr', { 'class': 'tr' }, [
				E('td', { 'class': 'td left', 'style': 'font-weight: 600;' }, _('Hardware Flow Capacity')),
				E('td', { 'class': 'td left' }, '32,768 Entries (FOE Table)'),
				E('td', { 'class': 'td left', 'style': 'font-weight: 600;' }, _('Update Interval')),
				E('td', { 'class': 'td left', 'id': 'stat-interval' }, '2s')
			])
		]);

		viewContainer.appendChild(E('div', { 'class': 'cbi-section' }, [
			E('h3', {}, _('Hardware Specifications')),
			hwTable
		]));

		// 3. Per-Host Traffic Table (Full Center Alignment)
		var hostTbody = E('tbody', { 'id': 'table-hosts-body' });
		var hostTable = E('table', { 'class': 'table', 'id': 'table-lan-hosts' }, [
			E('thead', {}, [
				E('tr', { 'class': 'tr table-titles' }, [
					E('th', { 'class': 'th center', 'style': 'flex: 0 0 13%; width: 13%; text-align: center;' }, _('Hostname')),
					E('th', { 'class': 'th center', 'style': 'flex: 0 0 21%; width: 21%; text-align: center;' }, _('IP Address (IPv4 / IPv6)')),
					E('th', { 'class': 'th center', 'style': 'flex: 0 0 12%; width: 12%; text-align: center;' }, _('MAC Address')),
					E('th', { 'class': 'th center', 'style': 'flex: 0 0 17%; width: 17%; text-align: center;' }, _('Realtime Rate (Download / Upload)')),
					E('th', { 'class': 'th center', 'style': 'flex: 0 0 14%; width: 14%; text-align: center;' }, _('Total Traffic (Download / Upload)')),
					E('th', { 'class': 'th center', 'style': 'flex: 0 0 11%; width: 11%; text-align: center;' }, _('Total Traffic (IPv4 + IPv6)')),
					E('th', { 'class': 'th center', 'style': 'flex: 0 0 6%; width: 6%; text-align: center;' }, _('Active Connections')),
					E('th', { 'class': 'th center', 'style': 'flex: 0 0 6%; width: 6%; text-align: center;' }, _('Discovery Method'))
				])
			]),
			hostTbody
		]);

		viewContainer.appendChild(E('div', { 'class': 'cbi-section' }, [
			E('h3', {}, _('LAN Clients Traffic Statistics')),
			hostTable
		]));

		// 4. Top Active Hardware Flows Table
		var flowTbody = E('tbody', { 'id': 'table-flows-body' });
		var flowTable = E('table', { 'class': 'table', 'id': 'table-active-flows' }, [
			E('thead', {}, [
				E('tr', { 'class': 'tr table-titles' }, [
					E('th', { 'class': 'th center', 'style': 'width: 10%;' }, _('Flow Entry Index')),
					E('th', { 'class': 'th', 'style': 'width: 12%;' }, _('Protocol')),
					E('th', { 'class': 'th', 'style': 'width: 23%;' }, _('Source Address')),
					E('th', { 'class': 'th', 'style': 'width: 23%;' }, _('Destination Address')),
					E('th', { 'class': 'th', 'style': 'width: 10%;' }, _('Bytes')),
					E('th', { 'class': 'th', 'style': 'width: 8%;' }, _('Packets')),
					E('th', { 'class': 'th', 'style': 'width: 8%;' }, _('Direction')),
					E('th', { 'class': 'th', 'style': 'width: 6%;' }, _('State'))
				])
			]),
			flowTbody
		]);

		var flowLimitSelect = E('select', {
			'id': 'select-flow-limit',
			'class': 'cbi-input-select',
			'style': 'margin-left: 8px; width: auto; font-size: 12px; padding: 2px 8px;',
			'change': function(ev) {
				self.currentFlowLimit = parseInt(ev.target.value) || 10;
				if (self.lastData) self.updateFlowsTable(self.lastData);
			}
		}, [
			E('option', { 'value': '10', 'selected': '' }, 'Top 10'),
			E('option', { 'value': '25' }, 'Top 25'),
			E('option', { 'value': '50' }, 'Top 50')
		]);

		var flowHeader = E('div', {
			'style': 'display: flex; justify-content: space-between; align-items: center; margin-bottom: 8px; flex-wrap: nowrap;'
		}, [
			E('h3', { 'style': 'margin: 0; white-space: nowrap;' }, _('Top Active Hardware Flows')),
			E('div', { 'style': 'display: flex; align-items: center; font-size: 12px; white-space: nowrap; flex-shrink: 0;' }, [
				E('span', { 'style': 'opacity: 0.85; white-space: nowrap;' }, _('Display Limit:')),
				flowLimitSelect
			])
		]);

		viewContainer.appendChild(E('div', { 'class': 'cbi-section' }, [
			flowHeader,
			flowTable
		]));

		// Initial Data Update
		self.updateView(response);

		// Register 2s Polling Routine
		poll.add(function() {
			return fs.exec('/usr/bin/hnat_mib_stat', ['--json']).then(function(res) {
				self.updateView(res);
			});
		}, 2);

		return viewContainer;
	},

	updateView: function(res) {
		var self = this;
		if (!res || res.code !== 0 || !res.stdout) return;

		var data = null;
		try {
			data = JSON.parse(res.stdout);
		} catch (e) {
			return;
		}

		if (!data || !data.hosts) return;

		var totalRxRate = 0;
		var totalTxRate = 0;
		var totalRxBytes = 0;
		var totalTxBytes = 0;

		var hostList = [];
		for (var ip in data.hosts) {
			var h = data.hosts[ip];
			totalRxRate += (h.rx_rate_bps || 0);
			totalTxRate += (h.tx_rate_bps || 0);
			totalRxBytes += (h.rx_bytes || 0);
			totalTxBytes += (h.tx_bytes || 0);
			hostList.push(h);
		}

		// Sort hosts by total traffic descending
		hostList.sort(function(a, b) {
			return ((b.rx_bytes || 0) + (b.tx_bytes || 0)) - ((a.rx_bytes || 0) + (a.tx_bytes || 0));
		});

		// 1. Update Overview Metric Cards
		var elRxRate = document.getElementById('stat-total-rx-rate');
		var elTxRate = document.getElementById('stat-total-tx-rate');
		var elVolume = document.getElementById('stat-total-volume');
		var elFlows  = document.getElementById('stat-active-flows');
		var elIntv   = document.getElementById('stat-interval');

		if (elRxRate) elRxRate.textContent = self.formatRate(totalRxRate);
		if (elTxRate) elTxRate.textContent = self.formatRate(totalTxRate);
		if (elVolume) elVolume.textContent = self.formatBytes(data.total_hardware_bytes || (totalRxBytes + totalTxBytes));
		if (elFlows)  elFlows.textContent  = (data.active_flows || 0) + ' ' + _('flows');
		if (elIntv)   elIntv.textContent   = (data.interval || 2) + 's';

		// 2. Update LAN Hosts Table (Grid-Aligned Multi-Row Stacking Layout)
		var hostTbody = document.getElementById('table-hosts-body');
		if (hostTbody) {
			dom.content(hostTbody, null);

			if (hostList.length === 0) {
				hostTbody.appendChild(E('tr', { 'class': 'tr' }, [
					E('td', { 'class': 'td center', 'colspan': 7, 'style': 'padding: 16px; opacity: 0.6;' }, _('No active LAN devices found'))
				]));
			} else {
				for (var i = 0; i < hostList.length; i++) {
					var host = hostList[i];
					var rowClass = 'tr cbi-rowstyle-' + ((i % 2) + 1);

					// Column 2: IP Address (Row 1: IPv4, Row 2: IPv6) —— 居中
					var ipCellContent = [
						E('div', { 'style': 'font-weight: 600; text-align: center;' }, host.ip),
						E('div', {
							'style': 'font-size: 11px; opacity: 0.75; font-family: monospace; word-break: break-all; text-align: center; margin-top: 3px;'
						}, (host.ipv6 && host.ipv6 !== '') ? host.ipv6 : '-')
					];

					// 上下行成对居中：flex 行 + 固定最小宽度，保证两列对称不抖动
					var pairStyle = 'display: flex; justify-content: center; align-items: baseline; gap: 10px; white-space: nowrap; line-height: 1.5;';
					var pairSubStyle = pairStyle + ' font-size: 11px; font-family: monospace; opacity: 0.75; margin-top: 3px;';
					var valStyle = 'min-width: 78px; text-align: center;';

					// Column 4: Realtime Rate (Row 1: IPv4, Row 2: IPv6)
					var rateCellContent = [
						E('div', { 'style': pairStyle }, [
							E('span', { 'style': valStyle + ' font-weight: 600; color: var(--primary-color, #1890ff);' }, [
								E('span', { 'style': 'font-size: 11px; opacity: 0.8; margin-right: 3px;' }, '↓'),
								self.formatRate(host.v4_rx_rate_bps || host.rx_rate_bps)
							]),
							E('span', { 'style': valStyle + ' font-weight: 600; color: #52c41a;' }, [
								E('span', { 'style': 'font-size: 11px; opacity: 0.8; margin-right: 3px;' }, '↑'),
								self.formatRate(host.v4_tx_rate_bps || host.tx_rate_bps)
							])
						]),
						E('div', { 'style': pairSubStyle }, [
							E('span', { 'style': valStyle + ' color: var(--primary-color, #1890ff); opacity: 0.8;' }, [
								E('span', { 'style': 'margin-right: 3px;' }, '↓'),
								self.formatRate(host.v6_rx_rate_bps || 0)
							]),
							E('span', { 'style': valStyle + ' color: #52c41a; opacity: 0.8;' }, [
								E('span', { 'style': 'margin-right: 3px;' }, '↑'),
								self.formatRate(host.v6_tx_rate_bps || 0)
							])
						])
					];

					// Column 5: Total Traffic (Row 1: IPv4, Row 2: IPv6)
					var volumeCellContent = [
						E('div', { 'style': pairStyle }, [
							E('span', { 'style': valStyle }, [
								E('span', { 'style': 'font-size: 11px; opacity: 0.75; margin-right: 3px;' }, '↓'),
								self.formatBytes(host.v4_rx_bytes || host.rx_bytes)
							]),
							E('span', { 'style': valStyle }, [
								E('span', { 'style': 'font-size: 11px; opacity: 0.75; margin-right: 3px;' }, '↑'),
								self.formatBytes(host.v4_tx_bytes || host.tx_bytes)
							])
						]),
						E('div', { 'style': pairSubStyle }, [
							E('span', { 'style': valStyle }, [
								E('span', { 'style': 'margin-right: 3px;' }, '↓'),
								self.formatBytes(host.v6_rx_bytes || 0)
							]),
							E('span', { 'style': valStyle }, [
								E('span', { 'style': 'margin-right: 3px;' }, '↑'),
								self.formatBytes(host.v6_tx_bytes || 0)
							])
						])
					];

					// Column 6: Total Traffic of IPv4 + IPv6 (summed, single row)
					var sumRx = (host.v4_rx_bytes || host.rx_bytes || 0) + (host.v6_rx_bytes || 0);
					var sumTx = (host.v4_tx_bytes || host.tx_bytes || 0) + (host.v6_tx_bytes || 0);
					var valStyleSum = 'min-width: 64px; text-align: center;';

					var totalVolumeCellContent = E('div', { 'style': pairStyle + ' font-weight: 600;' }, [
						E('span', { 'style': valStyleSum + ' color: var(--primary-color, #1890ff);' }, [
							E('span', { 'style': 'font-size: 11px; opacity: 0.8; margin-right: 3px;' }, '↓'),
							self.formatBytes(sumRx)
						]),
						E('span', { 'style': valStyleSum + ' color: #52c41a;' }, [
							E('span', { 'style': 'font-size: 11px; opacity: 0.8; margin-right: 3px;' }, '↑'),
							self.formatBytes(sumTx)
						])
					]);

					hostTbody.appendChild(E('tr', { 'class': rowClass }, [
						E('td', { 'class': 'td center', 'style': 'flex: 0 0 13%; width: 13%; text-align: center; font-weight: 600;' }, host.hostname || 'unknown'),
						E('td', { 'class': 'td center', 'style': 'flex: 0 0 21%; width: 21%; text-align: center;' }, ipCellContent),
						E('td', { 'class': 'td center', 'style': 'flex: 0 0 12%; width: 12%; text-align: center; font-family: monospace; opacity: 0.85;' }, host.mac || '-'),
						E('td', { 'class': 'td center', 'style': 'flex: 0 0 17%; width: 17%; text-align: center;' }, rateCellContent),
						E('td', { 'class': 'td center', 'style': 'flex: 0 0 14%; width: 14%; text-align: center;' }, volumeCellContent),
						E('td', { 'class': 'td center', 'style': 'flex: 0 0 11%; width: 11%; text-align: center;' }, totalVolumeCellContent),
						E('td', { 'class': 'td center', 'style': 'flex: 0 0 6%; width: 6%; text-align: center;' }, [
							E('span', { 'class': 'badge', 'style': 'padding: 2px 8px; border-radius: 10px;' }, host.active_conns || 0)
						]),
						E('td', { 'class': 'td center', 'style': 'flex: 0 0 6%; width: 6%; text-align: center;' }, [
							E('span', { 'class': 'label', 'style': 'font-size: 11px;' }, host.source || 'DHCP')
						])
					]));
				}
			}
		}

		self.lastData = data;
		self.updateFlowsTable(data);
	},

	updateFlowsTable: function(data) {
		var self = this;
		var flowTbody = document.getElementById('table-flows-body');
		if (!flowTbody || !data) return;

		dom.content(flowTbody, null);

		var topFlows = data.top_flows || [];
		var limit = self.currentFlowLimit || 10;
		var displayCount = Math.min(topFlows.length, limit);

		if (displayCount === 0) {
			flowTbody.appendChild(E('tr', { 'class': 'tr' }, [
				E('td', { 'class': 'td center', 'colspan': 8, 'style': 'padding: 16px; opacity: 0.6;' }, _('No active hardware flow data'))
			]));
		} else {
			for (var j = 0; j < displayCount; j++) {
				var fl = topFlows[j];
				var flowRowClass = 'tr cbi-rowstyle-' + ((j % 2) + 1);
				var isRx = (fl.dir === 'RX');

				flowTbody.appendChild(E('tr', { 'class': flowRowClass }, [
					E('td', { 'class': 'td center', 'style': 'font-family: monospace;' }, '#' + fl.index),
					E('td', { 'class': 'td' }, [
						E('span', { 'class': 'badge' }, fl.proto)
					]),
					E('td', { 'class': 'td', 'style': 'font-family: monospace; font-size: 12px;' }, fl.src),
					E('td', { 'class': 'td', 'style': 'font-family: monospace; font-size: 12px;' }, fl.dst),
					E('td', { 'class': 'td', 'style': 'font-weight: 600;' }, self.formatBytes(fl.bytes)),
					E('td', { 'class': 'td' }, (fl.packets || 0) + ' ' + _('pkts')),
					E('td', { 'class': 'td' }, [
						E('span', {
							'style': 'padding: 2px 6px; border-radius: 4px; font-size: 11px; font-weight: 600; ' +
							         (isRx ? 'background: rgba(24,144,255,0.15); color: var(--primary-color, #1890ff);' : 'background: rgba(82,196,26,0.15); color: #52c41a;')
						}, isRx ? _('Download (Rx)') : _('Upload (Tx)'))
					]),
					E('td', { 'class': 'td' }, [
						E('span', {
							'style': 'color: #52c41a; font-weight: 600;'
						}, '● ' + fl.state)
					])
				]));
			}
		}
	},

	handleSaveApply: null,
	handleSave: null,
	handleReset: null
});
