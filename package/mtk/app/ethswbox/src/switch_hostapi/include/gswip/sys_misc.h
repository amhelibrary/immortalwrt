/******************************************************************************

   Copyright 2025 MaxLinear, Inc.

   For licensing information, see the file 'LICENSE' in the root folder of
   this software module.

******************************************************************************/

#ifndef _SYS_MISC_H_
#define _SYS_MISC_H_

/** \defgroup DOC_SYS_MISC System Miscellaneous APIs
 *  \brief This chapter describes the program interface to configure to level
 *  services of Ethernet Switch module (other than switch or PHY
 *  configurations).
 */

/** \addtogroup DOC_SYS_MISC
 * @{
 */

#pragma pack(push, 1)
#pragma scalar_storage_order little-endian

/** \brief Firmware Version.
 *  Used by \ref sys_misc_fw_version.
 */
struct sys_fw_image_version {
	/** Major number */
	uint8_t major;
	/** Minor number */
	uint8_t minor;
	/** Revision number of kernel */
	uint16_t revision;
	/** Revision number of application */
	uint32_t app_revision;
};

/** \brief Delay Time.
 *  Used by \ref sys_misc_delay.
 */
struct sys_delay {
	/** unit is 1ms, the accuracy is 10ms. */
	uint32_t m_sec;
};

/** \brief GPIO Configuration.
 *  GPIO is indexed 0~47. Configure multiple GPIOs' function (pinmux),
 *  direction and output level. Used by \ref sys_misc_gpio_configure.
 */
struct sys_gpio_config {
	/** bit 1 to configure this GPIO pin. */
	uint16_t enable_mask[3];
	/** alt_sel_0 and alt_sel_1 to configure GPIO function (pinmux) */
	uint16_t alt_sel_0[3];
	/** alt_sel_0 and alt_sel_1 to configure GPIO function (pinmux) */
	uint16_t alt_sel_1[3];
	/** 1 to configure output, and 0 to configure input */
	uint16_t dir[3];
	/** 1 to output high level, and 0 to configure output low level */
	uint16_t out_val[3];
	/** Reserved for open drain in future. Must be initialized to 0. */
	uint16_t reserve_1[3];
	/** Reserved for pull up enable in future. Must be initialized to 0. */
	uint16_t reserve_2[3];
	/** Reserved for pull up/down in future. Must be initialized to 0. */
	uint16_t reserve_3[3];
	/** unit is 1ms, the accuracy is 10ms. */
	uint32_t timeout_val;
};

/** \brief Representation of a sensor readout value.
 *
 * The value is represented as having an integer and a fractional part,
 * and can be obtained using the formula val1 + val2 * 10^(-6). Negative
 * values also adhere to the above formula, but may need special attention.
 * Here are some examples of the value representation:
 *
 *      0.5: val1 =  0, val2 =  500000
 *     -0.5: val1 =  0, val2 = -500000
 *     -1.0: val1 = -1, val2 =  0
 *     -1.5: val1 = -1, val2 = -500000
 *
 * Used by \ref sys_misc_pvt_temp and \ref sys_misc_pvt_voltage.
 */
struct sys_sensor_value {
	/** Integer part of the value. */
	int32_t val1;
	/** Fractional part of the value (in one-millionth parts). */
	int32_t val2;
};

/** @cond INTERNAL */
/** \brief Register read/write data structure
 *  Used by \ref sys_misc_reg_rd and \ref sys_misc_reg_wr.
 */
struct sys_reg_rw {
	/** 32-bit register address */
	uint32_t addr;
	/** register value */
	uint32_t val;
};

/** \brief Register read/write data structure
 *  Used by \ref sys_misc_reg_mod.
 */
struct sys_reg_mod {
	/** 32-bit register address */
	uint32_t addr;
	/** register value to write */
	uint32_t val;
	/** register value mask, bit value 1 to write, 0 to ignore */
	uint32_t mask;
};

enum {
	/** CML Clock 50Mhz */
	SYS_CML_CLK_50MHZ = 0,
	/** CML Clock 156.25MHz */
	SYS_CML_CLK_156P25MHz = 1,
	/** CML Clock from XO */
	SYS_CML_CLK_XO = 2,
	/** Number of clock source of CML Clock */
	SYS_CML_CLK_SRC_MAX,

