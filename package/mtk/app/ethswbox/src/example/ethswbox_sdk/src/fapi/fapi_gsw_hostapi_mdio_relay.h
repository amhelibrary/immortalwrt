#ifndef _FAPI_GSW_HOSTAPI_MDIO_RELAY_H_
#define _FAPI_GSW_HOSTAPI_MDIO_RELAY_H_

/******************************************************************************

    Copyright 2022 Maxlinear

    SPDX-License-Identifier: (BSD-3-Clause OR GPL-2.0-only)

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.
******************************************************************************/

#include <mmd_apis.h>

int fapi_int_gphy_read(int prmc, char *prmv[]);
/* write internal GPHY MDIO/MMD registers */
int fapi_int_gphy_write(int prmc, char *prmv[]);
/* modify internal GPHY MDIO/MMD registers */
int fapi_int_gphy_mod(int prmc, char *prmv[]);

/* read external GPHY MDIO/MMD registers via MDIO bus */
int fapi_ext_mdio_read(int prmc, char *prmv[]);
/* write external GPHY MDIO/MMD registers via MDIO bus */
int fapi_ext_mdio_write(int prmc, char *prmv[]);
/* modify external GPHY MDIO/MMD registers via MDIO bus */
int fapi_ext_mdio_mod(int prmc, char *prmv[]);

/* Direct GPIO bit-bang MDIO access (Clause-22) */
int fapi_c22_mdio_read(int prmc, char *prmv[]);
int fapi_c22_mdio_write(int prmc, char *prmv[]);

/* Direct GPIO bit-bang MDIO access (Clause-45) */
int fapi_c45_mdio_read(int prmc, char *prmv[]);
int fapi_c45_mdio_write(int prmc, char *prmv[]);

/* SMDIO (Serial MDIO / SSB) access */
int fapi_smdio_read(int prmc, char *prmv[]);
int fapi_smdio_write(int prmc, char *prmv[]);
int fapi_rescue_enhwuart(int prmc, char *prmv[]);

#ifdef ETHSWBOX_RPI4EVK
/* Force a full GPIO auto-scan and refresh the pin cache.
 * Run this once after initial setup or when wiring changes.
 * Subsequent fapi commands use the cached result and are fast. */
int fapi_lif_scan(int prmc, char *prmv[]);
int fapi_lif_pin_set(int prmc, char *prmv[]);
#endif

#endif /* _FAPI_GSW_HOSTAPI_MDIO_RELAY_H_ */

