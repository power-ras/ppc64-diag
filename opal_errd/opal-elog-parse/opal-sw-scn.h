#ifndef _H_OPAL_SW_SCN
#define _H_OPAL_SW_SCN

#include "opal-v6-hdr.h"
#include "opal-sw-v1-scn.h"
#include "opal-sw-v2-scn.h"

struct opal_sw_scn {
	struct opal_v6_hdr v6hdr;
	union {
		struct opal_sw_v1_scn v1;
		struct opal_sw_v2_scn v2;
	} version;
} __attribute__((packed));

int parse_sw_scn(struct opal_sw_scn **r_sw,
                 struct opal_v6_hdr *hdr, const char *buf, int buflen);

int print_sw_scn(const struct opal_sw_scn *sw);

#endif /* _H_OPAL_SW_SCN */
