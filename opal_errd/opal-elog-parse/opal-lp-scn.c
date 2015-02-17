#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "opal-lp-scn.h"
#include "print_helpers.h"

int parse_lp_scn(struct opal_lp_scn **r_lp,
                 struct opal_v6_hdr *hdr, const char *buf, int buflen)
{
	struct opal_lp_scn *lp;
	struct opal_lp_scn *lpbuf = (struct opal_lp_scn *)buf;
	uint16_t *lps;
	uint16_t *lpsbuf;
	if (buflen < sizeof(struct opal_lp_scn) ||
			hdr->length < sizeof(struct opal_lp_scn)) {
		fprintf(stderr, "%s: corrupted, expected length => %lu, got %u\n",
		        __func__, sizeof(struct opal_lp_scn),
		        buflen < hdr->length ? buflen : hdr->length);
		return -EINVAL;
	}

	*r_lp = malloc(hdr->length);
	if (!*r_lp) {
		fprintf(stderr, "%s: out of memory\n", __func__);
		return -ENOMEM;
	}

	lp = *r_lp;

	lp->v6hdr = *hdr;
	lp->primary = be16toh(lpbuf->primary);
	lp->length_name = lpbuf->length_name;
	lp->lp_count = lpbuf->lp_count;
	lp->partition_id = be32toh(lpbuf->partition_id);
	lp->name[0] = '\0';
	int expected_len = sizeof(struct opal_lp_scn) + lp->length_name;
	if (buflen < expected_len || hdr->length < expected_len) {
		fprintf(stderr, "%s: corrupted, expected length => %u, got %u",
		        __func__, expected_len,
		        buflen < hdr->length ? buflen : hdr->length);
		free(lp);
		return -EINVAL;
	}
	memcpy(lp->name, lpbuf->name, lp->length_name);

	expected_len += lp->lp_count * sizeof(uint16_t);
	if (buflen < expected_len || hdr->length < expected_len) {
		fprintf(stderr, "%s: corrupted, expected length => %u, got %u",
		        __func__, expected_len,
		        buflen < hdr->length ? buflen : hdr->length);
		free(lp);
		return -EINVAL;
	}

	lpsbuf = (uint16_t *)(lpbuf->name + lpbuf->length_name);
	lps = (uint16_t *)(lp->name + lp->length_name);
	int i;
	for(i = 0; i < lp->lp_count; i++)
		lps[i] = be16toh(lpsbuf[i]);

	return 0;
}

int print_lp_scn(const struct opal_lp_scn *lp)
{
	print_header("Logical Partition");
	print_opal_v6_hdr(lp->v6hdr);
	print_line("Primary Partition ID", "0x%04x", lp->primary);
	print_line("Logical Partition Log ID", "0x%08x", lp->partition_id);
	print_line("Length of LP Name", "0x%02x", lp->length_name);
	print_line("Primary Partition Name", "%s", lp->name);
	int i;
	uint16_t *lps = (uint16_t *) (lp->name + lp->length_name);
	print_line("Target LP Count", "0x%02x", lp->lp_count);
	for(i = 0; i < lp->lp_count; i+=2) {
		if (i + 1 < lp->lp_count)
			print_line("Target LP", "0x%04x	 0x%04x", lps[i], lps[i+1]);
		else
			print_line("Target LP", "0x%04X", lps[i]);
	}

	print_bar();
	return 0;
}
