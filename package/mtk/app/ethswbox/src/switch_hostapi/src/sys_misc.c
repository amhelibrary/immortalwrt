/******************************************************************************

   Copyright 2023-2024 MaxLinear, Inc.

   For licensing information, see the file 'LICENSE' in the root folder of
   this software module.

******************************************************************************/

#include "host_adapt.h"
#include "host_api_impl.h"

int sys_misc_fw_update(const GSW_Device_t *dev)
{
	return gsw_api_wrap(dev,
			    SYS_MISC_FW_UPDATE,
			    NULL,
			    0,
			    0,
			    0);
}

int sys_misc_fw_version(const GSW_Device_t *dev, struct sys_fw_image_version *sys_img_ver)
{
	return gsw_api_wrap(dev,
			    SYS_MISC_FW_VERSION,
			    sys_img_ver,
			    sizeof(*sys_img_ver),
			    0,
			    sizeof(*sys_img_ver));
}

int sys_misc_pvt_temp(const GSW_Device_t *dev, struct sys_sensor_value *sys_temp_val)
{
	return gsw_api_wrap(dev,
			    SYS_MISC_PVT_TEMP,
			    sys_temp_val,
			    sizeof(*sys_temp_val),
			    0,
			    sizeof(*sys_temp_val));
}

int sys_misc_pvt_voltage(const GSW_Device_t *dev, struct sys_sensor_value *sys_voltage)
{
	return gsw_api_wrap(dev,
			    SYS_MISC_PVT_VOLTAGE,
			    sys_voltage,
			    sizeof(*sys_voltage),
			    0,
			    sizeof(*sys_voltage));
}

int sys_misc_delay(const GSW_Device_t *dev, struct sys_delay *pdelay)
{
	return gsw_api_wrap(dev,
			    SYS_MISC_DELAY,
			    pdelay,
			    sizeof(*pdelay),
			    0,
			    0);
}

int sys_misc_gpio_configure(const GSW_Device_t *dev, struct sys_gpio_config *sys_gpio_conf)
{
	return gsw_api_wrap(dev,
			    SYS_MISC_GPIO_CONFIGURE,
			    sys_gpio_conf,
			    sizeof(*sys_gpio_conf),
			    0,
			    0);
}

int sys_misc_reboot(const GSW_Device_t *dev)
{
	return gsw_api_wrap(dev,
			    SYS_MISC_REBOOT,
			    NULL,
			    0,
			    0,
			    0);
}

int sys_misc_reg_rd(const GSW_Device_t *dev, struct sys_reg_rw *sys_reg)
{
	return gsw_api_wrap(dev,
			    SYS_MISC_REG_RD,
			    sys_reg,
			    sizeof(*sys_reg),
			    0,
			    sizeof(*sys_reg));
}

int sys_misc_reg_wr(const GSW_Device_t *dev, struct sys_reg_rw *sys_reg)
{
	return gsw_api_wrap(dev,
			    SYS_MISC_REG_WR,
			    sys_reg,
			    sizeof(*sys_reg),
			    0,
			    0);
}

int sys_misc_reg_mod(const GSW_Device_t *dev, struct sys_reg_mod *sys_reg)
{
	return gsw_api_wrap(dev,
			    SYS_MISC_REG_MOD,
			    sys_reg,
			    sizeof(*sys_reg),
			    0,
			    0);
}

int sys_misc_cml_clk_get(const GSW_Device_t *dev, struct sys_cml_clk *clk)
{
	return gsw_api_wrap(dev,
			    SYS_MISC_CML_CLK_GET,
			    clk,
			    sizeof(*clk),
			    0,
			    sizeof(*clk));
}

int sys_misc_cml_clk_set(const GSW_Device_t *dev, struct sys_cml_clk *clk)
{
	return gsw_api_wrap(dev,
			    SYS_MISC_CML_CLK_SET,
			    clk,
			    sizeof(*clk),
			    0,
			    0);
}

int sys_misc_sfp_get(const GSW_Device_t *dev, struct sys_sfp_cfg *cfg)
{
	return gsw_api_wrap(dev,
			    SYS_MISC_SFP_GET,
			    cfg,
			    sizeof(*cfg),
			    0,
			    sizeof(*cfg));
}

int sys_misc_sfp_set(const GSW_Device_t *dev, struct sys_sfp_cfg *cfg)
{
	return gsw_api_wrap(dev,
			    SYS_MISC_SFP_SET,
			    cfg,
			    sizeof(*cfg),
			    0,
			    sizeof(*cfg));
}

int sys_misc_lldp_get(const GSW_Device_t *dev, struct sys_lldp_get *lldp_sts)
{
	return gsw_api_wrap(dev,
			    SYS_MISC_LLDP_GET,
			    lldp_sts,
			    sizeof(*lldp_sts),
			    0,
			    sizeof(*lldp_sts));
}

int sys_misc_lldp_set(const GSW_Device_t *dev, struct sys_lldp_set *lldp_op)
{
	return gsw_api_wrap(dev,
			    SYS_MISC_LLDP_SET,
			    lldp_op,
			    sizeof(*lldp_op),
			    0,
			    0);
}

