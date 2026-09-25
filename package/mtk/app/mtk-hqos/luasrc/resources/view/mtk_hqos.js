'use strict';
'require form';
'require fs';
'require network';
'require poll';
'require uci';
'require view';

return view.extend({
	load() {
		return Promise.all([
			uci.load('mtk-hqos'),
			network.getHostHints()
		]);
	},

	render(data) {
		const hostHints = data[1] || {};
		let m, s, o;

		m = new form.Map('mtk-hqos', _('MediaTek Hardware QoS (HQoS)'),
			_('High-performance line-rate QoS rate limiter powered by MediaTek Filogic / MTK QDMA hardware queues, supporting independent IPv4/IPv6 and MAC rate limiting under hardware NAT acceleration.'));

		// 1. Status Section (Always visible with live update)
		s = m.section(form.NamedSection, '_status');
		s.anonymous = true;
		s.render = function() {
			return E('div', { 'class': 'cbi-section' }, [
				E('h3', _('Running Status')),
				E('div', { 'class': 'cbi-section-node' }, [
					E('div', { 'class': 'cbi-value' }, [
						E('label', { 'class': 'cbi-value-title' }, _('Service Status')),
						E('div', { 'class': 'cbi-value-field', 'id': 'hqos_status_val' }, [
							E('span', { 'class': 'badge badge-secondary', 'style': 'background-color:#6c757d;color:#fff;padding:4px 8px;border-radius:4px;font-weight:bold;' }, _('Collecting data...'))
						])
					]),
					E('div', { 'class': 'cbi-value' }, [
						E('label', { 'class': 'cbi-value-title' }, _('Hardware Acceleration Engine')),
						E('div', { 'class': 'cbi-value-field', 'id': 'hqos_driver_val' }, '-')
					]),
					E('div', { 'class': 'cbi-value' }, [
						E('label', { 'class': 'cbi-value-title' }, _('Available QDMA Hardware Queues')),
						E('div', { 'class': 'cbi-value-field', 'id': 'hqos_queues_val' }, '-')
					])
				])
			]);
		};

		// 2. Global Bandwidth Settings
		s = m.section(form.NamedSection, 'global', 'global', _('Global Bandwidth Settings'));

		o = s.option(form.Flag, 'enabled', _('Enable MTK Hardware QoS'));
		o.default = o.disabled;
		o.rmempty = false;

		o = s.option(form.Value, 'download', _('Global Download Bandwidth (Mbit/s) [Optional]'),
			_('Leave empty for unconstrained line-rate (recommended). Set only if you need to limit total WAN throughput or mitigate Bufferbloat.'));
		o.datatype = 'and(uinteger,min(1))';
		o.placeholder = _('Unlimited');
		o.rmempty = true;

		o = s.option(form.Value, 'upload', _('Global Upload Bandwidth (Mbit/s) [Optional]'),
			_('Leave empty for unconstrained line-rate (recommended).'));
		o.datatype = 'and(uinteger,min(1))';
		o.placeholder = _('Unlimited');
		o.rmempty = true;

		o = s.option(form.ListValue, 'scheduling', _('QDMA Scheduling Policy'),
			_('Hardware queue scheduling algorithm. WRR is recommended for most traffic shaping scenarios.'));
		o.value('wrr', _('Weighted Round Robin (WRR)'));
		o.value('sp', _('Strict Priority (SP)'));
		o.default = 'wrr';

		// 3. Device Rules Configuration
		s = m.section(form.TableSection, 'device', _('IP / MAC Bandwidth Limits'),
			_('Allocate independent QDMA hardware queues for specific LAN devices to accurately throttle IPv4 and IPv6 traffic while maintaining hardware NAT acceleration.'));
		s.addremove = true;
		s.anonymous = true;
		s.sortable = true;

		o = s.option(form.Flag, 'enabled', _('Enabled'));
		o.default = o.enabled;
		o.editable = true;

		o = s.option(form.ListValue, 'type', _('Match Type'));
		o.value('ip', _('IP Address (IPv4 / IPv6)'));
		o.value('mac', _('MAC Address'));
		o.default = 'ip';

		o = s.option(form.Value, 'ip', _('IP Address'));
		o.datatype = 'or(ip4addr, ip6addr, cidr4, cidr6)';
		o.placeholder = '192.168.1.100 / 240e:...';
		o.depends('type', 'ip');
		if (hostHints.hosts) {
			for (let mac in hostHints.hosts) {
				const host = hostHints.hosts[mac];
				if (host.ipaddrs) {
					for (let i = 0; i < host.ipaddrs.length; i++) {
						const ip = host.ipaddrs[i];
						const label = host.name ? (host.name + ' (' + ip + ')') : ip;
						o.value(ip, label);
					}
				}
				if (host.ip6addrs) {
					for (let i = 0; i < host.ip6addrs.length; i++) {
						const ip6 = host.ip6addrs[i];
						if (ip6.indexOf('fe80:') === 0) continue;
						const label = host.name ? (host.name + ' [IPv6] (' + ip6 + ')') : (ip6 + ' [IPv6]');
						o.value(ip6, label);
					}
				}
			}
		}

		o = s.option(form.Value, 'mac', _('MAC Address'));
		o.datatype = 'macaddr';
		o.placeholder = 'AA:BB:CC:DD:EE:FF';
		o.depends('type', 'mac');
		if (hostHints.hosts) {
			for (let mac in hostHints.hosts) {
				const host = hostHints.hosts[mac];
				const label = host.name ? (host.name + ' (' + mac + ')') : mac;
				o.value(mac, label);
			}
		}

		o = s.option(form.Value, 'download', _('Download Limit (Mbit/s)'));
		o.datatype = 'and(uinteger,min(1))';
		o.placeholder = '50';
		o.rmempty = true;

		o = s.option(form.Value, 'upload', _('Upload Limit (Mbit/s)'));
		o.datatype = 'and(uinteger,min(1))';
		o.placeholder = '10';
		o.rmempty = true;

		o = s.option(form.Value, 'comment', _('Description'));
		o.placeholder = _('e.g. Office PC / Phone');

		// 4. Active Targets & Bindings (Always present in DOM)
		s = m.section(form.NamedSection, '_active_devices');
		s.anonymous = true;
		s.render = function() {
			return E('div', { 'class': 'cbi-section' }, [
				E('h3', _('Active Rate Limited Targets & IP Bindings')),
				E('div', { 'class': 'cbi-section-descr' }, _('Hardware NAT flow table bindings and allocated queue mappings.')),
				E('div', { 'class': 'table-responsive' }, [
					E('table', { 'id': 'active_devices_table', 'class': 'table cbi-section-table' }, [
						E('tr', { 'class': 'tr table-titles cbi-section-table-titles' }, [
							E('th', { 'class': 'th cbi-section-table-cell' }, _('Target')),
							E('th', { 'class': 'th cbi-section-table-cell' }, _('MAC Address')),
							E('th', { 'class': 'th cbi-section-table-cell' }, _('IPv4 Address')),
							E('th', { 'class': 'th cbi-section-table-cell' }, _('Associated IPv6 Address(es)')),
							E('th', { 'class': 'th cbi-section-table-cell' }, _('Download Queue / Rate')),
							E('th', { 'class': 'th cbi-section-table-cell' }, _('Upload Queue / Rate'))
						]),
						E('tr', { 'class': 'tr placeholder' }, [
							E('td', { 'class': 'td', 'colspan': 6 }, E('em', _('No active rate limited devices.')))
						])
					])
				])
			]);
		};

		// 5. Active Hardware Queue Monitor (Always present in DOM)
		s = m.section(form.NamedSection, '_active_queues');
		s.anonymous = true;
		s.render = function() {
			return E('div', { 'class': 'cbi-section' }, [
				E('h3', _('Active Hardware Queue Monitor (QDMA TX Queues)')),
				E('div', { 'class': 'cbi-section-descr' }, _('Real-time packet counters and rate limiting status on MediaTek network hardware queues.')),
				E('div', { 'class': 'table-responsive' }, [
					E('table', { 'id': 'queue_stats_table', 'class': 'table cbi-section-table' }, [
						E('tr', { 'class': 'tr table-titles cbi-section-table-titles' }, [
							E('th', { 'class': 'th cbi-section-table-cell' }, _('Queue ID')),
							E('th', { 'class': 'th cbi-section-table-cell' }, _('Queue Role / Target')),
							E('th', { 'class': 'th cbi-section-table-cell' }, _('Rate Limit')),
							E('th', { 'class': 'th cbi-section-table-cell' }, _('Transmitted Packets')),
							E('th', { 'class': 'th cbi-section-table-cell' }, _('Transmitted Bytes')),
							E('th', { 'class': 'th cbi-section-table-cell' }, _('Hardware Drops'))
						]),
						E('tr', { 'class': 'tr placeholder' }, [
							E('td', { 'class': 'td', 'colspan': 6 }, E('em', _('Collecting data...')))
						])
					])
				])
			]);
		};

		function formatBytes(bytes) {
			const b = parseInt(bytes) || 0;
			if (b >= 1073741824) return (b / 1073741824).toFixed(2) + ' GB';
			if (b >= 1048576) return (b / 1048576).toFixed(2) + ' MB';
			if (b >= 1024) return (b / 1024).toFixed(1) + ' KB';
			return b + ' B';
		}

		function updateStatusView(res) {
			const statusEl = document.getElementById('hqos_status_val');
			const driverEl = document.getElementById('hqos_driver_val');
			const queuesEl = document.getElementById('hqos_queues_val');

			const isRunning = res && res.running;
			if (statusEl) {
				if (isRunning) {
					statusEl.innerHTML = '<span class="badge badge-success" style="background-color:#28a745;color:#fff;padding:4px 8px;border-radius:4px;font-weight:bold;">' +
						_('Running (Hardware Active)') + '</span>';
				} else {
					statusEl.innerHTML = '<span class="badge badge-danger" style="background-color:#dc3545;color:#fff;padding:4px 8px;border-radius:4px;font-weight:bold;">' +
						_('Stopped (Disabled / Inactive)') + '</span>';
				}
			}

			if (driverEl) {
				if (res && (res.debug_dir || res.toggle_path)) {
					const toggle = res.toggle_path || '';
					const debug = res.debug_dir || '';
					const isHnat = toggle.indexOf('hnat') !== -1 || debug.indexOf('hnat') !== -1;
					const drv = isHnat ? 'mtkhnat' : 'mtk_ppe';
					driverEl.textContent = drv + ' (' + (debug || toggle) + ')';
				} else {
					driverEl.textContent = _('mtkhnat driver not detected');
				}
			}

			if (queuesEl) {
				queuesEl.textContent = (res && res.total_queues ? res.total_queues : 16) + ' ' + _('hardware TX queues');
			}

			// Update Active Devices Table
			const devRows = [];
			const devMap = {};
			if (res && Array.isArray(res.devices) && res.devices.length > 0) {
				for (let i = 0; i < res.devices.length; i++) {
					const d = res.devices[i];
					if (d.dl_qid > 0) devMap[d.dl_qid] = (d.target || d.ip || d.mac) + ' [' + _('Download') + ']';
					if (d.up_qid > 0) devMap[d.up_qid] = (d.target || d.ip || d.mac) + ' [' + _('Upload') + ']';

					let ip6Node;
					if (d.ip6_list && d.ip6_list.trim().length > 0) {
						const ip6Clean = d.ip6_list.trim().split(/\s+/).filter(Boolean);
						ip6Node = E('div', { 'style': 'font-size:11px;' }, ip6Clean.map(ip => E('code', { 'style': 'display:block;white-space:nowrap;margin:1px 0;' }, ip)));
					} else {
						ip6Node = E('span', { 'style': 'color:#888;' }, _('None'));
					}

					devRows.push([
						E('b', {}, d.target || '-'),
						E('code', {}, d.mac || '-'),
						d.ip || '-',
						ip6Node,
						d.dl_qid > 0 ? ('Q' + d.dl_qid + ' (' + d.dl_mbps + ' Mbps)') : _('Unlimited'),
						d.up_qid > 0 ? ('Q' + d.up_qid + ' (' + d.up_mbps + ' Mbps)') : _('Unlimited')
					]);
				}
			}
			cbi_update_table('#active_devices_table', devRows, _('No active rate limited devices.'));

			// Update Queue Stats Table
			const statRows = [];
			if (res && Array.isArray(res.queue_stats) && res.queue_stats.length > 0) {
				for (let i = 0; i < res.queue_stats.length; i++) {
					const q = res.queue_stats[i];
					let roleText = devMap[q.qid] || (q.qid === 0 ? _('Default Queue (Unshaped)') : ('QDMA Q' + q.qid));
					let rateText = (q.max_en > 0 && q.max_rate_kbps > 0) ?
						(q.max_rate_kbps + ' Kbps (' + (q.max_rate_kbps / 1000).toFixed(1) + ' Mbps)') :
						_('Unlimited (Line-rate)');

					let dropNode = (q.drops > 0) ?
						E('span', { 'style': 'color:#dc3545;font-weight:bold;' }, String(q.drops)) :
						String(q.drops);

					statRows.push([
						E('b', {}, 'QDMA TXQ ' + q.qid),
						roleText,
						rateText,
						parseInt(q.packets || 0).toLocaleString(),
						formatBytes(q.bytes || 0),
						dropNode
					]);
				}
			}
			cbi_update_table('#queue_stats_table', statRows, isRunning ? _('No active queue statistics available.') : _('Service is stopped.'));
		}

		return m.render().then(function(mapEl) {
			const fetchStatus = function() {
				return fs.exec_direct('/usr/sbin/mtk-hqos', ['status'], 'json')
					.then(updateStatusView)
					.catch(function(err) {
						updateStatusView(null);
					});
			};

			poll.add(fetchStatus, 3);
			fetchStatus();

			return mapEl;
		});
	}
});
