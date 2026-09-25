/******************************************************************************

   Copyright 2026 MaxLinear, Inc.

   For licensing information, see the file 'LICENSE' in the root folder of
   this software module.

******************************************************************************/

#ifndef _XPCS_EXT_H_
#define _XPCS_EXT_H_

#include <stdint.h>

/* ================================================================
 * Data structures - packed, little-endian for MDIO transport
 * ================================================================ */

#pragma pack(push, 1)
#pragma scalar_storage_order little-endian

/**
 * \brief XPCS interface mode.
 *
 * These values are used in the \ref xpcs_pcs_cfg.interface field to select
 * the PCS/SERDES operating mode. They correspond to Linux phy_interface_t
 * values supported by the MXL862xx XPCS.
 */
enum xpcs_if_mode {
	XPCS_IF_SGMII        = 0,  /**< SGMII (CL37, MAC or PHY side) */
	XPCS_IF_1000BASEX    = 1,  /**< 1000BASE-X (CL37, 802.3z) */
	XPCS_IF_2500BASEX    = 2,  /**< 2500BASE-X (CL37 at 3.125 GBaud) */
	XPCS_IF_USXGMII      = 3,  /**< USXGMII (single/dual/quad) */
	XPCS_IF_10GBASER     = 4,  /**< 10GBASE-R (no AN) */
	XPCS_IF_10GKR        = 5,  /**< 10GBASE-KR (CL73 + CL72) */
	XPCS_IF_5GBASER      = 6,  /**< 5GBASE-R */
	XPCS_IF_QSGMII       = 7,  /**< QSGMII */
};

/**
 * \brief PCS negotiation mode.
 *
 * Matches Linux phylink PCS negotiation mode concept.
 */
enum xpcs_neg_mode {
	XPCS_NEG_NONE           = 0, /**< No inband negotiation (fixed link) */
	XPCS_NEG_INBAND_AN_OFF  = 1, /**< Inband selected but AN disabled */
	XPCS_NEG_INBAND_AN_ON   = 2, /**< Inband with AN enabled (normal) */
};

/**
 * \brief USXGMII lane mode.
 */
enum xpcs_usx_lane_mode {
	XPCS_USX_SINGLE = 0, /**< Single USXGMII lane */
	XPCS_USX_QUAD   = 1, /**< Quad USXGMII (4 ports per lane) */
};

/**
 * \brief PCS protocol role.
 *
 * Selects the role our XPCS plays in protocols that have an
 * asymmetric AN code word (Cisco SGMII / QSGMII / USXGMII).
 * The bit drives VR_MII_AN_CTRL.TX_CONFIG -- 0 means we receive
 * the partner's AN word, 1 means we source one. Ignored for
 * symmetric protocols (1000BX, 2500BX, 10GBASE-R/KR).
 */
enum xpcs_role {
	XPCS_ROLE_MAC = 0, /**< Local end is MAC side (TX_CONFIG=0) */
	XPCS_ROLE_PHY = 1, /**< Local end is PHY side (TX_CONFIG=1) */
};

/**
 * \brief PCS speed values.
 */
enum xpcs_speed {
	XPCS_SPEED_UNKNOWN = 0,
	XPCS_SPEED_10      = 10,
	XPCS_SPEED_100     = 100,
	XPCS_SPEED_1000    = 1000,
	XPCS_SPEED_2500    = 2500,
	XPCS_SPEED_5000    = 5000,
	XPCS_SPEED_10000   = 10000,
};

/**
 * \brief PCS duplex values.
 */
enum xpcs_duplex {
	XPCS_DUPLEX_HALF = 0,
	XPCS_DUPLEX_FULL = 1,
};

