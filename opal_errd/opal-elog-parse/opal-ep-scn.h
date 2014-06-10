#ifndef _H_OPAL_EP_SCN
#define _H_OPAL_EP_SCN

#include "opal-v6-hdr.h"

#define OPAL_EP_VALUE_SHIFT 4
#define OPAL_EP_ACTION_BITS 0x0F
#define OPAL_EP_VALUE_SET 3

#define OPAL_EP_EVENT_BITS 0x0F
#define OPAL_EP_EVENT_SHIFT 4

#define OPAL_EP_HDR_V 0x02

struct opal_ep_scn {
	struct opal_v6_hdr v6hdr;
	uint8_t value;
	uint8_t modifier;
	uint16_t ext_modifier;
	uint32_t reason;
} __attribute__((packed));

int parse_ep_scn(struct opal_ep_scn **r_ep,
                 const struct opal_v6_hdr *hdr,
                 const char *buf, int buflen);

int print_ep_scn(const struct opal_ep_scn *ep);

#endif /* _H_OPAL_EP_SCN */