	/** \cond INTERNAL */
	/** \brief force 32-bit enum type */
	SYS_CML_CLK_INTERNAL_SIZE = 0x7fffffff,
	/** \endcond */
};

/** \brief Enable/Disable/Config CML Clock Output
 *  Used by \ref sys_misc_cml_clk_get and \ref sys_misc_cml_clk_set.
 */
struct sys_cml_clk {
	/** select CML Clock Output (0 or 1) */
	uint32_t clk:		1;
	/** value 1 to enable or disable CML Clock Output */
	uint32_t en_val:	1;
	/** value 1 to enable CML Clock Output when en_val is 1 */
	uint32_t en:		1;
	/** value 1 to change clock source selection */
	uint32_t src_val:	1;
	/** select clock source when src_val is 1
	 *     0 - select LJPLL FOUT0 (50MHz)
	 *     1 - select LJPLL FOUT1 (156.25MHz)
	 *     2 - select XO (25MHz)
	 */
	uint32_t src_sel:	2;
};
/** @endcond INTERNAL */

/** \brief Config SFP
 *  Used by \ref sys_misc_sfp_get and \ref sys_misc_sfp_set.
 */
struct sys_sfp_cfg {
	/** Port id (0 or 1) */
	uint8_t	port_id: 4;
	/** config options
	 *    0 - SFP mode/speed/link-status
	 *    1 - flow control
	 *    2 - enable RX EQ adaptation (set only, return error if get)
	 */
	uint8_t option: 4;
	union {
		struct {
			/** select SFP mode
			 *	0 - auto
			 *	1 - fix
			 *	2 - disable
			 */
			uint8_t mode;
			/** select speed when mode is 1
			 *	0 - 10G Quad USXGMII
			 *	1 - 1000BaseX ANeg
			 *	2 - 10G	XFI
			 *	3 - 10G Single USXGMII
			 *	4 - 2.5G SGMII
			 *	5 - 2500 Single USXGMI
			 *	6 - 2500BaseX NonANeg
			 *	7 - 1000BaseX NonANeg
			 *	8 - 1G SGMI
			 */
			uint8_t speed;
			/** link status
			 *	0 - link down
			 *	1 - link up
			 */
			uint8_t link;
		};
		/** flow control
		 *   0 - disable
		 *   1 - enable
		 */
		uint8_t fc_en;
		/** RX EQ adaptation
		 *   0 - disable
		 *   1 - enable
		 */
		uint8_t adapt_en;
	};
};

/** \brief LLDP configuration set.
 *  Used by \ref sys_misc_lldp_set.
 */
struct sys_lldp_set {
	/** application port bit map for the lldp set port selection
	 *  bit 0 for PORT 1, and bit 15 for PORT 16
	 */
	uint16_t app_port_bitmap;
	/** operation mode, 0: disable; non-zero: enable,
	 *  the same operation mode will applied to all the selected ports
	 */
	uint8_t op_type;
	/** reserved */
	uint8_t reserved;
};

/** \brief LLDP configuration get.
 *  Used by \ref sys_misc_lldp_get.
 */
struct sys_lldp_get {
	/** port lldp mode setting bit map,
	 *  if the bit is 0, the corresponding ethernet port (ethX) is disabled;
	 *  and if the bit is 1, the corresponding ethernet port (ethX) is enabled
	 *  bit 0 is for eth0 and bit 15 is for eth15
	 */
	uint16_t state;
};

/** \brief IGMP snooping configuration.
 *  Used by \ref sys_misc_igmp_get and \ref sys_misc_igmp_set.
 */
struct sys_igmp_cfg {
	/** reserved for future */
	uint32_t mask;
	/** enable/disable igmp daemon
	 * 1 - enable
	 * 0 - disable
	 */
	uint8_t enable;
};

/** \brief STP configuration.
 *  Used by \ref sys_misc_stp_get and \ref sys_misc_stp_set.
 */
struct sys_stp_cfg {
	/** Each bit specifies edge port. Bit1 - port1, Bit2 - port2 ... */
	uint32_t stp_edge_ports;
	/** 0 - STP, 1 - RSTP */
	uint8_t mode; /* 0 - STP, 1 - RSTP */
	/** enable/disable stp daemon
	 * 1 - enable
	 * 0 - disable
	 */
	uint8_t enable;
	union {
		struct {
			/** RSTP migrate time (default is 3) */
			uint8_t migrate_time;
		} rstp;
	};
};

