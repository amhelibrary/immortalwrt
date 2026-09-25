/******************************************************************************

   Copyright 2026 MaxLinear, Inc.

   For licensing information, see the file 'LICENSE' in the root folder of
   this software module.

******************************************************************************/

#include "host_adapt.h"
#include "host_api_impl.h"

int host_mac_rd_reg(const GSW_Device_t *dev, host_mac_register_t *param)
{
	return gsw_api_wrap(dev,
			    MAC_REGISTER_GET,
			    param,
			    sizeof(*param),
			    0,
			    sizeof(*param));
}

int host_mac_wr_reg(const GSW_Device_t *dev, host_mac_register_t *param)
{
	return gsw_api_wrap(dev,
			    MAC_REGISTER_SET,
			    param,
			    sizeof(*param),
			    0,
			    0);
}
int host_mac_rmon_get(const GSW_Device_t *dev, host_mac_rmon_t *prmon)
{
	memset(prmon->cnt, 0, sizeof(prmon->cnt));
	return gsw_api_wrap(dev,
			    MAC_RMON_GET,
			    prmon,
			    sizeof(*prmon),
			    0,
			    sizeof(*prmon));
}

int host_mac_rmon_clr(const GSW_Device_t *dev, uint8_t *pidx)
{
	return gsw_api_wrap(dev,
			    MAC_RMON_CLEAR,
			    pidx,
			    sizeof(*pidx),
			    0,
			    0);
}
