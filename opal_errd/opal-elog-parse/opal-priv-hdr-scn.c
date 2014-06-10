#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "opal-priv-hdr-scn.h"
#include "opal-datetime.h"
#include "opal-event-data.h"
#include "print_helpers.h"

int parse_priv_hdr_scn(struct opal_priv_hdr_scn **r_privhdr,
                       const struct opal_v6_hdr *hdr, const char *buf,
                       int buflen)
{
	struct opal_priv_hdr_scn *bufhdr = (struct opal_priv_hdr_scn*)buf;
	struct opal_priv_hdr_scn *privhdr;

	if (buflen < sizeof(struct opal_priv_hdr_scn)) {
		fprintf(stderr, "%s: corrupted, expected length %lu, got %u\n",
			__func__,
			sizeof(struct opal_priv_hdr_scn), buflen);
		return -EINVAL;
	}

	if (hdr->length != sizeof(struct opal_priv_hdr_scn)) {
		fprintf(stderr, "%s: section header length disagrees with spec"
			". section header length %u, spec: %lu\n",
			__func__,
			hdr->length, sizeof(struct opal_priv_hdr_scn));
		return -EINVAL;
	}

	*r_privhdr = (struct opal_priv_hdr_scn*) malloc(sizeof(struct opal_priv_hdr_scn));
	if (!*r_privhdr)
		return -ENOMEM;
	privhdr = *r_privhdr;

	privhdr->v6hdr = *hdr;
	privhdr->create_datetime = parse_opal_datetime(bufhdr->create_datetime);
	privhdr->commit_datetime = parse_opal_datetime(bufhdr->commit_datetime);
	privhdr->creator_id = bufhdr->creator_id;
	privhdr->scn_count = bufhdr->scn_count;
	if (privhdr->scn_count < 1) {
		fprintf(stderr, "%s: section header has an invalid section count %u, "
				"should be greater than 0, setting section count to 1 "
				"to attempt recovery\n", __func__, privhdr->scn_count);
		privhdr->scn_count = 1;
	}
	// FIXME: are these ASCII? Need spec clarification
	privhdr->creator_subid_hi = bufhdr->creator_subid_hi;
	privhdr->creator_subid_lo = bufhdr->creator_subid_lo;

	privhdr->plid = be32toh(bufhdr->plid);

	privhdr->log_entry_id = be32toh(bufhdr->log_entry_id);

	return 0;
}

int print_opal_priv_hdr_scn(const struct opal_priv_hdr_scn *privhdr)
{
	print_header("Private Header");
	print_opal_v6_hdr(privhdr->v6hdr);
	print_line("Created at", "%4u-%02u-%02u | %02u:%02u:%02u",
	           privhdr->create_datetime.year,
	           privhdr->create_datetime.month,
	           privhdr->create_datetime.day,
	           privhdr->create_datetime.hour,
	           privhdr->create_datetime.minutes,
	           privhdr->create_datetime.seconds);
	print_line("Committed at", "%4u-%02u-%02u | %02u:%02u:%02u",
	           privhdr->commit_datetime.year,
	           privhdr->commit_datetime.month,
	           privhdr->commit_datetime.day,
	           privhdr->commit_datetime.hour,
	           privhdr->commit_datetime.minutes,
	           privhdr->commit_datetime.seconds);
	print_line("Created by", "%s", get_creator_name(privhdr->creator_id));
	print_line("Creator Sub Id", "0x%x (%u), 0x%x (%u)",
	           privhdr->creator_subid_hi,
	           privhdr->creator_subid_hi,
	           privhdr->creator_subid_lo,
	           privhdr->creator_subid_lo);
	print_line("Platform Log Id", "0x%x", privhdr->plid);
	print_line("Entry ID", "0x%x", privhdr->log_entry_id);
	print_line("Section Count","%u",privhdr->scn_count);
	return 0;
}
