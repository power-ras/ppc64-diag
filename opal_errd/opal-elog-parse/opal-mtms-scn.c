#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "opal-mtms-scn.h"
#include "print_helpers.h"

int parse_mtms_scn(struct opal_mtms_scn **r_mtms, const struct opal_v6_hdr *hdr,
		const char *buf, int buflen) {

	struct opal_mtms_scn *bufmtms = (struct opal_mtms_scn*)buf;
	struct opal_mtms_scn *mtms;

	if (buflen < sizeof(struct opal_mtms_scn)) {
		fprintf(stderr, "%s: corrupted, expected length %lu, got %u\n",
		        __func__, sizeof(struct opal_mtms_scn), buflen);
		return -EINVAL; }

	if (hdr->length != sizeof(struct opal_mtms_scn)) {
		fprintf(stderr, "%s: section header length disagrees with spec"
		        ". section header length %u, spec: %lu\n",
		        __func__, hdr->length, sizeof(struct opal_mtms_scn));
		return -EINVAL;
	}

	*r_mtms = malloc(sizeof(struct opal_mtms_scn));
	if(!*r_mtms)
		return -ENOMEM;
	mtms = *r_mtms;

	mtms->v6hdr = *hdr;
	copy_mtms_struct(&(mtms->mtms), &(bufmtms->mtms));

	return 0;
}

int print_mtms_scn(const struct opal_mtms_scn *mtms)
{
	/*
	 * This is a workaround for a known gcc bug where passing packed structs by
	 * value to functions requires an intermediate otherwise gcc will read past
	 * the end of the struct during the copy.
	 */
	struct opal_mtms_struct tmp = mtms->mtms;

	print_header("Machine Type/Model & Serial Number");
	print_opal_v6_hdr(mtms->v6hdr);
	print_mtms_struct(tmp);
	print_bar();
	return 0;
}
