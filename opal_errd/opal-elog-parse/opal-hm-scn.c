#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "opal-hm-scn.h"
#include "print_helpers.h"

int parse_hm_scn(struct opal_hm_scn **r_hm,
                 const struct opal_v6_hdr *hdr,
                 const char *buf, int buflen)
{
	struct opal_hm_scn *bufhm = (struct opal_hm_scn *)buf;
	struct opal_hm_scn *hm;
	if (buflen < sizeof(struct opal_hm_scn)) {
		fprintf(stderr, "%s: corrupted, expected length >= %lu, got %u\n",
			__func__,
			sizeof(struct opal_hm_scn), buflen);
		return -EINVAL;
	}

	*r_hm = malloc(sizeof(struct opal_hm_scn));
	if(!*r_hm)
		return -ENOMEM;
	hm = *r_hm;

	hm->v6hdr = *hdr;
	copy_mtms_struct(&(hm->mtms), &(bufhm->mtms));

	return 0;
}

int print_hm_scn(const struct opal_hm_scn *hm)
{
	print_header("Hypervisor ID");

	print_opal_v6_hdr(hm->v6hdr);
	print_mtms_struct(hm->mtms);

	return 0;
}
