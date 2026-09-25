/******************************************************************************

   Copyright 2023-2024 MaxLinear, Inc.

   For licensing information, see the file 'LICENSE' in the root folder of
   this software module.

******************************************************************************/

#ifndef _LOOP_DETECT_CFG_H_
#define _LOOP_DETECT_CFG_H_

/** \addtogroup DOC_SYS_MISC
 * @{
 */

#pragma pack(push, 1)
#pragma scalar_storage_order little-endian

/** @cond INTERNAL */
/** \brief Passive loop detection config data structure.
 *  Used by \ref loop_detect_passive_start.
 */
struct loop_detect_passive_cfg {
	/** Port is in loop state if MAC violation counter between interval
	 *  is equal to or larger than this threshold.
	 *  Range 1~255.
	 *  Value 0 to maintain current value.
	 */
	uint8_t threshold;
	/** Polling interval in 100 milliseconds.
	 *  Range 1~100 (0.1s ~ 10s).
	 *  Value 0 to maintain current value.
	 */
	uint8_t interval;
};
/** @endcond INTERNAL */

/** \brief Active loop detection config data structure.
 *  Used by \ref loop_detect_active_start and \ref loop_detect_active_cfg_get.
 */
struct loop_detect_active_cfg {
	/** Port is not in loop state if test packet is not received within
	 *  this number of transmission cycles.
	 *  Range 1~100.
	 *  Value 0 to maintain current value.
	 */
	uint8_t refresh;
	/** Interval in 100 milliseconds to send test packet.
	 *  Range 1~100 (0.1s ~ 10s).
	 *  Value 0 to maintain current value.
	 */
	uint8_t interval;
};

/** \brief Loop detect cascade configuration data structure.
 *  Configures cascade port topology and loop blocking strategy.
 *  Used by \ref loop_detect_cascade_cfg_set and \ref loop_detect_cascade_cfg_get.
 */
struct loop_detect_cascade_cfg {
	/** Cascade port bitmap (physical port ID / CTP ID).
	 *  Bit (N & 0x0F) set means CTP N is a cascade port, always exempt
	 *  from blocking. CTP 1~15 maps to bit 1~15, CTP 16 maps to bit 0.
	 *  e.g. CTP 9 = BIT(9), CTP 16 = BIT(0).
	 *  Value 0 means no cascade port is configured.
	 */
	uint32_t cascade_portmap			: 16;
	/** Blocking strategy control.
	 *  - 0: block all non-cascade ports found in loop.
	 *  - 1: block all except the one with highest App Port ID.
	 */
	uint32_t block_mode				: 1;
	/** Reserved field. Must be initialized to 0. */
	uint32_t rev					: 15;
};

/** \brief Loop prevention config data structure.
 *  Used by \ref loop_prevention_start.
 */
struct loop_prevention_cfg {
	/** Reserved field. Must be initialized to 0. */
	uint8_t res;
};

#pragma scalar_storage_order default
#pragma pack(pop)

/** @cond INTERNAL */
/** \brief Start passive loop detection.
 *
 *  \param dev Pointer to GSW_Device_t (defined by application).
 *  \param pcfg Pointer to \ref loop_detect_passive_cfg.
 *
 *  \return Return value as follows:
 *  - 0: if successful
 *  - negative: error code
 */
/* Default setting is applied if pcfg is NULL.
 * This should be called in thread context only.
 * Parameter dev is dummy (NULL) in Zephyr context.
 */
int loop_detect_passive_start(const GSW_Device_t *dev,
			      struct loop_detect_passive_cfg *pcfg);

/** \brief Stop passive loop detection.
 *
 *  \param dev Pointer to GSW_Device_t (defined by application).
 *
 *  \return Return value as follows:
 *  - 0: if successful
 *  - negative: error code
 */
/* This should be called in thread context only.
 * Parameter dev is dummy (NULL) in Zephyr context.
 */
int loop_detect_passive_stop(const GSW_Device_t *dev);
/** @endcond INTERNAL */

/** \brief Start active loop detection.
 *
 *  \param dev Pointer to GSW_Device_t (defined by application).
 *  \param pcfg Pointer to \ref loop_detect_active_cfg.
 *
 *  \return Return value as follows:
 *  - 0: if successful
 *  - negative: error code
 */
/* Default setting is applied if pcfg is NULL.
 * This should be called in thread context only.
 * Parameter dev is dummy (NULL) in Zephyr context.
 */
