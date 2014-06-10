#ifndef _H_OPAL_IE_SCN
#define _H_OPAL_IE_SCN

#include "opal-v6-hdr.h"

#define IE_TYPE_ERROR_DET 0x01
#define IE_TYPE_ERROR_REC 0x02
#define IE_TYPE_EVENT 0x03
#define IE_TYPE_RPC_PASS_THROUGH 0x04

#define IE_SUBTYPE_REBALANCE 0x01
#define IE_SUBTYPE_NODE_ONLINE 0x03
#define IE_SUBTYPE_NODE_OFFLINE 0x04
#define IE_SUBTYPE_PLAT_MAX_CHANGE 0x05

#define IE_DATA_MAX 216

struct opal_ie_scn {
	struct opal_v6_hdr v6hdr;
	uint8_t type;
	uint8_t rpc_len;
	uint8_t scope;
	uint8_t subtype;
	uint32_t drc;
	union {
		uint8_t rpc[IE_DATA_MAX];
		uint64_t max;
	} data;
} __attribute__((packed));

int parse_ie_scn(struct opal_ie_scn **r_ie,
                 struct opal_v6_hdr *hdr,
                 const char *buf, int buflen);

int print_ie_scn(const struct opal_ie_scn *ie);

#endif /* _H_OPAL_IE_SCN */