int sys_misc_igmp_get(const GSW_Device_t *dev, struct sys_igmp_cfg *cfg)
{
	return gsw_api_wrap(dev,
			    SYS_MISC_IGMP_GET,
			    cfg,
			    sizeof(*cfg),
			    0,
			    sizeof(*cfg));
}

int sys_misc_igmp_set(const GSW_Device_t *dev, struct sys_igmp_cfg *cfg)
{
	return gsw_api_wrap(dev,
			    SYS_MISC_IGMP_SET,
			    cfg,
			    sizeof(*cfg),
			    0,
			    0);
}

int loop_detect_passive_start(const GSW_Device_t *dev,
			      struct loop_detect_passive_cfg *cfg)
{
	return gsw_api_wrap(dev,
			    SYS_MISC_LOOP_PASSIVE_START,
			    cfg,
			    sizeof(*cfg),
			    0,
			    0);
}

int loop_detect_passive_stop(const GSW_Device_t *dev)
{
	return gsw_api_wrap(dev,
			    SYS_MISC_LOOP_PASSIVE_STOP,
			    NULL,
			    0,
			    0,
			    0);
}

int loop_detect_active_start(const GSW_Device_t *dev,
			     struct loop_detect_active_cfg *cfg)
{
	return gsw_api_wrap(dev,
			    SYS_MISC_LOOP_ACTIVE_START,
			    cfg,
			    sizeof(*cfg),
			    0,
			    0);
}

int loop_detect_active_stop(const GSW_Device_t *dev)
{
	return gsw_api_wrap(dev,
			    SYS_MISC_LOOP_ACTIVE_STOP,
			    NULL,
			    0,
			    0,
			    0);
}

int loop_detect_active_cfg_get(const GSW_Device_t *dev,
			       struct loop_detect_active_cfg *cfg)
{
	return gsw_api_wrap(dev,
			    SYS_MISC_LOOP_ACTIVE_CFG_GET,
			    cfg,
			    sizeof(*cfg),
			    0,
			    sizeof(*cfg));
}

int loop_prevention_start(const GSW_Device_t *dev,
			  struct loop_prevention_cfg *cfg)
{
	return gsw_api_wrap(dev,
			    SYS_MISC_LOOP_PREVENTION_START,
			    cfg,
			    sizeof(*cfg),
			    0,
			    0);
}

int loop_prevention_stop(const GSW_Device_t *dev)
{
	return gsw_api_wrap(dev,
			    SYS_MISC_LOOP_PREVENTION_STOP,
			    NULL,
			    0,
			    0,
			    0);
}

int mxl_led_sys_cfg_get(const GSW_Device_t *dev, struct mxl_led_sys_cfg *cfg)
{
	return gsw_api_wrap(dev,
			    SYS_MISC_SYS_LED_CFG_GET,
			    cfg,
			    sizeof(*cfg),
			    0,
			    sizeof(*cfg));
}

int mxl_led_sys_cfg_set(const GSW_Device_t *dev, struct mxl_led_sys_cfg *cfg)
{
	return gsw_api_wrap(dev,
			    SYS_MISC_SYS_LED_CFG_SET,
			    cfg,
			    sizeof(*cfg),
			    0,
			    0);
}

int sys_misc_igmp_report_flood_get(const GSW_Device_t *dev,
				   struct sys_igmp_cfg *cfg)
{
	return gsw_api_wrap(dev,
			    SYS_MISC_IGMP_REPORT_FLOOD_GET,
			    cfg,
			    sizeof(*cfg),
			    0,
			    sizeof(*cfg));
}

int sys_misc_igmp_report_flood_set(const GSW_Device_t *dev,
				   struct sys_igmp_cfg *cfg)
{
	return gsw_api_wrap(dev,
			    SYS_MISC_IGMP_REPORT_FLOOD_SET,
			    cfg,
			    sizeof(*cfg),
			    SYS_MISC_IGMP_REPORT_FLOOD_GET,
			    0);
}

int sys_misc_daemon_get(const GSW_Device_t *dev,
			struct sys_daemon_cfg *pcfg)
{
	return gsw_api_wrap(dev,
			    SYS_MISC_DAEMON_GET,
			    pcfg,
			    sizeof(*pcfg),
			    0,
			    sizeof(*pcfg));
}

int sys_misc_daemon_set(const GSW_Device_t *dev,
			struct sys_daemon_cfg *pcfg)
{
	return gsw_api_wrap(dev,
			    SYS_MISC_DAEMON_SET,
			    pcfg,
			    sizeof(*pcfg),
			    SYS_MISC_DAEMON_GET,
			    0);
}

int sys_misc_host_ip_get(const GSW_Device_t *dev, struct sys_host_ip_cfg *cfg)
{
	return gsw_api_wrap(dev,
			    SYS_MISC_HOST_IP_GET,
			    cfg,
			    sizeof(*cfg),
			    0,
			    sizeof(*cfg));
}