/**
 * \brief XPCS AN code word, tagged by the interface mode in use.
 *
 * Three protocol families on this XPCS exchange AN words of three
 * different sizes; the host picks the right member of this union
 * based on the @interface field of the surrounding struct (and,
 * for the asymmetric protocols, on @role):
 *
 *   .cl37  -- CL37 16-bit word (SR_MII_AN_ADV on write,
 *             SR_MII_LP_BABL on read).  Used by SGMII, QSGMII,
 *             1000BASE-X, 2500BASE-X.
 *   .usx   -- USXGMII 16-bit AN code word.  Linux MDIO_USXGMII_*
 *             layout: speed in bits 9-11 (000=10M, 001=100M,
 *             010=1G, 011=10G, 100=2.5G, 101=5G), FD in bit 12,
 *             LINK in bit 15, EEE in bits 7-8.
 *   .cl73  -- CL73 48-bit base page (10GBASE-KR).  Three 16-bit
 *             registers per 802.3 Annex 28C: SR_AN_ADV{1,2,3} on
 *             write, SR_AN_LP_ABL{1,2,3} on read.
 *
 * The raw[] view exists only to fix sizeof(union) at 6 bytes for
 * deterministic MMD transport layout regardless of host alignment.
 */
union xpcs_an_word {
	uint16_t cl37;          /**< CL37 16-bit word */
	uint16_t usx;           /**< USXGMII 16-bit AN word */
	struct {
		uint16_t adv1;  /**< CL73 SR_AN_ADV1 / SR_AN_LP_ABL1 */
		uint16_t adv2;  /**< CL73 SR_AN_ADV2 / SR_AN_LP_ABL2 */
		uint16_t adv3;  /**< CL73 SR_AN_ADV3 / SR_AN_LP_ABL3 */
	} cl73;
	uint8_t  raw[6];        /**< Byte view, forces sizeof = 6 */
};

/* ----------------------------------------------------------------
 * PCS Config - Configure PCS mode and advertisement
 *
 * Configures PCS/SERDES operating mode and auto-negotiation.
 * The host (DSA driver) is responsible for calling pcs_enable before
 * pcs_config if the SERDES was previously disabled.
 * ---------------------------------------------------------------- */

/**
 * \brief PCS configuration parameters (6 bytes, bitfield-packed).
 */
struct xpcs_pcs_cfg {
	/* Byte 0 */
	uint8_t  port_id      : 2; /**< [in] XPCS port index (0-3) */
	uint8_t  interface    : 6; /**< [in] \ref xpcs_if_mode */
	/* Byte 1 */
	uint8_t  neg_mode     : 2; /**< [in] \ref xpcs_neg_mode (0-2) */
	uint8_t  permit_pause : 1; /**< [in] Allow pause to MAC (0/1) */
	uint8_t  usx_lane_mode: 2; /**< [in] \ref xpcs_usx_lane_mode */
	uint8_t  role         : 1; /**< [in] \ref xpcs_role */
	uint8_t  usx_subport  : 2; /**< [in] Sub-port (0-3) within the XPCS */
	/** [in] AN code word the local end transmits.  The active
	 *  union member is selected by @interface (and, for the
	 *  asymmetric protocols, by @role):
	 *
	 *    - 1000BASE-X / 2500BASE-X        -> .cl37 (802.3 Cl37
	 *      base page: FD, HD, PAUSE, ASYM_PAUSE, RF, NP)
	 *    - SGMII / QSGMII with role=PHY   -> .cl37 (Cisco SGMII
	 *      config word; written per-lane for QSGMII)
	 *    - USXGMII with role=PHY          -> .usx  (Linux
	 *      MDIO_USXGMII_* layout; written per-lane for QUSXGMII)
	 *    - 10GBASE-KR (CL73)              -> .cl73 (48-bit base
	 *      page across SR_AN_ADV{1,2,3})
	 *
	 *  Ignored when the local end does not transmit an AN word
	 *  (role=MAC for SGMII/QSGMII/USXGMII, 10GBASE-R, 5GBASE-R)
	 *  or when neg_mode != INBAND_AN_ON.  Pass all-zero to keep
	 *  the firmware's default advertisement.
	 */
	union xpcs_an_word advertising;
	/** [out] Result:
	 *    >0 host must follow with XPCS_AN_RESTART
	 *     0 no host follow-up needed
	 *    <0 errno
	 *
	 *  result==1 is only returned when the local end actually sources
	 *  an AN code word the partner needs to see:
	 *    - INBAND_AN_ON with role=PHY for SGMII/QSGMII/USXGMII (the
	 *      asymmetric, MAC-vs-PHY protocols), or
	 *    - INBAND_AN_ON for 1000BASE-X/2500BASE-X/10GBASE-KR (the
	 *      symmetric AN protocols where both ends transmit).
	 *  MAC-side asymmetric and the no-AN modes return 0.
	 */
	int16_t  result;
};

