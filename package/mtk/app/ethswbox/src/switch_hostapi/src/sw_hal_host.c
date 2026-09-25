/******************************************************************************

   Copyright 2026 MaxLinear, Inc.

   For licensing information, see the file 'LICENSE' in the root folder of
   this software module.

******************************************************************************/

#include "host_adapt.h"
#include "host_api_impl.h"

int sw_hal_host_lag_cfg_get(const GSW_Device_t *dev, struct sw_hal_host_cfg *cfg)
{
	return gsw_api_wrap(dev,
			    SW_HAL_HOST_LAGCFGGET,
			    cfg,
			    sizeof(*cfg),
			    0,
			    sizeof(*cfg));
}

int sw_hal_host_lag_cfg_set(const GSW_Device_t *dev, struct sw_hal_host_cfg *cfg)
{
	return gsw_api_wrap(dev,
			    SW_HAL_HOST_LAGCFGSET,
			    cfg,
			    sizeof(*cfg),
			    SW_HAL_HOST_LAGCFGGET,
			    0);
}

int host_poe_global_cfg_set(const GSW_Device_t *dev,
				    struct host_poe_global_cfg *cfg)
{
	return gsw_api_wrap(dev,
			    HOST_POE_GLOBAL_CFG_SET,
			    cfg,
			    sizeof(*cfg),
			    HOST_POE_GLOBAL_STATUS_GET,
			    0);
}

int host_poe_port_cfg_set(const GSW_Device_t *dev,
				  struct host_poe_port_cfg *cfg)
{
	return gsw_api_wrap(dev,
			    HOST_POE_PORT_CFG_SET,
			    cfg,
			    sizeof(*cfg),
			    HOST_POE_PORT_CFG_GET,
			    0);
}

int host_poe_port_cfg_get(const GSW_Device_t *dev,
				  struct host_poe_port_status *status)
{
	return gsw_api_wrap(dev,
			    HOST_POE_PORT_CFG_GET,
			    status,
			    sizeof(*status),
			    0,
			    sizeof(*status));
}

int host_poe_global_status_get(const GSW_Device_t *dev,
				       struct host_poe_global_status *status)
{
	return gsw_api_wrap(dev,
			    HOST_POE_GLOBAL_STATUS_GET,
			    status,
			    sizeof(*status),
			    0,
			    sizeof(*status));
}