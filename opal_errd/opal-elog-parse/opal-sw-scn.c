#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "opal-v6-hdr.h"
#include "opal-sw-scn.h"
#include "print_helpers.h"

int parse_sw_v1_scn(struct opal_sw_v1_scn *swv1,
                    const char *buf, int buflen)
{
	struct opal_sw_v1_scn *swv1buf = (struct opal_sw_v1_scn *)buf;

	if (buflen < sizeof(struct opal_sw_v1_scn)) {
		fprintf(stderr, "%s: corrupted buffer, expecting length > %lu "
		        "got %d", __func__, sizeof(struct opal_sw_v1_scn),
		        buflen);
		return -EINVAL;
	}

	swv1->rc = be32toh(swv1buf->rc);
	swv1->line_num = be32toh(swv1buf->line_num);
	swv1->object_id = be32toh(swv1buf->object_id);
	swv1->id_length = swv1buf->id_length;

	if(buflen < sizeof(struct opal_sw_v1_scn) + swv1->id_length) {
		fprintf(stderr, "%s: corrupted buffer, expecting length > %lu "
		        "got %d", __func__, sizeof(struct opal_sw_v1_scn) +
		        swv1->id_length, buflen);
		return -EINVAL;
	}

	bzero(swv1->file_id, swv1->id_length);
	memcpy(swv1->file_id, swv1buf->file_id, swv1->id_length);
	swv1->file_id[swv1->id_length - 1] = '\0';

	return 0;
}

int parse_sw_v2_scn(struct opal_sw_v2_scn *swv2,
                    const char *buf, int buflen)
{
	struct opal_sw_v2_scn *swv2buf = (struct opal_sw_v2_scn *)buf;

	if (buflen < sizeof(struct opal_sw_v2_scn)) {
		fprintf(stderr, "%s: corrupted buffer, expected length == %lu, "
				"got %u\n", __func__, sizeof(struct opal_sw_v2_scn),
				buflen);
		return -EINVAL;
	}

	swv2->rc = be32toh(swv2buf->rc);
	swv2->file_id = be16toh(swv2buf->file_id);
	swv2->location_id = be16toh(swv2buf->location_id);
	swv2->object_id = be32toh(swv2buf->object_id);

	return 0;
}



int parse_sw_scn(struct opal_sw_scn **r_sw,
                 struct opal_v6_hdr *hdr, const char *buf, int buflen)
{
	struct opal_sw_scn *sw;
	int rc = 0;

	*r_sw = (struct opal_sw_scn *)malloc(hdr->length);
	if(!*r_sw)
		return -ENOMEM;
	sw = *r_sw;

	if (buflen < sizeof(struct opal_v6_hdr)) {
		free(sw);
		return -EINVAL;
	}

	sw->v6hdr = *hdr;

	if (hdr->version == 1) {
		if (hdr->length > sizeof(struct opal_sw_v1_scn) + sizeof(struct opal_v6_hdr)) {
			rc = parse_sw_v1_scn(&(sw->version.v1), buf, buflen - sizeof(struct opal_v6_hdr));
		} else {
			fprintf(stderr, "%s: corrupted section header, expected length > %lu, "
			        "got %u\n", __func__, sizeof(struct opal_sw_v1_scn) +
			        sizeof(struct opal_v6_hdr), hdr->length);
			rc = -EINVAL;
		}
	} else if (hdr->version == 2) {
		if (hdr->length == sizeof(struct opal_sw_v2_scn) + sizeof(struct opal_v6_hdr)) {
			rc =  parse_sw_v2_scn(&(sw->version.v2), buf, buflen - sizeof(struct opal_v6_hdr));
		} else {
			fprintf(stderr, "%s: corrupted section header, expected length == %lu, "
			        "got %u\n", __func__, sizeof(struct opal_sw_v2_scn) +
			        sizeof(struct opal_v6_hdr), hdr->length);
			rc = -EINVAL;
		}
	} else {
		fprintf(stderr, "ERROR %s: unknown version 0x%x\n", __func__, hdr->version);
		rc = -EINVAL;
	}

	if(rc != 0) {
		free(sw);
		return rc;
	}

	return 0;
}

int print_sw_scn(const struct opal_sw_scn *sw)
{
	print_header("Firmware Error Description");
	print_opal_v6_hdr(sw->v6hdr);
	if (sw->v6hdr.version == 1) {
		print_line("Return Code", "0x%08x", sw->version.v1.rc);
		print_line("Line Number", "%08d", sw->version.v1.line_num);
		print_line("Object Identifier", "0x%08x", sw->version.v1.object_id);
		print_line("File ID Length", "0x%x", sw->version.v1.id_length);
		print_line("File Identifier", "%s", sw->version.v1.file_id);
	} else if (sw->v6hdr.version == 2) {
		print_line("File Identifier", "0x%04x", sw->version.v2.file_id);
		print_line("Code Location", "0x%04x", sw->version.v2.location_id);
		print_line("Return Code", "0x%08x", sw->version.v2.rc);
		print_line("Object Identifier", "0x%08x", sw->version.v2.object_id);
	} else {
		print_line("Parse error", "Incompatible version - 0x%x", sw->v6hdr.version);
	}

	return 0;
}
