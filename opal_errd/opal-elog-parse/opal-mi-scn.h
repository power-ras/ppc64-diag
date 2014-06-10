#ifndef _H_OPAL_MI_SCN
#define _H_OPAL_MI_SCN

#include "opal-v6-hdr.h"

struct opal_mi_scn {
	struct opal_v6_hdr v6hdr;
	uint32_t flags;
	uint32_t reserved;
} __attribute__((packed));

int parse_mi_scn(struct opal_mi_scn **r_mi,
                 struct opal_v6_hdr *hdr,
                 const char *buf, int buflen);

int print_mi_scn(const struct opal_mi_scn *mi);

#endif /* _H_OPAL_MI_SCN */