int loop_detect_active_start(const GSW_Device_t *dev,
			     struct loop_detect_active_cfg *pcfg);

/** \brief Stop active loop detection.
 *
 *  \param dev Pointer to GSW_Device_t (defined by application).
 *
 *  \return Return value as follows:
 *  - 0: if successful
 *  - negative: error code
 */
/* This should be called in thread context only.
 * Parameter dev is dummy (NULL) in Zephyr context.
 */
int loop_detect_active_stop(const GSW_Device_t *dev);

/** \brief Get the active loop detecting configuration.
 *
 *  \param dev Pointer to GSW_Device_t (defined by application).
 *  \param pcfg Pointer to \ref loop_detect_active_cfg. Should not be NULL.
 *
 *  \remarks pcfg is output, should not be NULL.
 *           Only get the detect interval and refresh time.
 *
 *  \return Return value as follows:
 *  - 0: if successful
 *  - negative: error code
 */
/* Parameter dev is dummy (NULL) in Zephyr context. */
int loop_detect_active_cfg_get(const GSW_Device_t *dev,
			       struct loop_detect_active_cfg *pcfg);

/** \brief Start loop prevention.
 *
 *  \param dev Pointer to GSW_Device_t (defined by application).
 *  \param pcfg Pointer to \ref loop_prevention_cfg.
 *
 *  \return Return value as follows:
 *  - 0: if successful
 *  - negative: error code
 */
/* Default setting is applied if pcfg is NULL.
 * This should be called in thread context only.
 * Parameter dev is dummy (NULL) in Zephyr context.
 */
int loop_prevention_start(const GSW_Device_t *dev,
			  struct loop_prevention_cfg *pcfg);

/** \brief Stop loop prevention.
 *
 *  \param dev Pointer to GSW_Device_t (defined by application).
 *
 *  \return Return value as follows:
 *  - 0: if successful
 *  - negative: error code
 */
/* This should be called in thread context only.
 * Parameter dev is dummy (NULL) in Zephyr context.
 */
int loop_prevention_stop(const GSW_Device_t *dev);

/** \brief Set destination MAC of loop detection packet.
 *
 *  \param dev Pointer to GSW_Device_t (defined by application).
 *  \param mac 6-byte array with destination MAC address.
 *
 *  \return Return value as follows:
 *  - 0: if successful
 *  - negative: error code
 */
/* Can be called in both thread context and interrupt context.
 * Parameter dev is dummy (NULL) in Zephyr context.
 */
int loop_detect_active_mac_set(const GSW_Device_t *dev, uint8_t mac[6]);

/** \brief Get destination MAC of loop detection packet.
 *
 *  \param dev Pointer to GSW_Device_t (defined by application).
 *  \param mac 6-byte array with destination MAC address.
 *
 *  \return Return value as follows:
 *  - 0: if successful
 *  - negative: error code
 */
/* Can be called in both thread context and interrupt context.
 * Parameter dev is dummy (NULL) in Zephyr context.
 */
int loop_detect_active_mac_get(const GSW_Device_t *dev, uint8_t mac[6]);

/** \brief Set loop detect cascade configuration.
 *
 *  \param dummy Pointer to GSW_Device_t (defined by application).
 *  \param pcfg Pointer to \ref loop_detect_cascade_cfg.
 *
 *  \return Return value as follows:
 *  - 0: if successful
 *  - negative: error code
 */
/* This should be called in thread context only.
 * Parameter dummy is dummy (NULL) in Zephyr context.
 */

int loop_detect_cascade_cfg_set(const GSW_Device_t *dummy,
				struct loop_detect_cascade_cfg *pcfg);

/** \brief Get loop detect cascade configuration.
 *
 *  \param dummy Pointer to GSW_Device_t (defined by application).
 *  \param pcfg Pointer to \ref loop_detect_cascade_cfg. Should not be NULL.
 *
 *  \remarks pcfg is output, should not be NULL.
 *
 *  \return Return value as follows:
 *  - 0: if successful
 *  - negative: error code
 */
/* Parameter dummy is dummy (NULL) in Zephyr context. */
int loop_detect_cascade_cfg_get(const GSW_Device_t *dummy,
				struct loop_detect_cascade_cfg *pcfg);
/** @} */	/* DOC_SYS_MISC */

#endif /* _LOOP_DETECT_CFG_H_ */
