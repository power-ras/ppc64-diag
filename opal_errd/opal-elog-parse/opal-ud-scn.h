#ifndef _H_OPAL_UD_SCN
#define _H_OPAL_UD_SCN

#include "opal-v6-hdr.h"

/* User defined data header section */
struct opal_ud_scn {
	struct opal_v6_hdr v6hdr;
	uint8_t data[0]; /* variable sized */
} __attribute__((packed));

int parse_ud_scn(struct opal_ud_scn **r_ud,
                 const struct opal_v6_hdr *hdr,
                 const char *buf, int buflen);

int print_ud_scn(const struct opal_ud_scn *ud);

#endif /* _H_OPAL_UD_SCN */
