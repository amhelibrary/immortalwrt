/******************************************************************************

   Copyright 2026 MaxLinear, Inc.

   For licensing information, see the file 'LICENSE' in the root folder of
   this software module.

******************************************************************************/

#ifndef _SW_HAL_HOST_H_
#define _SW_HAL_HOST_H_

#pragma pack(push, 1)
struct sw_hal_host_cfg {
	uint8_t nGrpId;
	uint8_t reserved;
	uint32_t nPortMap;
};

struct host_poe_global_cfg {
	uint8_t  enable;     /**< 0 = disable, 1 = enable */
	uint8_t  reserved;
	uint16_t total_pwr;  /**< trunk power budget (W), valid when enable=1 */
};

struct host_poe_port_cfg {
	uint8_t nPortId;  /**< 0-based port index */
	uint8_t enable;   /**< 0 = disabled, 1 = enabled */
	uint8_t mode;     /**< 0 = AF, 1 = AT */
	uint8_t reserved;
};

struct host_poe_port_status {
	uint8_t  nPortId;      /**< 0-based port index (input) */
	uint8_t  pwr_on;       /**< 1 if PD powered, 0 if off */
	uint8_t  admin;        /**< current admin state: 0=disabled, 1=enabled */
	uint8_t  mode;         /**< current AF/AT mode: 0=AF, 1=AT */
	uint8_t  pd_class;     /**< PD IEEE class (0-4, 0xFF if unknown) */
	uint8_t  event;        /**< latest port event bitmask */
	uint16_t current_ma;   /**< port current in mA */
	uint16_t temperature;  /**< raw temperature register value */
};

struct host_poe_global_status {
	uint8_t  enable;       /**< 1 if PoE globally enabled */
	uint8_t  reserved;
	uint16_t total_pwr;    /**< configured trunk power budget (W) */
	uint32_t supply_mv;    /**< main supply voltage in mV */
	uint8_t  pwr_mode;     /**< power management mode reported by chip */
	uint8_t  chip_avail;   /**< 1 if MXL86038 detected and accessible */
};

#pragma pack(pop)

int sw_hal_host_lag_cfg_set(const GSW_Device_t *dev, struct sw_hal_host_cfg *cfg);
int sw_hal_host_lag_cfg_get(const GSW_Device_t *dev, struct sw_hal_host_cfg *cfg);

int host_poe_global_cfg_set(const GSW_Device_t *dev, struct host_poe_global_cfg *cfg);
int host_poe_global_status_get(const GSW_Device_t *dev, struct host_poe_global_status *status);
int host_poe_port_cfg_set(const GSW_Device_t *dev, struct host_poe_port_cfg *cfg);
int host_poe_port_cfg_get(const GSW_Device_t *dev, struct host_poe_port_status *status);

#endif /* _SW_HAL_HOST_H_ */