enum {
	/** special type used to get or delete IP
	 *  only used for get
	 */
	SYS_HOST_IP_TYPE_ANY = 0,
	/** IP assigned via autoconf (IPv6 only) */
	SYS_HOST_IP_TYPE_AUTOCONF,
	/** IP assigned via DHCP */
	SYS_HOST_IP_TYPE_DHCP,
	/** static IP can not be overridden */
	SYS_HOST_IP_TYPE_MANUAL,
	/** static IP can be overridden by DHCP */
	SYS_HOST_IP_TYPE_OVERRIDABLE,
	/** number of Host IP Types */
	SYS_HOST_IP_TYPE_MAX,

	/** \cond INTERNAL */
	/** \brief force 32-bit enum type */
	SYS_HOST_IP_TYPE_INTERNAL_SIZE = 0x7fffffff,
	/** \endcond */
};

enum daemon_id {
	DAEMON_SBDIO_WA = 0,
	DAEMON_FDMA_WA,
	DAEMON_LAG_LINK_UPDATE,
	DAEMON_MAC_LINK_CLR,
	DAEMON_CONGESTED_WRED,
	DAEMON_CONNECTED_WRED,
	DAEMON_PAUSE_FRAME,
	DAEMON_LINK_DETECT,
	DAEMON_PVT,
	DAEMON_USRA,
	DAEMON_SFP,
	DAEMON_RX_DROP,
	DAEMON_PHC_SYNC,
	DAEMON_VIRT_IF_STS_SYNC,
	DAEMON_POE,
	DAEMON_HTOL,
	DAEMON_LINK_MGR,
	DAEMON_PORTMAP_UPDATE,
	DAEMON_ID_NUM,
	DAEMON_ALWAYS_ON = DAEMON_ID_NUM
};

/** \brief Host IP configuration.
 *  Used by \ref sys_misc_host_ip_get and \ref sys_misc_host_ip_set.
 */
struct sys_host_ip_cfg {
	union {
		struct {
			union {
				/** IPv4 address
				 *  ip is output in get
				 *  ip is input in set
				 *  mask is output in get
				 *  mask is input in set and valid is 1
				 */
				uint8_t s4_addr[4];
				uint16_t s4_addr16[2];
				uint32_t s4_addr32[1];
			} ip, mask;
		};
		union {
			/** IPv6 address */
			uint8_t s6_addr[16];
			uint16_t s6_addr16[8];
			uint32_t s6_addr32[4];
		} ip6;
	};

	/** value 1 is IPv6, value 0 is IPv4
	 *  reserved for future
	 */
	uint8_t ipv6: 1;
	/** to indicate whether the IP is valid
	 *  in set function, IP is deleted if valid is 0 and
	 *  address match
	 */
	uint8_t valid: 1;
	/** enum SYS_HOST_IP_TYPE_xxx */
	uint8_t type: 3;
	/** reserved field */
	uint8_t res: 3;
};

/** \brief Program System MAC Address in Flash Header
 *  Used by \ref sys_misc_prg_mac
 */
struct sys_prg_mac {
	/** offset in flash header (0~0x0ffc)
	 *  must be 4-byte aligned
	 */
	uint16_t off;
	/** new System MAC address */
	uint8_t mac[6];
};

/** \brief Logical port ID (user view) to physcial port ID (HW view) map.
 *  This is used by customer who wants to change default mapping for
 *  customer board design.
 *  Used by \ref sys_misc_port_map_get and \ref sys_misc_port_map_set.
 */
struct sys_port_map {
	/** The port map array.
	 *  The first entry (map[0]) is reserved for internal use.
	 *  Each entry represent one port ID which is unique in this array and
	 *  valid in the switch.
	 */
	uint8_t map[17];
	/** The Number of ports that need to be mapped.
	 *  If used by \ref sys_misc_port_map_get, this returns number of ports
	 *  available. map[1] ~ map[length] has current port map.
	 *  If used by \ref sys_misc_port_map_set, this must be number of ports.
	 *  Suggest to use \ref sys_misc_port_map_get to get current map first,
	 *  modify, then use \ref sys_misc_port_map_set to change the map.
	 */
	uint8_t port_num;
};

