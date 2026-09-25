/******************************************************************************

   Copyright 2023-2024 MaxLinear, Inc.

   For licensing information, see the file 'LICENSE' in the root folder of
   this software module.

******************************************************************************/

#include "host_adapt.h"
#include "host_api_impl.h"

#if defined(__has_builtin)
#if __has_builtin(__builtin_popcount)
#define HAVE_BUILTIN_POPCOUNT	1
#endif
#elif defined(__GNUC__) || defined(__clang__)
#define HAVE_BUILTIN_POPCOUNT	1
#endif

#ifdef HAVE_BUILTIN_POPCOUNT
static inline int popcount(uint32_t x)
{
	return __builtin_popcount(x);
}
#else
static inline int popcount(uint32_t x)
{
	int cnt = 0;

	for (; x != 0; x >>= 1) {
		if ((x & BIT(0)) != 0)
			cnt++;
	}

	return cnt;
}
#endif

int int_gphy_read(const GSW_Device_t *dev, struct mdio_relay_data *parm)
{
	return gsw_api_wrap(dev,
			    INT_GPHY_READ,
			    parm,
			    sizeof(*parm),
			    0,
			    sizeof(parm->data));
}

int int_gphy_write(const GSW_Device_t *dev, struct mdio_relay_data *parm)
{
	return gsw_api_wrap(dev,
			    INT_GPHY_WRITE,
			    parm,
			    sizeof(*parm),
			    0,
			    0);
}

int int_gphy_mod(const GSW_Device_t *dev, struct mdio_relay_mod_data *parm)
{
	return gsw_api_wrap(dev,
			    INT_GPHY_MOD,
			    parm,
			    sizeof(*parm),
			    0,
			    0);
}

int int_gphy_bulk_read(const GSW_Device_t *dev, union mdio_bulk_relay_data *parm)
{
	uint32_t phy_num = popcount(parm->input.phy_map);
	uint32_t size = phy_num * (1 + parm->input.num) * sizeof(parm->data[0]);

	return gsw_api_wrap(dev,
			    INT_GPHY_BULK_READ,
			    parm,
			    sizeof(*parm),
			    0,
			    size);
}

int ext_mdio_read(const GSW_Device_t *dev, struct mdio_relay_data *parm)
{
	return gsw_api_wrap(dev,
			    EXT_MDIO_READ,
			    parm,
			    sizeof(*parm),
			    0,
			    sizeof(parm->data));
}

int ext_mdio_write(const GSW_Device_t *dev, struct mdio_relay_data *parm)
{
	return gsw_api_wrap(dev,
			    EXT_MDIO_WRITE,
			    parm,
			    sizeof(*parm),
			    0,
			    0);
}

int ext_mdio_mod(const GSW_Device_t *dev, struct mdio_relay_mod_data *parm)
{
	return gsw_api_wrap(dev,
			    EXT_MDIO_MOD,
			    parm,
			    sizeof(*parm),
			    0,
			    0);
}

