#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "opal-mi-scn.h"
#include "print_helpers.h"

int parse_mi_scn(struct opal_mi_scn **r_mi,
                 struct opal_v6_hdr *hdr,
                 const char *buf, int buflen)
{
	struct opal_mi_scn *mibuf = (struct opal_mi_scn *)buf;
	struct opal_mi_scn *mi;

	if (buflen < sizeof(struct opal_mi_scn)) {
		fprintf(stderr, "%s: corrupted, expected length => %lu, got %u",
		        __func__, sizeof(struct opal_mi_scn), buflen);
		return -EINVAL;
	}

	*r_mi = malloc(sizeof(struct opal_mi_scn));
	if (!*r_mi)
		return -ENOMEM;
	mi = *r_mi;

	mi->v6hdr = *hdr;
	mi->flags = be32toh(mibuf->flags);

	return 0;
}

int print_mi_scn(const struct opal_mi_scn *mi)
{

	print_header("Manufacturing Information");
	print_opal_v6_hdr(mi->v6hdr);
	print_line("Policy Flags", "0x%08x", mi->flags);

	return 0;
}

