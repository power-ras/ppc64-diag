#ifndef _H_OPAL_DH_SCN
#define _H_OPAL_DH_SCN

#include "opal-v6-hdr.h"

#define DH_FLAG_DUMP_HEX 0x40

#define DH_DUMP_STR_MAX 40

struct opal_dh_scn {
	struct opal_v6_hdr v6hdr;
	uint32_t dump_id;
	uint8_t flags;
	uint8_t reserved[2];
	uint8_t length_dump_os;
	uint64_t dump_size;
	union {
		char dump_str[DH_DUMP_STR_MAX];
		uint32_t dump_hex;
	} shared;
} __attribute__((packed));

int parse_dh_scn(struct opal_dh_scn **r_dh,
                 struct opal_v6_hdr *hdr,
                 const char *buf, int buflen);

int print_dh_scn(const struct opal_dh_scn *dh);

#endif /* _H_OPAL_DH_SCN */
