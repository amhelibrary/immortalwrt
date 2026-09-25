/******************************************************************************

   Copyright 2026 MaxLinear, Inc.

   For licensing information, see the file 'LICENSE' in the root folder of
   this software module.

******************************************************************************/

#ifndef _GSW_DEFCONF_H__
#define _GSW_DEFCONF_H__

/* Meter allocation layout (total 128 meters: 0-127):
 * 0:    Pre-allocated for CPU at init
 * 1-16: Pre-allocated for per CTP ingress at init
 * 17:   Pre-allocated for global storm control at init
 * 18+:  Dynamically allocated based on get_chip_port_num()
 *
 * Dynamic allocations (via bulk allocation on first use):
 * - Control packets:  Allocated per-port in gsw_defconf.c (base: 18)
 * - Storm Control:    num_ports * 4 meters
 * - DHCP Snooping:    num_ports * 1 meters
 *
 * Actual allocation ranges depend on port count (8/10/16 port variants):
 *   8-port:  Storm Control needs 32 meters, DHCP needs 8 meters
 *   10-port: Storm Control needs 40 meters, DHCP needs 10 meters
 *   16-port: Storm Control needs 64 meters, DHCP needs 16 meters
 */
#define CPU_PORT_METER			0
#define CTP_INGRESS_METER_BASE		1
#define GLOBAL_STORM_CTRL_METER		17
#define CTRL_PKT_METER_BASE		18

#endif /* _GSW_DEFCONF_H__ */