/* ----------------------------------------------------------------
 * PCS Get State - Read resolved PCS link state
 *
 * Returns the current link, speed, duplex, AN status, and pause
 * negotiation result from the PCS hardware registers.
 * ---------------------------------------------------------------- */

/**
 * \brief PCS link state output (8 bytes, bitfield-packed).
 */
struct xpcs_pcs_state {
	/* Byte 0: inputs */
	uint8_t  port_id      : 2; /**< [in] XPCS port index (0-3) */
	uint8_t  interface    : 6; /**< [in] \ref xpcs_if_mode */
	/* Byte 1: inputs + outputs */
	uint8_t  usx_lane_mode: 2; /**< [in] USX lane mode */
	uint8_t  usx_subport  : 2; /**< [in] USX sub-port (0-3) */
	uint8_t  link         : 1; /**< [out] Link up (1) / down (0) */
	uint8_t  an_complete  : 1; /**< [out] AN complete */
	uint8_t  duplex       : 1; /**< [out] \ref xpcs_duplex */
	uint8_t  pcs_fault    : 1; /**< [out] PCS fault */
	/* Byte 2: outputs */
	uint8_t  pause        : 2; /**< [out] Pause (b0=sym, b1=asym) */
	uint8_t  lp_eee_cap   : 1; /**< [out] LP supports EEE */
	uint8_t  lp_eee_cs_cap: 1; /**< [out] LP supports EEE clock-stop */
	uint8_t  _rsv         : 4;
	uint8_t  _pad;
	/** [out] Speed (\ref xpcs_speed) */
	uint16_t speed;
	/** [out] Link Partner ability word.  Same union as
	 *  \ref xpcs_pcs_cfg::advertising; the host picks the
	 *  member based on @interface:
	 *    - SGMII / QSGMII / 1000BX / 2500BX -> .cl37
	 *      (SR_MII_LP_BABL, lane 0 or @usx_subport for QSGMII)
	 *    - USXGMII                          -> .usx
	 *      (synthesised from VR_MII_AN_INTR_STS USXG fields)
	 *    - 10GBASE-KR                       -> .cl73
	 *      (SR_AN_LP_ABL{1,2,3}; CL73 48-bit base page)
	 */
	union xpcs_an_word lpa;
};

/* ----------------------------------------------------------------
 * PCS Disable - SERDES power down
 *
 * Asserts IDDQ + PHY + XPCS resets to power down the SerDes when
 * the port is admin-down or no module is plugged in.  The next
 * PCS_CONFIG implicitly powers it back up (bringup_*_mode()
 * deasserts the resets) and reprograms the desired interface.
 * ---------------------------------------------------------------- */

/**
 * \brief PCS disable parameters.
 */
struct xpcs_pcs_disable {
	/** [in] XPCS port index (0 = XPCS0, 1 = XPCS1) */
	uint8_t  port_id;
	uint8_t  _pad;
	/** [out] Result: 0 = success, <0 = error */
	int16_t  result;
};

/* ----------------------------------------------------------------
 * Phase 2: AN Restart - Restart auto-negotiation
 *
 * Restarts CL37 (SGMII/1000BASE-X/2500BASE-X/QSGMII/USXGMII) or
 * CL73 (10GBASE-KR) auto-negotiation on the given XPCS port.
 * The SERDES must already be configured via PCS_CONFIG.
 * ---------------------------------------------------------------- */

/**
 * \brief AN restart parameters (4 bytes, bitfield-packed).
 */
struct xpcs_an_restart {
	uint8_t  port_id      : 2; /**< [in] XPCS port index (0-3) */
	uint8_t  interface    : 6; /**< [in] \ref xpcs_if_mode */
	uint8_t  usx_lane_mode: 2; /**< [in] USX lane mode */
	uint8_t  usx_subport  : 2; /**< [in] Sub-port (0-3) within the XPCS */
	uint8_t  _rsv         : 4;
	/** [out] Result: 0 = success, <0 = error */
	int16_t  result;
};

