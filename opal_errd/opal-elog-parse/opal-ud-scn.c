#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stddef.h>

#include "opal-ud-scn.h"
#include "print_helpers.h"

int parse_ud_scn(struct opal_ud_scn **r_ud,
                 const struct opal_v6_hdr *hdr,
                 const char *buf, int buflen)
{
	struct opal_ud_scn *ud;
	const struct opal_ud_scn *bufud = (const struct opal_ud_scn *)buf;
	size_t len = hdr->length;   /* widen once, after reading */

	if (buflen < 0 || (size_t)buflen < sizeof(*ud)) {
		fprintf(stderr, "%s: corrupted, expected buflen >= %zu, got %d\n",
		        __func__, sizeof(*ud), buflen);
		return -EINVAL;
	}

	/* hdr->length is untrusted: bound it on both sides before using it */
	if (len < sizeof(*ud) || len > (size_t)buflen) {
		fprintf(stderr, "%s: bad section length %zu (buflen %d, min %zu)\n",
		        __func__, len, buflen, sizeof(*ud));
		return -EINVAL;
	}

	ud = malloc(len);
	if (!ud)
		return -ENOMEM;

	ud->v6hdr = *hdr;
	memcpy(ud->data, bufud->data, len - offsetof(struct opal_ud_scn, data));

	*r_ud = ud;   /* only publish on success */
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
