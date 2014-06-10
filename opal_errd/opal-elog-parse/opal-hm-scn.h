#ifndef _H_OPAL_HM_SCN
#define _H_OPAL_HM_SCN

#include "opal-v6-hdr.h"
#include "opal-mtms-struct.h"

struct opal_hm_scn {
	struct opal_v6_hdr v6hdr;
	struct opal_mtms_struct mtms;
} __attribute__((packed));

int parse_hm_scn(struct opal_hm_scn **r_hm,
                 const struct opal_v6_hdr *hdr,
                 const char *buf, int buflen);

int print_hm_scn(const struct opal_hm_scn *hm);

#endif /* _H_OPAL_HM_SCN */