/* ----------------------------------------------------------------
 * PCS Link Up - notify XPCS that the line side has come up
 *
 * Called once per link-up event after the host has resolved the
 * line-side speed/duplex (from the PHY's read_status, from a
 * preceding PCS_GET_STATE, or from a fixed-link description).
 *
 * Per-mode behaviour:
 *   NEG_INBAND_AN_ON, role=MAC (SGMII/QSGMII/USXGMII): silent
 *     no-op -- the XPCS auto-resolves from the partner's AN word
 *     and the host should be reading state, not forcing.
 *   NEG_INBAND_AN_ON, role=PHY (SGMII/QSGMII/USXGMII): updates
 *     the values we advertise in the outgoing AN word and triggers
 *     RESTART_AN so the partner re-runs AN against the new speed.
 *   NEG_INBAND_AN_OFF / NEG_NONE (any interface): writes
 *     SR_MII_CTRL.SS5/SS6/SS13 + DUPLEX_MODE directly to drive the
 *     XPCS at the requested speed/duplex without involving CL37 AN.
 *   1000BASE-X / 2500BASE-X with NEG_INBAND_AN_ON: silent no-op --
 *     speed is fixed by the interface mode, duplex by 802.3z AN
 *     resolution.  Callers may invoke without harm.
 *   10GBASE-R / 5GBASE-R / 10GBASE-KR: silent no-op.
 * ---------------------------------------------------------------- */

/**
 * \brief PCS link-up parameters (6 bytes, bitfield-packed).
 */
struct xpcs_pcs_link_up {
	uint8_t  port_id      : 2; /**< [in] XPCS port index (0-3) */
	uint8_t  interface    : 6; /**< [in] \ref xpcs_if_mode */
	uint8_t  duplex       : 1; /**< [in] \ref xpcs_duplex */
	uint8_t  usx_lane_mode: 2; /**< [in] \ref xpcs_usx_lane_mode
				    *  (USXGMII only; ignored otherwise) */
	uint8_t  usx_subport  : 2; /**< [in] USX sub-port (0-3,
				    *  QUSXGMII only; ignored otherwise) */
	uint8_t  _rsv0        : 3;
	uint16_t speed;            /**< [in] \ref xpcs_speed */
	int16_t  result;           /**< [out] 0 = success, <0 = errno */
};

/* ----------------------------------------------------------------
 * Phase 3: Loopback - PCS/PMA loopback control
 *
 * Sets or clears loopback on the XPCS or PMA.
 * AN is disabled before enabling loopback.
 * ---------------------------------------------------------------- */

/**
 * \brief Loopback mode.
 */
enum xpcs_loopback_mode {
	XPCS_LB_DISABLE      = 0, /**< Disable all loopback */
	XPCS_LB_PCS_SERIAL   = 1, /**< PCS TX-to-RX serial (internal) */
	XPCS_LB_PCS_PARALLEL = 2, /**< PCS RX-to-TX parallel (external) */
	XPCS_LB_PMA_SERIAL   = 3, /**< PMA TX-to-RX serial (analog) */
	XPCS_LB_PMA_PARALLEL = 4, /**< PMA RX-to-TX parallel */
};

/**
 * \brief Loopback control parameters.
 */
struct xpcs_loopback_cfg {
	/** [in] XPCS port index (0 = XPCS0, 1 = XPCS1) */
	uint8_t  port_id;
	/** [in] Loopback mode (\ref xpcs_loopback_mode) */
	uint8_t  mode;
	/** [out] Result: 0 = success, <0 = error */
	int16_t  result;
};

/* ----------------------------------------------------------------
 * Phase 3: Reset - XPCS soft/hard reset
 *
 * Performs a reset on the XPCS without a full power cycle.
 * VR_RESET: vendor-specific reset (fast, preserves config).
 * SOFT_RESET: PCS-level reset (SR_XS_PCS_CTRL1 reset bit).
 * HARD_RESET: full XPCS reset via RCU (IDDQ deassert sequence).
 * ---------------------------------------------------------------- */

/**
 * \brief Reset type.
 */
enum xpcs_reset_type {
	XPCS_RESET_VR   = 0, /**< Vendor-specific reset */
	XPCS_RESET_SOFT = 1, /**< PCS soft reset */
	XPCS_RESET_HARD = 2, /**< Full hardware reset (RCU) */
};

/**
 * \brief Reset parameters.
 */
