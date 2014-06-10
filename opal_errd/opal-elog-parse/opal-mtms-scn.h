#ifndef _H_OPAL_MTMS_SCN
#define _H_OPAL_MTMS_SCN

#include "opal-v6-hdr.h"
#include "opal-mtms-struct.h"

struct opal_mtms_scn {
	struct opal_v6_hdr v6hdr;
	struct opal_mtms_struct mtms;
} __attribute__((packed));

int parse_mtms_scn(struct opal_mtms_scn **r_mtms, const struct opal_v6_hdr *hdr,
                   const char *buf, int buflen);

int print_mtms_scn(const struct opal_mtms_scn *mtms);

#endif /* _H_OPAL_MTMS_SCN */