/** \brief PTP timestamp correction calibration.
 *  Used by \ref sys_misc_ptp_ts_corr_cal_get and
 *  \ref sys_misc_ptp_ts_corr_cal_set.
 */
struct sys_ptp_ts_corr_cal {
	/** MAC id */
	uint8_t port_id;
	/** Ingress timestamp correction ns */
	uint32_t ingress_ns;
	/** Ingress timestamp correction sns */
	uint8_t ingress_sns;
	/** Ingress timestamp correction positive */
	uint8_t ingress_positive;
	/** Egress timestamp correction ns */
	uint32_t egress_ns;
	/** Egress timestamp correction sns */
	uint8_t egress_sns;
	/** Egress timestamp correction positive */
	uint8_t egress_positive;
};

/** \brief Data LED Speed Configuration.
 *  Used by \ref sys_misc_data_led_set.
 */
struct sys_data_led_cfg {
	/** Configure data LED blink speed:
	 *  1 - fast speed,
	 *  0 - slow speed.
	 */
	uint8_t fast;
};

/** \brief 10G Port Flow Control Threshold Ratio Configuration.
 *  This threshold is based on ratio of total resource (switch buffer).
 *  Used by \ref sys_misc_xg_fc_thr_ratio_get and
 *  \ref sys_misc_xg_fc_thr_ratio_set.
 */
struct sys_xg_fc_thr_ratio {
	/** Numerator of the ratio. Must be less than denominator. */
	uint16_t num;
	/** Denominator of the ratio. */
	uint16_t den;
};

/** \brief Daemon Control Configuration.
 *  Used by \ref sys_misc_daemon_get and \ref sys_misc_daemon_set.
 */
struct sys_daemon_cfg {
	/** Daemon ID (from enum daemon_id) */
	uint8_t id;
	union {
		/** Daemon state: 1 - running, 0 - suspended */
		uint8_t state;
	};
};

/** \brief PCE Service Policy Configuration.
 *  Used by \ref sys_misc_pce_svc_set.
 *  Each bit in the bitmaps corresponds to a service (PCE_SVC_BIT_*).
 *  These bits control service startup decisions in main_pthread.
 */

/** @name PCE Service Bitmap Bits
 * @{
 */
#define PCE_SVC_BIT_IGMP	BIT(0)	/**< IGMP snooping */
#define PCE_SVC_BIT_MLD		BIT(1)	/**< MLD snooping */
#define PCE_SVC_BIT_EAPOL	BIT(2)	/**< 802.1X EAPOL (reserved) */
#define PCE_SVC_BIT_STP		BIT(3)	/**< STP/RSTP */
#define PCE_SVC_BIT_LLDP	BIT(4)	/**< LLDP protocol */
#define PCE_SVC_BIT_LOOP_DETECT	BIT(5)	/**< Loop Detection */
#define PCE_SVC_BIT_LACP	BIT(6)	/**< LACP/LAG (reserved) */
#define PCE_SVC_BIT_PTP		BIT(7)	/**< gPTP/PTP */
#define PCE_SVC_BIT_DSA		BIT(8)	/**< DSA tag extraction (reserved) */
#define PCE_SVC_BIT_WEB		BIT(9)	/**< CivetWeb management */
#define PCE_SVC_BIT_SNTP	BIT(10)	/**< SNTP client */
#define PCE_SVC_BIT_MQTT	BIT(11)	/**< MQTT client */
#define PCE_SVC_BIT_SNMP	BIT(12)	/**< SNMP agent */
#define PCE_SVC_BIT_ETH_FW	BIT(13)	/**< Ethernet FW upgrade */
#define PCE_SVC_BIT_ETH_FW_TO	BIT(14)	/**< Ethernet FW upgrade auto-disable */
#define PCE_SVC_BIT_ICMP_DOS	BIT(15)	/**< ICMP DoS prevention (reserved) */
#define PCE_SVC_BIT_PORTMAP_UPD	BIT(29) /**< Portmap Update Daemon */
#define PCE_SVC_BIT_SFP		BIT(30)	/**< SFP Daemon */
#define PCE_SVC_BIT_POE		BIT(31)	/**< PoE Companion Chip Support */
#define PCE_SVC_BIT_ALL		~0u	/**< All services */
/** @} */

