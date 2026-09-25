/******************************************************************************

   Copyright 2025 MaxLinear, Inc.

   For licensing information, see the file 'LICENSE' in the root folder of
   this software module.

******************************************************************************/

#ifndef _MDIO_RELAY_H_
#define _MDIO_RELAY_H_

/** \addtogroup DOC_SYS_MISC
 * @{
 */

#pragma pack(push, 1)
#pragma scalar_storage_order little-endian

/** \brief Data structure to help host to read/write internal PHY registers or
 *  external PHY registers via embedded MDIO controller.
 *  Used by \ref int_gphy_read, \ref int_gphy_write,
 *  \ref ext_mdio_read and \ref ext_mdio_write.
 */
struct mdio_relay_data {
	/** data to be read or written */
	uint16_t data;
	/** PHY index (0~7) for internal PHY.
	 *  PHY address (0~31) for external PHY access via MDIO bus.
	 */
	uint8_t phy;
	/** MMD device (0~31) */
	uint8_t mmd;
	/** Register Index:
	 *  0~31 if mmd is 0 (CL22).
	 *  0~65535 otherwise (CL45).
	 */
	uint16_t reg;
};

/** \brief Data structure to help host to modify internal PHY registers or
 *  external PHY registers via embedded MDIO controller.
 *  Used by \ref int_gphy_mod and \ref ext_mdio_mod.
 */
struct mdio_relay_mod_data {
	/** data to be written with mask */
	uint16_t data;
	/** PHY index (0~7) for internal PHY.
	 *  PHY address (0~31) for external PHY access via MDIO bus.
	 */
	uint8_t phy;
	/** MMD device (0~31) */
	uint8_t mmd;
	/** Register Index:
	 *  0~31 if mmd is 0 (CL22).
	 *  0~65535 otherwise (CL45).
	 */
	uint16_t reg;
	/** mask of bit fields to be updated:
	 *  1 to write the bit.
	 *  0 to ignore.
	 */
	uint16_t mask;
};

/** \brief Data structure input PHY list and register list.
 *  Used by \ref mdio_bulk_relay_data.
 */
struct mdio_bulk_relay_input {
	/** List of PHY to read (not more than 8):
	 *    For internal PHY, each bit (0~7) represents one internal PHY.
	 *    For external PHY, each bit (0~31) represents one external PHY address.
	 */
	uint32_t phy_map;
	/** MMD device (0~31) */
	uint8_t mmd[10];
	/** Register Index:
	 * 0~31 if mmd is 0 (CL22).
	 * 0~65535 otherwise (CL45).
	 */
	uint16_t reg[10];
	/** Number of register to read for each PHY. */
	uint8_t num;
};

/** \brief Data structure to help host to read/write internal PHY registers or
 *  external PHY registers via embedded MDIO controller. This is to enhance
 *  efficiency by reading multiple registers of multiple PHYs.
 *  Used by \ref int_gphy_bulk_read.
 */
union mdio_bulk_relay_data {
	/** Input data */
	struct mdio_bulk_relay_input input;
	/** Output data listed in following sequence:
	 *    data_idx = phy_idx * (1 + reg_num) + (1 + reg_idx).
	 *    data[phy_idx * (1 + reg_num)] is reserved for return status.
	 *    Each bit is one data:
	 *      0 - valid register value.
	 *      1 - signed 16-bit return value.
	 */
	uint16_t data[88];
};
#pragma scalar_storage_order default
#pragma pack(pop)

/** \brief Read internal GPHY MDIO/MMD registers.
 *
 *  \param dev Pointer to GSW_Device_t (defined by application).
 *  \param pdata Pointer to \ref mdio_relay_data.
 *
 *  \return Return value as follows:
 *  - 0: if successful
 *  - negative: error code
 */
int int_gphy_read(const GSW_Device_t *dev, struct mdio_relay_data *pdata);

/** \brief Write internal GPHY MDIO/MMD registers.
 *
 *  \param dev Pointer to GSW_Device_t (defined by application).
 *  \param pdata Pointer to \ref mdio_relay_data.
 *
 *  \return Return value as follows:
 *  - 0: if successful
 *  - negative: error code
 */
int int_gphy_write(const GSW_Device_t *dev, struct mdio_relay_data *pdata);

/** \brief Modify internal GPHY MDIO/MMD registers.
 *
 *  \param dev Pointer to GSW_Device_t (defined by application).
 *  \param pdata Pointer to \ref mdio_relay_mod_data.
 *
 *  \return Return value as follows:
 *  - 0: if successful
 *  - negative: error code
 */
int int_gphy_mod(const GSW_Device_t *dev, struct mdio_relay_mod_data *pdata);

/** \brief Read multiple MDIO/MMD registers of multiple internal GPHY.
 *
 *  \param dev Pointer to GSW_Device_t (defined by application).
 *  \param pdata Pointer to \ref mdio_bulk_relay_data.
 *
 *  \return Return value as follows:
 *  - 0: if successful
 *  - negative: error code
 */
int int_gphy_bulk_read(const GSW_Device_t *dev,
		       union mdio_bulk_relay_data *pdata);

/** \brief Read external GPHY MDIO/MMD registers via embedded MDIO controller.
 *
 *  \param dev Pointer to GSW_Device_t (defined by application).
 *  \param pdata Pointer to \ref mdio_relay_data.
 *
 *  \return Return value as follows:
 *  - 0: if successful
 *  - negative: error code
 */
int ext_mdio_read(const GSW_Device_t *dev, struct mdio_relay_data *pdata);

/** \brief Write external GPHY MDIO/MMD registers via embedded MDIO controller.
 *
 *  \param dev Pointer to GSW_Device_t (defined by application).
 *  \param pdata Pointer to \ref mdio_relay_data.
 *
 *  \return Return value as follows:
 *  - 0: if successful
 *  - negative: error code
 */
int ext_mdio_write(const GSW_Device_t *dev, struct mdio_relay_data *pdata);

/** \brief Modify external GPHY MDIO/MMD registers via embedded MDIO controller.
 *
 *  \param dev Pointer to GSW_Device_t (defined by application).
 *  \param pdata Pointer to \ref mdio_relay_mod_data.
 *
 *  \return Return value as follows:
 *  - 0: if successful
 *  - negative: error code
 */
int ext_mdio_mod(const GSW_Device_t *dev, struct mdio_relay_mod_data *pdata);

/** @} */	/* DOC_SYS_MISC */

#endif /*  _MDIO_RELAY_H_ */
