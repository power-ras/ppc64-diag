#ifndef _H_OPAL_V6_HEADER
#define _H_OPAL_V6_HEADER

#include <inttypes.h>

struct opal_v6_hdr {
	char     id[2];
	uint16_t length;       /* section length */
	uint8_t  version;      /* section version */
	uint8_t  subtype;      /* section sub-type id */
	uint16_t component_id; /* component id of section creator */
} __attribute__((packed));


int parse_section_header(struct opal_v6_hdr *hdr,
                         const char *buf, int buflen);

int print_opal_v6_hdr(const struct opal_v6_hdr hdr);

#endif /* _H_OPAL_V6_HEADER */
