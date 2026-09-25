/******************************************************************************

   Copyright 2026 MaxLinear, Inc.

   For licensing information, see the file 'LICENSE' in the root folder of
   this software module.

******************************************************************************/

#include "host_adapt.h"
#include "host_api_impl.h"

/* ================================================================
 * Phase 1: Core PCS operations
 * ================================================================ */

/**
 * xpcs_ext_pcs_config - Configure PCS mode and auto-negotiation
 * @dev:  MDIO device context
 * @cfg:  PCS configuration (in/out)
 *
 * Return: 0 on success, negative errno on MDIO/transport error.
 *         cfg->result carries the firmware-level return value.
 */
int xpcs_ext_pcs_config(const GSW_Device_t *dev, struct xpcs_pcs_cfg *cfg)
{
	return gsw_api_wrap(dev,
			    XPCS_PCS_CONFIG,
			    cfg,
			    sizeof(*cfg),
			    0,
			    sizeof(*cfg));
}

/**
 * xpcs_ext_pcs_get_state - Read resolved PCS link state
 * @dev:   MDIO device context
 * @state: PCS state (in/out)
 *
 * Return: 0 on success, negative errno on MDIO/transport error.
 */
int xpcs_ext_pcs_get_state(const GSW_Device_t *dev,
			   struct xpcs_pcs_state *state)
{
	return gsw_api_wrap(dev,
			    XPCS_PCS_GET_STATE,
			    state,
			    sizeof(*state),
			    0,
			    sizeof(*state));
}

/**
 * xpcs_ext_pcs_disable - Disable (power down) SERDES
 * @dev: MDIO device context
 * @pwr: Power config (in/out)
 *
 * Return: 0 on success, negative errno on MDIO/transport error.
 *         pwr->result carries the firmware-level return value.
 */
int xpcs_ext_pcs_disable(const GSW_Device_t *dev, struct xpcs_pcs_disable *pcfg)
{
	return gsw_api_wrap(dev,
			    XPCS_PCS_DISABLE,
			    pcfg,
			    sizeof(*pcfg),
			    0,
			    sizeof(*pcfg));
}

/* ================================================================
 * Phase 2: AN Control
 * ================================================================ */

/**
 * xpcs_ext_an_restart - Restart auto-negotiation (CL37 or CL73)
 * @dev: MDIO device context
 * @cfg: AN restart config (in/out)
 *
 * Return: 0 on success, negative errno on MDIO/transport error.
 *         cfg->result carries the firmware-level return value.
 */
int xpcs_ext_an_restart(const GSW_Device_t *dev, struct xpcs_an_restart *cfg)
{
	return gsw_api_wrap(dev,
			    XPCS_AN_RESTART,
			    cfg,
			    sizeof(*cfg),
			    0,
			    sizeof(*cfg));
}

/* ================================================================
 * Phase 3: Force / Utility
 * ================================================================ */

/**
 * xpcs_ext_pcs_link_up - link-up event with resolved speed/duplex.
 * @dev: MDIO device context
 * @cfg: Force speed config (in/out)
 *
 * Return: 0 on success, negative errno on MDIO/transport error.
 *         cfg->result carries the firmware-level return value.
 */
int xpcs_ext_pcs_link_up(const GSW_Device_t *dev, struct xpcs_pcs_link_up *cfg)
{
	return gsw_api_wrap(dev,
			    XPCS_PCS_LINK_UP,
			    cfg,
			    sizeof(*cfg),
			    0,
			    sizeof(*cfg));
}

/**
 * xpcs_ext_loopback - Set/clear PCS or PMA loopback
 * @dev: MDIO device context
 * @cfg: Loopback config (in/out)
 *
 * Return: 0 on success, negative errno on MDIO/transport error.
 *         cfg->result carries the firmware-level return value.
 */
int xpcs_ext_loopback(const GSW_Device_t *dev, struct xpcs_loopback_cfg *cfg)
{
	return gsw_api_wrap(dev,
			    XPCS_LOOPBACK,
			    cfg,
			    sizeof(*cfg),
			    0,
			    sizeof(*cfg));
}

/**
 * xpcs_ext_reset - Reset the XPCS (VR, soft, or hard)
 * @dev: MDIO device context
 * @cfg: Reset config (in/out)
 *
 * Return: 0 on success, negative errno on MDIO/transport error.
 *         cfg->result carries the firmware-level return value.
 */
int xpcs_ext_reset(const GSW_Device_t *dev, struct xpcs_reset_cfg *cfg)
{
	return gsw_api_wrap(dev,
			    XPCS_RESET,
			    cfg,
			    sizeof(*cfg),
			    0,
			    sizeof(*cfg));
}

/* ================================================================
 * Phase 4: Diagnostics
 * ================================================================ */

/**
 * xpcs_ext_prbs_cfg - Configure PCS-level PRBS31 test pattern
 * @dev: MDIO device context
 * @cfg: PRBS config (in/out). On return, rx_err_cnt is populated.
 *
 * Return: 0 on success, negative errno on MDIO/transport error.
 *         cfg->result carries the firmware-level return value.
 */
int xpcs_ext_prbs_cfg(const GSW_Device_t *dev, struct xpcs_prbs_cfg *cfg)
{
	return gsw_api_wrap(dev,
			    XPCS_PRBS_CFG,
			    cfg,
			    sizeof(*cfg),
			    0,
			    sizeof(*cfg));
}

/**
 * xpcs_ext_bert_cfg - Configure SerDes-level BERT pattern
 * @dev: MDIO device context
 * @cfg: BERT config (in/out). On return, rx_err_cnt is populated.
 *
 * Return: 0 on success, negative errno on MDIO/transport error.
 *         cfg->result carries the firmware-level return value.
 */
int xpcs_ext_bert_cfg(const GSW_Device_t *dev, struct xpcs_bert_cfg *cfg)
{
	return gsw_api_wrap(dev,
			    XPCS_BERT_CFG,
			    cfg,
			    sizeof(*cfg),
			    0,
			    sizeof(*cfg));
}

/**
 * xpcs_ext_eq_get - Read TX and RX equalization status
 * @dev: MDIO device context
 * @cfg: EQ get request (in/out)
 *
 * Return: 0 on success, negative errno on MDIO/transport error.
 *         cfg->result carries the firmware-level return value.
 */
int xpcs_ext_eq_get(const GSW_Device_t *dev, struct xpcs_eq_get *cfg)
{
	return gsw_api_wrap(dev,
			    XPCS_EQ_GET,
			    cfg,
			    sizeof(*cfg),
			    0,
			    sizeof(*cfg));
}

/**
 * xpcs_ext_signal_detect - Read signal detect and link status flags
 * @dev: MDIO device context
 * @cfg: Signal detect request (in/out)
 *
 * Return: 0 on success, negative errno on MDIO/transport error.
 *         cfg->result carries the firmware-level return value.
 */
int xpcs_ext_signal_detect(const GSW_Device_t *dev,
			   struct xpcs_signal_detect *cfg)
{
	return gsw_api_wrap(dev,
			    XPCS_SIGNAL_DETECT,
			    cfg,
			    sizeof(*cfg),
			    0,
			    sizeof(*cfg));
}
