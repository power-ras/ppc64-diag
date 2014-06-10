#ifndef _H_OPAL_LP_SCN
#define _H_OPAL_LP_SCN

#include "opal-v6-hdr.h"

struct opal_lp_scn {
	struct opal_v6_hdr v6hdr;
	uint16_t primary;
	uint8_t length_name;
	uint8_t lp_count;
	uint32_t partition_id;
	char name[0]; /* variable length */
	/* uint16_t *lps; variable length
	 * exists after name, position will be name + length name
	 */
} __attribute__((packed));

int parse_lp_scn(struct opal_lp_scn **r_lp,
                 struct opal_v6_hdr *hdr, const char *buf, int buflen);

int print_lp_scn(const struct opal_lp_scn *lp);

#endif /* _H_OPAL_LP_SCN */
