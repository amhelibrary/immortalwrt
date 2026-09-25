/******************************************************************************

   Copyright 2025 MaxLinear, Inc.

   For licensing information, see the file 'LICENSE' in the root folder of
   this software module.

******************************************************************************/

#ifndef _SYS_LED_H_
#define _SYS_LED_H_

/** \addtogroup DOC_SYS_MISC
 * @{
 */

#pragma pack(push, 1)
#pragma scalar_storage_order little-endian

/** \brief Data structure set/get system LED configuration.
 *  Used \ref mxl_led_sys_cfg_set and \ref mxl_led_sys_cfg_get.
 */
struct mxl_led_sys_cfg {
	/** 1 to enable System LED (config and drive GPIO pin to toggle LED).
	 *  0 to disable System LED (free GPIO pin with current state).
	 */
	uint8_t enable: 1;

	/** GPIO pin number used to drive System LED
	 *  (range 0~47).
	 */
	uint8_t gpio : 6;

	/** GPIO pin is active low (output low to turn on LED) */
	uint8_t active_low: 1;

	/** LED On time during system start.
	 *  Unit is 10 milliseconds.
	 *  If it's value 0 and start_delay_off is non-zero,
	 *  System LED is constant off.
	 */
	uint8_t start_delay_on;

	/** LED Off time during system start.
	 *  Unit is 10 milliseconds.
	 *  If it's value 0 and start_delay_on is non-zero,
	 *  System LED is constant on.
	 */
	uint8_t start_delay_off;

	/** LED On time when system is stable up.
	 *  Unit is 10 milliseconds.
	 *  If it's value 0 and stable_delay_off is non-zero,
	 *  System LED is constant off.
	 */
	uint8_t stable_delay_on;

	/** LED Off time when system is stable up.
	 *  Unit is 10 milliseconds.
	 *  If it's value 0 and stable_delay_on is non-zero,
	 *  System LED is constant on.
	 */
	uint8_t stable_delay_off;

	/** LED On time when system is in error state (such as loop detected)
	 *  Unit is 10 milliseconds.
	 *  If it's value 0 and error_delay_off is non-zero,
	 *  System LED is constant off.
	 */
	uint8_t error_delay_on;

	/** LED Off time when system is in error state (such as loop detected)
	 *  Unit is 10 milliseconds.
	 *  If it's value 0 and error_delay_on is non-zero,
	 *  System LED is constant on.
	 */
	uint8_t error_delay_off;
};

#pragma scalar_storage_order default
#pragma pack(pop)

/** \brief Configure system LED.
 *
 *  \param dev Pointer to GSW_Device_t (defined by application).
 *  \param parm Pointer to \ref mxl_led_sys_cfg.
 *
 *  \return Return value as follows:
 *  - 0: if successful
 *  - negative: error code
 */
int mxl_led_sys_cfg_set(const GSW_Device_t *dev, struct mxl_led_sys_cfg *parm);

/** \brief Get system LED configuration.
 *
 *  \param dev Pointer to GSW_Device_t (defined by application).
 *  \param parm Pointer to \ref mxl_led_sys_cfg.
 *
 *  \return Return value as follows:
 *  - 0: if successful
 *  - negative: error code
 */
int mxl_led_sys_cfg_get(const GSW_Device_t *dev, struct mxl_led_sys_cfg *parm);

/** @} */	/* DOC_SYS_MISC */

#endif /* _SYS_LED_H_ */