struct xpcs_reset_cfg {
	/** [in] XPCS port index (0 = XPCS0, 1 = XPCS1) */
	uint8_t  port_id;
	/** [in] Reset type (\ref xpcs_reset_type) */
	uint8_t  reset_type;
	/** [out] Result: 0 = success, <0 = error */
	int16_t  result;
};

/* ----------------------------------------------------------------
 * Phase 4: PRBS Config - PCS-level PRBS31 test pattern control
 *
 * Enables or disables the PRBS31 test pattern generator (TX) and
 * checker (RX) in the PCS layer (SR_XS_PCS_TP_CTRL register).
 * When the checker is enabled, the RX error counter can be read.
 * ---------------------------------------------------------------- */

/**
 * \brief PRBS31 test pattern configuration (6 bytes, bitfield-packed).
 */
struct xpcs_prbs_cfg {
	uint8_t  port_id  : 2; /**< [in] XPCS port index (0-3) */
	uint8_t  tx_en    : 1; /**< [in] TX PRBS31 enable (0/1) */
	uint8_t  rx_en    : 1; /**< [in] RX PRBS31 enable (0/1) */
	uint8_t  read_err : 1; /**< [in] Read error count (0/1) */
	uint8_t  _rsv     : 3;
	uint8_t  _pad;
	/** [out] RX PRBS31 error count (valid when read_err=1 or rx_en=1) */
	uint16_t rx_err_cnt;
	/** [out] Result: 0 = success, <0 = error */
	int16_t  result;
};

/* ----------------------------------------------------------------
 * Phase 4: BERT Config - SerDes-level Bit Error Rate Test control
 *
 * Enables or disables the SerDes hardware BERT pattern generator
 * (TX) and checker (RX). The BERT operates at the analog lane level,
 * below the PCS, using the Synopsys LBERT registers.
 * PRBS pattern types:
 *   1 = PRBS7, 2 = PRBS9, 3 = PRBS11, 4 = PRBS15, 5 = PRBS23,
 *   6 = PRBS31, 7 = PRBS13
 * ---------------------------------------------------------------- */

/**
 * \brief BERT configuration (6 bytes, bitfield-packed).
 */
struct xpcs_bert_cfg {
	/* Byte 0: control flags */
	uint8_t  port_id    : 2; /**< [in] XPCS port index (0-3) */
	uint8_t  tx_en      : 1; /**< [in] TX BERT enable (0/1) */
	uint8_t  rx_en      : 1; /**< [in] RX BERT enable (0/1) */
	uint8_t  read_err   : 1; /**< [in] Read RX error count */
	uint8_t  clear_err  : 1; /**< [in] Clear RX error counter */
	uint8_t  insert_err : 1; /**< [in] Insert one TX error */
	uint8_t  _rsv       : 1;
	/** [in] PRBS pattern type (1-7, see above). 0 = disable. */
	uint8_t  pattern;
	/** [out] RX BERT error count (valid when read_err=1) */
	uint16_t rx_err_cnt;
	/** [out] Result: 0 = success, <0 = error */
	int16_t  result;
};

/* ----------------------------------------------------------------
 * Phase 4: EQ Get - Read TX/RX equalization status
 *
 * Reads the current TX and RX analog equalization parameters
 * from the SerDes. Returns both initial (ASIC IN) and override
 * values with their enable flags.
 * ---------------------------------------------------------------- */

/**
 * \brief Equalization item (initial value + override).
 */
struct xpcs_eq_item {
	uint8_t  value;    /**< Current initial value */
	uint8_t  ovrd;     /**< Override value */
	uint8_t  ovrd_en;  /**< Override enable flag (1 = active) */
};

/**
 * \brief TX equalization status.
 */
struct xpcs_tx_eq_info {
	struct xpcs_eq_item main;       /**< TX main cursor (0-63) */
	struct xpcs_eq_item pre;        /**< TX pre-cursor (0-63) */
	struct xpcs_eq_item post;       /**< TX post-cursor (0-63) */
	struct xpcs_eq_item iboost_lvl; /**< TX iboost level (0-15) */
	struct xpcs_eq_item vboost_lvl; /**< TX vboost level (0-7) */
	struct xpcs_eq_item vboost_en;  /**< TX vboost enable (0-1) */
};

/**
 * \brief RX equalization status.
 */