struct sys_pce_svc_cfg {
	/** Bitmap of services to enable (PCE_SVC_BIT_*) */
	uint32_t enable;
	/** Bitmap of services to disable (PCE_SVC_BIT_*).
	 *  Disable takes precedence over enable. */
	uint32_t disable;
};

#pragma scalar_storage_order default
#pragma pack(pop)

/**
   \brief Trigger switch device to enter firmware upgrade mode.

   \param dev Pointer to GSW_Device_t (defined by application).

   \remarks The function returns an error code of Zephyr in case error occurs.

   \return Return value as follows:
   - 0: if successful
   - negative: error code
*/
int sys_misc_fw_update(const GSW_Device_t *dev);

/**
   \brief Get firmware version number.

   \param dev Pointer to GSW_Device_t (defined by application).
   \param sys_img_ver Pointer to \ref sys_fw_image_version.

   \remarks The function returns an error code of Zephyr in case error occurs.

   \return Return value as follows:
   - 0: if successful
   - negative: error code
*/
int sys_misc_fw_version(const GSW_Device_t *dev, struct sys_fw_image_version *sys_img_ver);

/**
   \brief Get switch device temperature info.

   \param dev Pointer to GSW_Device_t (defined by application).
   \param sys_temp_val Pointer to \ref sys_sensor_value.

   \remarks The function returns an error code of Zephyr in case error occurs.

   \return Return value as follows:
   - 0: if successful
   - negative: error code
*/
int sys_misc_pvt_temp(const GSW_Device_t *dev, struct sys_sensor_value *sys_temp_val);

/**
   \brief Get switch device voltage info.

   \param dev Pointer to GSW_Device_t (defined by application).
   \param sys_voltage Pointer to \ref sys_sensor_value.

   \remarks The function returns an error code of Zephyr in case error occurs.

   \return Return value as follows:
   - 0: if successful
   - negative: error code
*/
int sys_misc_pvt_voltage(const GSW_Device_t *dev, struct sys_sensor_value *sys_voltage);

/**
   \brief Wait for specified delay time.

   \param dev Pointer to GSW_Device_t (defined by application).
   \param pdelay Pointer to \ref sys_delay.

   \remarks The function returns an error code of Zephyr in case error occurs.

   \return Return value as follows:
   - 0: if successful
   - negative: error code
*/
int sys_misc_delay(const GSW_Device_t *dev, struct sys_delay *pdelay);

/**
   \brief Configure GPIO function (pinmux), direction and output level.

   \param dev Pointer to GSW_Device_t (defined by application).
   \param sys_gpio_conf Pointer to \ref sys_gpio_config.

   \remarks The function returns an error code of Zephyr in case error occurs.

   \return Return value as follows:
   - 0: if successful
   - negative: error code
*/
int sys_misc_gpio_configure(const GSW_Device_t *dev, struct sys_gpio_config *sys_gpio_conf);

/**
   \brief Reboot switch device.

   \param dev Pointer to GSW_Device_t (defined by application).

   \remarks The function returns an error code of Zephyr in case error occurs.

   \return Return value as follows:
   - 0: if successful
   - negative: error code
*/
int sys_misc_reboot(const GSW_Device_t *dev);

/** @cond INTERNAL */
/**
   \brief Register read.

   \param dev Pointer to GSW_Device_t (defined by application).
   \param sys_reg Pointer to \ref sys_reg_rw.

   \remarks The function returns an error code of Zephyr in case error occurs.

   \return Return value as follows:
   - 0: if successful
   - negative: error code
*/
int sys_misc_reg_rd(const GSW_Device_t *dev, struct sys_reg_rw *sys_reg);

/**
   \brief Register write.

   \param dev Pointer to GSW_Device_t (defined by application).
   \param sys_reg Pointer to \ref sys_reg_rw.

   \remarks The function returns an error code of Zephyr in case error occurs.

   \return Return value as follows:
   - 0: if successful
   - negative: error code
*/
int sys_misc_reg_wr(const GSW_Device_t *dev, struct sys_reg_rw *sys_reg);