int sys_misc_host_ip_set(const GSW_Device_t *dev, struct sys_host_ip_cfg *cfg)
{
	return gsw_api_wrap(dev,
			    SYS_MISC_HOST_IP_SET,
			    cfg,
			    sizeof(*cfg),
			    SYS_MISC_HOST_IP_GET,
			    0);
}

int sys_misc_prg_mac(const GSW_Device_t *dev, struct sys_prg_mac *cfg)
{
	return gsw_api_wrap(dev,
			    SYS_MISC_PRG_MAC,
			    cfg,
			    sizeof(*cfg),
			    0,
			    0);
}

int sys_misc_ptp_ts_corr_cal_get(const GSW_Device_t *dev, struct sys_ptp_ts_corr_cal *cfg)
{
	return gsw_api_wrap(dev,
			    SYS_MISC_PTP_TS_CORR_CAL_GET,
			    cfg,
			    sizeof(*cfg),
			    0,
			    0);
}

int sys_misc_ptp_ts_corr_cal_set(const GSW_Device_t *dev, struct sys_ptp_ts_corr_cal *cfg)
{
	return gsw_api_wrap(dev,
			    SYS_MISC_PTP_TS_CORR_CAL_SET,
			    cfg,
			    sizeof(*cfg),
			    SYS_MISC_PTP_TS_CORR_CAL_GET,
			    0);
}

int sys_misc_data_led_set(const GSW_Device_t *dev, struct sys_data_led_cfg *cfg)
{
	return gsw_api_wrap(dev,
			    SYS_MISC_DATA_LED_SET,
			    cfg,
			    sizeof(*cfg),
			    0,
			    0);
}

int sys_misc_port_map_get(const GSW_Device_t *dev, struct sys_port_map *cfg)
{
	return gsw_api_wrap(dev,
			    SYS_MISC_PORT_MAP_GET,
			    cfg,
			    sizeof(*cfg),
			    0,
			    sizeof(*cfg));
}

int sys_misc_port_map_set(const GSW_Device_t *dev, struct sys_port_map *cfg)
{
	return gsw_api_wrap(dev,
			    SYS_MISC_PORT_MAP_SET,
			    cfg,
			    sizeof(*cfg),
			    0,
			    0);
}

int loop_detect_active_mac_get(const GSW_Device_t *dev, uint8_t cfg[6])
{
	return gsw_api_wrap(dev,
			    SYS_MISC_LOOP_ACTIVE_MAC_GET,
			    cfg,
			    6,
			    0,
			    6);
}

int loop_detect_active_mac_set(const GSW_Device_t *dev, uint8_t cfg[6])
{
	return gsw_api_wrap(dev,
			    SYS_MISC_LOOP_ACTIVE_MAC_SET,
			    cfg,
			    6,
			    SYS_MISC_LOOP_ACTIVE_MAC_GET,
			    0);
}

int sys_misc_xg_fc_thr_ratio_get(const GSW_Device_t *dev,
				 struct sys_xg_fc_thr_ratio *pcfg)
{
	return gsw_api_wrap(dev,
			    SYS_MISC_XG_FC_THR_RATIO_GET,
			    pcfg,
			    sizeof(*pcfg),
			    0,
			    sizeof(*pcfg));
}

int sys_misc_xg_fc_thr_ratio_set(const GSW_Device_t *dev,
				 struct sys_xg_fc_thr_ratio *pcfg)
{
	return gsw_api_wrap(dev,
			    SYS_MISC_XG_FC_THR_RATIO_SET,
			    pcfg,
			    sizeof(*pcfg),
			    SYS_MISC_XG_FC_THR_RATIO_GET,
			    0);
}

int sys_misc_pce_svc_get(const GSW_Device_t *dev, struct sys_pce_svc_cfg *pcfg)
{
	return gsw_api_wrap(dev,
			    SYS_MISC_PCE_SVC_GET,
			    pcfg,
			    sizeof(*pcfg),
			    0,
			    sizeof(*pcfg));
}

int sys_misc_pce_svc_set(const GSW_Device_t *dev, struct sys_pce_svc_cfg *pcfg)
{
	return gsw_api_wrap(dev,
			    SYS_MISC_PCE_SVC_SET,
			    pcfg,
			    sizeof(*pcfg),
			    SYS_MISC_PCE_SVC_GET,
			    0);
}

int loop_detect_cascade_cfg_get(const GSW_Device_t *dev,
				struct loop_detect_cascade_cfg *pcfg)
{
	return gsw_api_wrap(dev,
			    SYS_MISC_LOOP_CASCADE_CFG_GET,
			    pcfg,
			    sizeof(*pcfg),
			    0,
			    sizeof(*pcfg));
}

int loop_detect_cascade_cfg_set(const GSW_Device_t *dev,
				struct loop_detect_cascade_cfg *pcfg)
{
	return gsw_api_wrap(dev,
			    SYS_MISC_LOOP_CASCADE_CFG_SET,
			    pcfg,
			    sizeof(*pcfg),
			    SYS_MISC_LOOP_CASCADE_CFG_GET,
			    0);
}