struct xpcs_rx_eq_info {
	struct xpcs_eq_item att_lvl;    /**< RX attenuation level (0-7) */
	struct xpcs_eq_item vga1_gain;  /**< RX VGA1 gain (0-7) */
	struct xpcs_eq_item vga2_gain;  /**< RX VGA2 gain (0-7) */
	struct xpcs_eq_item ctle_boost; /**< RX CTLE boost (0-31) */
	struct xpcs_eq_item ctle_pole;  /**< RX CTLE pole (0-3) */
	struct xpcs_eq_item dfe_tap1;   /**< RX DFE tap1 (0-255) */
	struct xpcs_eq_item dfe_bypass; /**< RX DFE bypass (0-1) */
	struct xpcs_eq_item adapt_mode; /**< RX adapt mode (0-3) */
	struct xpcs_eq_item adapt_sel;  /**< RX adapt select (0-1) */
};

/**
 * \brief EQ get request / response.
 */
struct xpcs_eq_get {
	/** [in] XPCS port index (0 = XPCS0, 1 = XPCS1) */
	uint8_t  port_id;
	/** [out] Result: 0 = success, <0 = error */
	int16_t  result;
	/** [out] TX equalization info */
	struct xpcs_tx_eq_info tx;
	/** [out] RX equalization info */
	struct xpcs_rx_eq_info rx;
};

/* ----------------------------------------------------------------
 * Phase 4: Signal Detect - Read signal/link status flags
 *
 * Reads low-level PMA/PCS status indicators: RX signal detect,
 * PMA link up, PCS link fault, and XPCS reset state.
 * ---------------------------------------------------------------- */

/**
 * \brief Signal detect status (4 bytes, bitfield-packed).
 */
struct xpcs_signal_detect {
	uint8_t  port_id    : 2; /**< [in] XPCS port index (0-3) */
	uint8_t  rx_signal  : 1; /**< [out] RX signal detected */
	uint8_t  pma_link   : 1; /**< [out] PMA link up */
	uint8_t  link_fault : 1; /**< [out] PCS link fault */
	uint8_t  in_reset   : 1; /**< [out] XPCS in reset */
	uint8_t  _rsv       : 2;
	uint8_t  _pad;
	/** [out] Result: 0 = success, <0 = error */
	int16_t  result;
};

/* Link partner individual channel capability and state */
struct xpcs_lpa_chnl_sts {
	uint8_t cl37_an_cmplt;	/* AN completed or not or disabled. 1: completed, 0: not completed or AN is disabled.
	                         *    only when this cl37_an_cmplt is 1, all the below fields are valid, otherwise ignore all below fields. */
	uint8_t link_sts;	/* LP Link State, 0: Link Down, 1: Link Up */
	uint16_t link_spd;	/* LP Link Speed, ref to xpcs_speed */
	uint8_t duplex;		/* LP Link Duplex. 0: Half Duplex, 1: Full Duplex*/
	uint8_t eee_cap;	/* LP EEE Capability. 0: Not Support, 1: Support */
	uint8_t eee_cs_cap;	/* LP EEE Clock-Stop Capability. 0: Not Support, 1: Support */
	uint8_t rsv;
};

/* Link partner capability and state */
struct xpcs_lpa_sts {
	uint8_t port_id;	/* [in] local XPCS port index XPCS0 or XPCS1*/
	uint8_t chnl_id;	/* [in] specify the channel to be get the status,
		                 *	0 for single channel mode or first channel to multiple (dual/quad) channels mode
		                 *	0 or 1 for multiple (dual/quad) channel mode
		                 *	0 or 1 or 2 or 3 for quad channel mode */
	struct xpcs_lpa_chnl_sts chnl_sts;	/* [out] Channel status for the specified channel (chnl_id). ref to lpa_chnl_sts
		                                 *       If chnl_id is out of the range, for example chnl_id == 2 for single channel mode
		                                 *            chnl_sts all fields will be 0.
		                                 */
};

#pragma scalar_storage_order default
#pragma pack(pop)

/* ================================================================
 * Handler function declarations
 * ================================================================ */

/**
 * \brief Configure PCS mode and auto-negotiation.
 * \param[in] dev Unused (NULL)
 * \param[in,out] cfg PCS configuration (\ref xpcs_pcs_cfg)
 * \return 0 on success, negative errno on error
 */