/**
   \brief Register modification (read, modify, then write).

   \param dev Pointer to GSW_Device_t (defined by application).
   \param sys_reg Pointer to \ref sys_reg_mod.

   \remarks The function returns an error code of Zephyr in case error occurs.

   \return Return value as follows:
   - 0: if successful
   - negative: error code
*/
int sys_misc_reg_mod(const GSW_Device_t *dev, struct sys_reg_mod *sys_reg);

/**
   \brief CML clock source get.

   \param dev Pointer to GSW_Device_t (defined by application).
   \param clk Pointer to \ref sys_cml_clk.

   \remarks The function returns an error code of Zephyr in case error occurs.

   \return Return value as follows:
   - 0: if successful
   - negative: error code
*/
int sys_misc_cml_clk_get(const GSW_Device_t *dev, struct sys_cml_clk *clk);

/**
   \brief CML clock source set.

   \param dev Pointer to GSW_Device_t (defined by application).
   \param clk Pointer to \ref sys_cml_clk.

   \remarks The function returns an error code of Zephyr in case error occurs.

   \return Return value as follows:
   - 0: if successful
   - negative: error code
*/
int sys_misc_cml_clk_set(const GSW_Device_t *dev, struct sys_cml_clk *clk);
/** @endcond INTERNAL */

/**
   \brief SFP mode/config get.

   \param dev Pointer to GSW_Device_t (defined by application).
   \param cfg Pointer to \ref sys_sfp_cfg.

   \remarks The function returns an error code of Zephyr in case error occurs.

   \return Return value as follows:
   - 0: if successful
   - negative: error code
*/
int sys_misc_sfp_get(const GSW_Device_t *dev, struct sys_sfp_cfg *cfg);

/**
   \brief SFP mode/config set.

   \param dev Pointer to GSW_Device_t (defined by application).
   \param cfg Pointer to \ref sys_sfp_cfg.

   \remarks The function returns an error code of Zephyr in case error occurs.

   \return Return value as follows:
   - 0: if successful
   - negative: error code
*/
int sys_misc_sfp_set(const GSW_Device_t *dev, struct sys_sfp_cfg *cfg);

/**
   \brief LLDP status get.

   \param dev Pointer to GSW_Device_t (defined by application).
   \param lldp_sts Pointer to \ref sys_lldp_get.

   \remarks The function returns an error code of Zephyr in case error occurs.

   \return Return value as follows:
   - 0: if successful
   - negative: error code
*/
int sys_misc_lldp_get(const GSW_Device_t *dev, struct sys_lldp_get *lldp_sts);

/**
   \brief LLDP config set.

   \param dev Pointer to GSW_Device_t (defined by application).
   \param lldp_op Pointer to \ref sys_lldp_set.

   \remarks The function returns an error code of Zephyr in case error occurs.

   \return Return value as follows:
   - 0: if successful
   - negative: error code
*/
int sys_misc_lldp_set(const GSW_Device_t *dev, struct sys_lldp_set *lldp_op);

/**
   \brief IGMP config get.

   \param dev Pointer to GSW_Device_t (defined by application).
   \param cfg Pointer to \ref sys_igmp_cfg.

   \remarks The function returns an error code of Zephyr in case error occurs.

   \return Return value as follows:
   - 0: if successful
   - negative: error code
*/
int sys_misc_igmp_get(const GSW_Device_t *dev, struct sys_igmp_cfg *cfg);

/**
   \brief IGMP config set.

   \param dev Pointer to GSW_Device_t (defined by application).
   \param cfg Pointer to \ref sys_igmp_cfg.

   \remarks The function returns an error code of Zephyr in case error occurs.

   \return Return value as follows:
   - 0: if successful
   - negative: error code
*/
int sys_misc_igmp_set(const GSW_Device_t *dev, struct sys_igmp_cfg *cfg);

/**
   \brief IGMP report flood get.

   \param dev Pointer to GSW_Device_t (defined by application).
   \param cfg Pointer to \ref sys_igmp_cfg.

   \remarks The function returns an error code of Zephyr in case error occurs.

   \return Return value as follows:
   - 0: if successful
   - negative: error code
*/
int sys_misc_igmp_report_flood_get(const GSW_Device_t *dev, struct sys_igmp_cfg *cfg);

