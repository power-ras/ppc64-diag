#ifndef _H_OPAL_EI_SCN
#define _H_OPAL_EI_SCN

#include "opal-v6-hdr.h"

struct opal_ei_env_scn {
	uint32_t corrosion;
	uint16_t temperature;
	uint16_t rate;
} __attribute__((packed));

#define CORROSION_RATE_NORM  0x00
#define CORROSION_RATE_ABOVE 0x01

struct opal_ei_scn {
	struct opal_v6_hdr v6hdr;
	uint64_t g_timestamp;
	struct opal_ei_env_scn genesis;
	uint8_t status;
	uint8_t user_data_scn;
	uint16_t read_count;
	struct opal_ei_env_scn readings[0]; /* variable length */
} __attribute__((packed));

int parse_ei_scn(struct opal_ei_scn **r_ei,
                 struct opal_v6_hdr *hdr,
                 const char *buf, int buflen);

int print_ei_scn(const struct opal_ei_scn *ei);

#endif /* _H_OPAL_EI_SCN */
