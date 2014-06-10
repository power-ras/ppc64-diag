#ifndef _H_OPAL_ED_SCN
#define _H_OPAL_ED_SCN

#include "opal-v6-hdr.h"

struct opal_ed_scn {
	struct opal_v6_hdr v6hdr;
	uint8_t creator_id;
	uint8_t reserved[3];
	uint8_t user_data[0]; /* variable length */
} __attribute__((packed));

int parse_ed_scn(struct opal_ed_scn **r_ed,
                 struct opal_v6_hdr *hdr,
                 const char *buf, int buflen);

int print_ed_scn(const struct opal_ed_scn *ed);

#endif /* _H_OPAL_ED_SCN */