/**
   \brief IGMP report flood set.

   \param dev Pointer to GSW_Device_t (defined by application).
   \param cfg Pointer to \ref sys_igmp_cfg.

   \remarks The function returns an error code of Zephyr in case error occurs.

   \return Return value as follows:
   - 0: if successful
   - negative: error code
*/
int sys_misc_igmp_report_flood_set(const GSW_Device_t *dev, struct sys_igmp_cfg *cfg);
/**
   \brief MLD config get.

   \param dev Pointer to GSW_Device_t (defined by application).
   \param cfg Pointer to \ref sys_igmp_cfg.

   \remarks The function returns an error code of Zephyr in case error occurs.

   \return Return value as follows:
   - 0: if successful
   - negative: error code
*/
int sys_misc_mld_get(const GSW_Device_t *dev, struct sys_igmp_cfg *cfg);

/**
   \brief MLD config set.

   \param dev Pointer to GSW_Device_t (defined by application).
   \param cfg Pointer to \ref sys_igmp_cfg.

   \remarks The function returns an error code of Zephyr in case error occurs.

   \return Return value as follows:
   - 0: if successful
   - negative: error code
*/
int sys_misc_mld_set(const GSW_Device_t *dev, struct sys_igmp_cfg *cfg);

/**
   \brief STP config get.

   \param dev Pointer to GSW_Device_t (defined by application).
   \param cfg Pointer to \ref sys_stp_cfg.

   \remarks The function returns an error code of Zephyr in case error occurs.

   \return Return value as follows:
   - 0: if successful
   - negative: error code
*/
int sys_misc_stp_get(const GSW_Device_t *dev, struct sys_stp_cfg *cfg);

/**
   \brief STP config set.

   \param dev Pointer to GSW_Device_t (defined by application).
   \param cfg Pointer to \ref sys_stp_cfg.

   \remarks The function returns an error code of Zephyr in case error occurs.

   \return Return value as follows:
   - 0: if successful
   - negative: error code
*/
int sys_misc_stp_set(const GSW_Device_t *dev, struct sys_stp_cfg *cfg);

/**
   \brief Host IP get by type.

   \param dev Pointer to GSW_Device_t (defined by application).
   \param cfg Pointer to \ref sys_host_ip_cfg.

   \remarks The function returns an error code of Zephyr in case error occurs.

   \return Return value as follows:
   - 0: if successful
   - negative: error code
*/
int sys_misc_host_ip_get(const GSW_Device_t *dev, struct sys_host_ip_cfg *cfg);

/**
   \brief Host IP set by type.

   \param dev Pointer to GSW_Device_t (defined by application).
   \param cfg Pointer to \ref sys_host_ip_cfg.

   \remarks The function returns an error code of Zephyr in case error occurs.

   \return Return value as follows:
   - 0: if successful
   - negative: error code
*/
int sys_misc_host_ip_set(const GSW_Device_t *dev, struct sys_host_ip_cfg *cfg);

/**
   \brief Program System MAC Address in Flash Header.

   \param dev Pointer to GSW_Device_t (defined by application).
   \param cfg Pointer to \ref sys_prg_mac.

   \remarks The function returns an error code of Zephyr in case error occurs.

   \return Return value as follows:
   - 0: if successful
   - negative: error code
*/
int sys_misc_prg_mac(const GSW_Device_t *dev, struct sys_prg_mac *cfg);

/**
   \brief Get port ID (user view) to physcial port ID (HW view) map.

   \param dev Pointer to GSW_Device_t (defined by application).
   \param cfg Pointer to \ref sys_port_map.

   \remarks The function returns an error code of Zephyr in case error occurs.

   \return Return value as follows:
   - 0: if successful
   - negative: error code
*/

int sys_misc_port_map_get(const GSW_Device_t *dev, struct sys_port_map *cfg);


/**
   \brief Configure port ID (user view) to physcial port ID (HW view) map.

   \param dev Pointer to GSW_Device_t (defined by application).
   \param cfg Pointer to \ref sys_port_map.

   \remarks The function returns an error code of Zephyr in case error occurs.

   \return Return value as follows:
   - 0: if successful
   - negative: error code
*/

int sys_misc_port_map_set(const GSW_Device_t *dev, struct sys_port_map *cfg);

