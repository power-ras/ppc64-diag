#ifndef _H_OPAL_CH_SCN
#define _H_OPAL_CH_SCN

#include "opal-v6-hdr.h"

#define OPAL_CH_COMMENT_MAX_LEN 144

struct opal_ch_scn {
	struct   opal_v6_hdr v6hdr;
	char comment[0]; /* varsized up to 144 byte null terminated */
} __attribute__((packed));

int parse_ch_scn(struct opal_ch_scn **r_ch,
                 struct opal_v6_hdr *hdr,
                 const char *buf, int buflen);

int print_ch_scn(const struct opal_ch_scn *ch);

#endif /* _H_OPAL_CH_SCN */