int xpcs_ext_pcs_config(const GSW_Device_t *dev, struct xpcs_pcs_cfg *cfg);

/**
 * \brief Read resolved PCS link state.
 * \param[in] dev Unused (NULL)
 * \param[in,out] state PCS state (\ref xpcs_pcs_state)
 * \return 0 on success, negative errno on error
 */
int xpcs_ext_pcs_get_state(const GSW_Device_t *dev, struct xpcs_pcs_state *state);

/**
 * \brief Disable (power down) SERDES.
 * \param[in] dev Unused (NULL)
 * \param[in,out] cfg Disable params (\ref xpcs_pcs_disable)
 * \return 0 on success, negative errno on error
 */
int xpcs_ext_pcs_disable(const GSW_Device_t *dev, struct xpcs_pcs_disable *cfg);

/* Phase 2: AN Control */

/**
 * \brief Restart auto-negotiation (CL37 or CL73).
 * \param[in] dev Unused (NULL)
 * \param[in,out] cfg AN restart config (\ref xpcs_an_restart)
 * \return 0 on success, negative errno on error
 */
int xpcs_ext_an_restart(const GSW_Device_t *dev, struct xpcs_an_restart *cfg);

/* Phase 3: Force/Utility */

/**
 * \brief Notify XPCS of a link-up event with resolved speed/duplex.
 * \param[in] dev Unused (NULL)
 * \param[in,out] cfg Link-up parameters (\ref xpcs_pcs_link_up)
 * \return 0 on success, negative errno on error
 */
int xpcs_ext_pcs_link_up(const GSW_Device_t *dev, struct xpcs_pcs_link_up *cfg);

/**
 * \brief Set/clear PCS or PMA loopback.
 * \param[in] dev Unused (NULL)
 * \param[in,out] cfg Loopback config (\ref xpcs_loopback_cfg)
 * \return 0 on success, negative errno on error
 */
int xpcs_ext_loopback(const GSW_Device_t *dev, struct xpcs_loopback_cfg *cfg);

/**
 * \brief Reset the XPCS (VR, soft, or hard).
 * \param[in] dev Unused (NULL)
 * \param[in,out] cfg Reset config (\ref xpcs_reset_cfg)
 * \return 0 on success, negative errno on error
 */
int xpcs_ext_reset(const GSW_Device_t *dev, struct xpcs_reset_cfg *cfg);

/* Phase 4: Diagnostics */

/**
 * \brief Configure PCS-level PRBS31 test pattern (TX/RX) and read errors.
 * \param[in] dev Unused (NULL)
 * \param[in,out] cfg PRBS config (\ref xpcs_prbs_cfg)
 * \return 0 on success, negative errno on error
 */
int xpcs_ext_prbs_cfg(const GSW_Device_t *dev, struct xpcs_prbs_cfg *cfg);

/**
 * \brief Configure SerDes-level BERT pattern (TX/RX) and read errors.
 * \param[in] dev Unused (NULL)
 * \param[in,out] cfg BERT config (\ref xpcs_bert_cfg)
 * \return 0 on success, negative errno on error
 */
int xpcs_ext_bert_cfg(const GSW_Device_t *dev, struct xpcs_bert_cfg *cfg);

/**
 * \brief Read TX and RX equalization status.
 * \param[in] dev Unused (NULL)
 * \param[in,out] cfg EQ get request (\ref xpcs_eq_get)
 * \return 0 on success, negative errno on error
 */
int xpcs_ext_eq_get(const GSW_Device_t *dev, struct xpcs_eq_get *cfg);

/**
 * \brief Read low-level signal detect and link status flags.
 * \param[in] dev Unused (NULL)
 * \param[in,out] cfg Signal detect request (\ref xpcs_signal_detect)
 * \return 0 on success, negative errno on error
 */
int xpcs_ext_signal_detect(const GSW_Device_t *dev, struct xpcs_signal_detect *cfg);

/**
 * \brief Get link partner link status
 * \param[in] dev Unused (NULL)
 * \param[in, out] p_lp_sts link partner status
 * \return 0 on success, negative errno on error
 **/
int xpcs_ext_lp_link_sts_get(const GSW_Device_t *dev, struct xpcs_lpa_sts *p_lp_sts);

#endif /* _XPCS_EXT_H_ */
