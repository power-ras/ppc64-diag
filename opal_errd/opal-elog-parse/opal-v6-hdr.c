#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <string.h>

#include "opal-v6-hdr.h"
#include "print_helpers.h"

int parse_section_header(struct opal_v6_hdr *hdr,
                         const char *buf, int buflen)
{
	if (buflen < sizeof(struct opal_v6_hdr)) {
		fprintf(stderr, "ERROR %s: section header is too small, "
		        "is meant to be %lu bytes, only %u found.\n",
		        __func__, sizeof(struct opal_v6_hdr), buflen);
		return -EINVAL;
	}

	const struct opal_v6_hdr* bufhdr = (struct opal_v6_hdr*)buf;

	assert(sizeof(struct opal_v6_hdr) == 8);

	memcpy(hdr->id, bufhdr->id, 2);
	hdr->length = be16toh(bufhdr->length);
	hdr->version = bufhdr->version;
	hdr->subtype = bufhdr->subtype;
	hdr->component_id = be16toh(bufhdr->component_id);

	if (hdr->length < 8) {
		fprintf(stderr, "ERROR %s: section header is corrupt. "
		        "Length < 8 bytes and must be at least 8 bytes "
		        "to include the length of itself. "
		        "Id 0x%x%x Length %u Version %u Subtype %u "
		        "Component ID: %u\n",
		        __func__, hdr->id[0], hdr->id[1],
		        hdr->length, hdr->version, hdr->subtype,
		        hdr->component_id);
		return -EINVAL;
	}

	return 0;
}

int print_opal_v6_hdr(const struct opal_v6_hdr hdr) {
	print_line("Section Version", "%d (%c%c)", hdr.version,
	           hdr.id[0], hdr.id[1]);
	print_line("Sub-section type", "0x%x", hdr.subtype);
	print_line("Section Length", "0x%x", hdr.length);
	print_line("Component ID", "%x", hdr.component_id);
	return 0;
}