/**
   \brief PTP timestamp correction calibration get.

   \param dev Pointer to GSW_Device_t (defined by application).
   \param cfg Pointer to \ref sys_ptp_ts_corr_cal.

   \remarks The function returns an error code of Zephyr in case error occurs.

   \return Return value as follows:
   - 0: if successful
   - negative: error code
*/
int sys_misc_ptp_ts_corr_cal_get(const GSW_Device_t *dev, struct sys_ptp_ts_corr_cal *cfg);

/**
   \brief PTP timestamp correction calibration set.

   \param dev Pointer to GSW_Device_t (defined by application).
   \param cfg Pointer to \ref sys_ptp_ts_corr_cal.

   \remarks The function returns an error code of Zephyr in case error occurs.

   \return Return value as follows:
   - 0: if successful
   - negative: error code
*/
int sys_misc_ptp_ts_corr_cal_set(const GSW_Device_t *dev, struct sys_ptp_ts_corr_cal *cfg);

/**
    \brief Data LED speed set.

    \param dev Pointer to GSW_Device_t (defined by application).
    \param led_cfg Pointer to \ref sys_data_led_cfg.

    \remarks The function returns an error code of Zephyr in case error occurs.

    \return Return value as follows:
    - 0: if successful
    - negative: error code
*/
int sys_misc_data_led_set(const GSW_Device_t *dev, struct sys_data_led_cfg *led_cfg);

/**
   \brief Get 10G port flow control threshold ratio.

   \param dev Pointer to GSW_Device_t (defined by application).
   \param pcfg Pointer to \ref sys_xg_fc_thr_ratio.

   \remarks The function returns an error code of Zephyr in case error occurs.

   \return Return value as follows:
   - 0: if successful
   - negative: error code
*/
int sys_misc_xg_fc_thr_ratio_get(const GSW_Device_t *dev, struct sys_xg_fc_thr_ratio *pcfg);

/**
   \brief Set 10G port flow control threshold ratio.

   \param dev Pointer to GSW_Device_t (defined by application).
   \param pcfg Pointer to \ref sys_xg_fc_thr_ratio.

   \remarks The function returns an error code of Zephyr in case error occurs.
            The numerator (num) must be less than denominator (den).

   \return Return value as follows:
   - 0: if successful
   - negative: error code
*/
int sys_misc_xg_fc_thr_ratio_set(const GSW_Device_t *dev, struct sys_xg_fc_thr_ratio *pcfg);

/**
   \brief Get daemon running state.

   \param dev Pointer to GSW_Device_t (defined by application).
   \param pcfg Pointer to \ref sys_daemon_cfg.

   \remarks The function returns an error code of Zephyr in case error occurs.

   \return Return value as follows:
   - 0: if successful
   - negative: error code
*/
int sys_misc_daemon_get(const GSW_Device_t *dev, struct sys_daemon_cfg *pcfg);

/**
   \brief Set daemon running state (suspend/resume).

   \param dev Pointer to GSW_Device_t (defined by application).
   \param pcfg Pointer to \ref sys_daemon_cfg.

   \remarks The function returns an error code of Zephyr in case error occurs.
            State 1 resumes the daemon, state 0 suspends the daemon.

   \return Return value as follows:
   - 0: if successful
   - negative: error code
*/
int sys_misc_daemon_set(const GSW_Device_t *dev, struct sys_daemon_cfg *pcfg);

/**
   \brief Set PCE service policy from Start Config.

   Stores service enable/disable bitmaps for main_pthread to query
   when deciding which services to start.

   \param dev Pointer to GSW_Device_t (defined by application).
   \param pcfg Pointer to \ref sys_pce_svc_cfg.

   \return Return value as follows:
   - 0: if successful
   - negative: error code
*/
int sys_misc_pce_svc_set(const GSW_Device_t *dev, struct sys_pce_svc_cfg *pcfg);

/**
   \brief Get the stored PCE service policy.

   Returns the policy set by sys_misc_pce_svc_set().
   Used by main_pthread to check which services to start.

   \param dev Pointer to GSW_Device_t (defined by application).
   \param pcfg Pointer to \ref sys_pce_svc_cfg (output).

   \return Return value as follows:
   - 0: if successful
   - negative: error code
*/
int sys_misc_pce_svc_get(const GSW_Device_t *dev, struct sys_pce_svc_cfg *pcfg);

/** @} */	/* DOC_SYS_MISC */

#endif
