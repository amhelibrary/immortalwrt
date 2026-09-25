#ifndef MXL_HOST_MAC_API_H_
#define MXL_HOST_MAC_API_H_

/* General */
#pragma pack(push, 1)
#pragma scalar_storage_order little-endian

enum {
	RMON_TX_PKTS = 0,
	RMON_TX_BYTES,
	RMON_TX_GOOD_PKTS,
	RMON_TX_GOOD_BYTES,
	RMON_TX_PAUSE,
	RMON_TX_UNDERFLOW,
	RMON_TX_SINGLE_COL,
	RMON_TX_MULTI_COL,
	RMON_TX_LATE_COL,
	RMON_TX_EXCS_COL,
	RMON_TX_ERR_PKTS,

	RMON_RX_PKTS,
	RMON_RX_BYTES,
	RMON_RX_GOOD_BYTES,
	RMON_RX_PAUSE,
	RMON_RX_CRC_ERR,
	RMON_RX_OVERFLOW,

	MAC_RMON_MAX
};

typedef struct {
	uint8_t mac_idx;
	u64 cnt[MAC_RMON_MAX];  /* RMON type number */
} host_mac_rmon_t;

typedef struct {
	uint8_t mac_idx;
	uint32_t reg_off;
	uint32_t reg_val;
} host_mac_register_t;

#pragma scalar_storage_order default
#pragma pack(pop)

int host_mac_rd_reg(const GSW_Device_t *dummy, host_mac_register_t *param);
int host_mac_wr_reg(const GSW_Device_t *dummy, host_mac_register_t *param);
int host_mac_rmon_get(const GSW_Device_t *dummy, host_mac_rmon_t *prmon);
int host_mac_rmon_clr(const GSW_Device_t *dummy, uint8_t *pidx);

#endif /* MXL_HOST_MAC_API_H_ */
