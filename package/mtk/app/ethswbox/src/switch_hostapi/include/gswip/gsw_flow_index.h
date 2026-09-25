/******************************************************************************

   Copyright 2023-2024 MaxLinear, Inc.

   For licensing information, see the file 'LICENSE' in the root folder of
   this software module.

******************************************************************************/

#ifndef MXL_GSW_FLOW_INDEX_H_
#define MXL_GSW_FLOW_INDEX_H_

/***************
 * Global Rules
 ***************/

/* IGMP v1/2/3 */
/* logical index 0 */
#define IGMP_PCE_RULE_INDEX             0
#define IGMP_PCE_RULE_INDEX_END         (IGMP_PCE_RULE_INDEX + 1)

/* 802.1x (reserved name for future) */
/* logical index 1 */
#define EAPOL_PCE_RULE_INDEX            IGMP_PCE_RULE_INDEX_END
#define EAPOL_PCE_RULE_INDEX_END        (EAPOL_PCE_RULE_INDEX + 1)

/* STP/RSTP */
/* logical index 2 */
#define BPDU_PCE_RULE_INDEX             EAPOL_PCE_RULE_INDEX_END
#define BPDU_PCE_RULE_INDEX_END         (BPDU_PCE_RULE_INDEX + 1)

/* Pause Frame Discard */
/* logical index 3 */
#define PFC_PCE_RULE_INDEX              BPDU_PCE_RULE_INDEX_END
#define PFC_PCE_RULE_INDEX_END          (PFC_PCE_RULE_INDEX + 1)

/* LLDP */
/* logical index 4 */
#define LLDP_PCE_RULE_INDEX             PFC_PCE_RULE_INDEX_END
#define LLDP_PCE_RULE_INDEX_END         (LLDP_PCE_RULE_INDEX + 1)

/* OAM 802.3ah (LACP, Loop Detection)
 * This has to be reserved for backward compatible to avoid broken (such as DSA)
 * even if CONFIG_NET_L2_OAM_8023AH_PER_PORT_RULE (per port OAM/LACP rule) is
 * enabled.
 */
/* logical index 5 */
#define OAM_8023AH_PCE_RULE_INDEX       LLDP_PCE_RULE_INDEX_END
#define OAM_8023AH_PCE_RULE_INDEX_END   (OAM_8023AH_PCE_RULE_INDEX + 1)
#define LACP_PCE_RULE_INDEX             OAM_8023AH_PCE_RULE_INDEX
#define LACP_PCE_RULE_INDEX_END         OAM_8023AH_PCE_RULE_INDEX_END

/* PTP (1588 v1/2) */
/* logical index 6 */
#define PTP_PCE_RULE_INDEX              LACP_PCE_RULE_INDEX_END
#define PTP_PCE_RULE_INDEX_END          (PTP_PCE_RULE_INDEX + 1)

/* System MAC */
/* logical index 7 */
#define MAC_PCE_RULE_INDEX              PTP_PCE_RULE_INDEX_END
#define MAC_PCE_RULE_INDEX_END          (MAC_PCE_RULE_INDEX + 1)

/* ARP Request */
/* logical index 8~9 */
#define ARP_PCE_RULE_INDEX              MAC_PCE_RULE_INDEX_END
#define ARP_PCE_RULE_INDEX_END          (ARP_PCE_RULE_INDEX + 2)
#if defined(CONFIG_SOC_F48X_ARC) && defined(NET_IF_MAX_IPV4_ADDR) && \
    NET_IF_MAX_IPV4_ADDR > 2
#error CONFIG_NET_IF_UNICAST_IPV4_ADDR_COUNT should not exceed 2, or ARP_PCE_RULE_INDEX_END need be fixed.
#endif

/* Defensive ARP Announcement */
/* logical index 10~11 */
#define ARP_DEF_PCE_RULE_INDEX		ARP_PCE_RULE_INDEX_END
#define ARP_DEF_PCE_RULE_INDEX_END	(ARP_DEF_PCE_RULE_INDEX + 2)

/* DSA Extension */
/* logical index 12 */
#define DSA_EXT_PCE_RULE_INDEX          ARP_DEF_PCE_RULE_INDEX_END
#define DSA_EXT_PCE_RULE_INDEX_END      (DSA_EXT_PCE_RULE_INDEX + 1)

/* ICMPv6 Neighbor Solicitation Request */
/* logical index 13~15 */
#define NS_PCE_RULE_INDEX               DSA_EXT_PCE_RULE_INDEX_END
#define NS_PCE_RULE_INDEX_END           (NS_PCE_RULE_INDEX + 3)
#if defined(CONFIG_SOC_F48X_ARC) && defined(NET_IF_MAX_IPV6_ADDR) && \
    NET_IF_MAX_IPV6_ADDR > 3
#error CONFIG_NET_IF_UNICAST_IPV6_ADDR_COUNT should not exceed 3, or NS_PCE_RULE_INDEX_END need be fixed.
#endif

/* ICMPv6 Router Advertisement */
/* logical index 16 */
#define RA_PCE_RULE_INDEX               NS_PCE_RULE_INDEX_END
#define RA_PCE_RULE_INDEX_END           (RA_PCE_RULE_INDEX + 1)

/* DHCPv6 */
/* logical index 17 */
#define DHCPV6_PCE_RULE_INDEX           RA_PCE_RULE_INDEX_END
#define DHCPV6_PCE_RULE_INDEX_END       (DHCPV6_PCE_RULE_INDEX + 1)

/* MLD v1/2 */
/* logical index 18~21 */
#define MLD_PCE_RULE_INDEX              DHCPV6_PCE_RULE_INDEX_END
#define MLD_PCE_RULE_INDEX_END          (MLD_PCE_RULE_INDEX + 4)

/* Ethernet FW Upgrade */
/* logical index 22 */
#define ETH_FW_PCE_RULE_INDEX		MLD_PCE_RULE_INDEX_END
#define ETH_FW_PCE_RULE_INDEX_END	(ETH_FW_PCE_RULE_INDEX + 1)

/* total number of predefined global PCE rules
 * must not exceed 128 (<= 128)
 */
#define MAX_PREDEFINED_GLOBAL_PCE_RULE	ETH_FW_PCE_RULE_INDEX_END

/****************
 * Per CTP Rules
 ****************/

/* Flow Control */
/* logical index 0 per CTP */
#define FC_PCE_RULE_INDEX		0

/* OAM 802.3ah (LACP, Loop Detection) */
/* logical index 1 per CTP */
#define OAM_LACP_PCE_RULE_PORT_INDEX	(FC_PCE_RULE_INDEX + 1)

/* ICMP DoS Attack Prevention */
/* logical index 2 per CTP */
#define ICMP_DOS_PCE_RULE_PORT_INDEX	(OAM_LACP_PCE_RULE_PORT_INDEX + 1)

/* total number of predefined per CTP PCE rules */
#define MAX_PREDEFINED_PER_CTP_PCE_RULE	(ICMP_DOS_PCE_RULE_PORT_INDEX + 1)

#endif /* #ifndef MXL_GSW_FLOW_INDEX_H_ */
