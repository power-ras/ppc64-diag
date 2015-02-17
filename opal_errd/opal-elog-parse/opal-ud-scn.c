#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "opal-ud-scn.h"
#include "print_helpers.h"

int parse_ud_scn(struct opal_ud_scn **r_ud,
                 const struct opal_v6_hdr *hdr,
                 const char *buf, int buflen)
{
	struct opal_ud_scn *ud;
	struct opal_ud_scn *bufud = (struct opal_ud_scn *)buf;

	if (buflen < sizeof(struct opal_ud_scn)) {
		fprintf(stderr, "%s: corrupted, expected length >= %lu, got %u\n",
		        __func__, sizeof(struct opal_ud_scn), buflen);
		return -EINVAL;
	}

	*r_ud = malloc(hdr->length);
	if (!*r_ud)
		return -ENOMEM;
	ud = *r_ud;

	ud->v6hdr = *hdr;
	memcpy(ud->data, bufud->data, hdr->length - sizeof(struct opal_v6_hdr));

	return 0;
}

int print_ud_scn(const struct opal_ud_scn *ud)
{
	print_header("User Defined Data");
	print_opal_v6_hdr(ud->v6hdr);
	/*FIXME this data should be parsable if documentation appears/exists
	 * In the mean time, just dump it in hex
	 */
	print_line("User data hex","length %d",ud->v6hdr.length - 8);
	print_hex(ud->data, ud->v6hdr.length - 8);
	print_bar();
	return 0;
}
