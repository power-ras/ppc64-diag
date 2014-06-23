#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "opal-src-scn.h"
#include "parse_helpers.h"
#include "print_helpers.h"

int parse_src_scn(struct opal_src_scn **r_src,
                  const struct opal_v6_hdr *hdr,
                  const char *buf, int buflen)
{
	struct opal_src_scn *bufsrc = (struct opal_src_scn*)buf;
	struct opal_src_scn *src;

	int offset = OPAL_SRC_SCN_STATIC_SIZE;

	int error = check_buflen(buflen, offset, __func__);
	if (error)
		return error;

/* header length can be > sizeof() as is variable sized section
 * subtract the size of the optional section
 */
	if (hdr->length < offset) {
		fprintf(stderr, "%s: section header length less than min size "
		        ". section header length %u, min size: %u\n",
		        __func__, hdr->length, offset);
		return -EINVAL;
	}

	*r_src = (struct opal_src_scn*) malloc(sizeof(struct opal_src_scn));
	if(!*r_src)
		return -ENOMEM;
	src = *r_src;

	src->v6hdr = *hdr;
	src->version = bufsrc->version;
	src->flags = bufsrc->flags;
	src->wordcount = bufsrc->wordcount;
	src->srclength = be16toh(bufsrc->srclength);
	src->ext_refcode2 = be32toh(bufsrc->ext_refcode2);
	src->ext_refcode3 = be32toh(bufsrc->ext_refcode3);
	src->ext_refcode4 = be32toh(bufsrc->ext_refcode4);
	src->ext_refcode5 = be32toh(bufsrc->ext_refcode5);
	src->ext_refcode6 = be32toh(bufsrc->ext_refcode6);
	src->ext_refcode7 = be32toh(bufsrc->ext_refcode7);
	src->ext_refcode8 = be32toh(bufsrc->ext_refcode8);
	src->ext_refcode9 = be32toh(bufsrc->ext_refcode9);

	memcpy(src->primary_refcode, bufsrc->primary_refcode,
	       OPAL_SRC_SCN_PRIMARY_REFCODE_LEN);

	src->fru_count = 0;
	if (src->flags & OPAL_SRC_ADD_SCN) {
		error = check_buflen(buflen, offset + sizeof(struct opal_src_add_scn_hdr), __func__);
		if (error) {
			free(src);
			return error;
		}

		src->addhdr.flags = bufsrc->addhdr.flags;
		src->addhdr.id = bufsrc->addhdr.id;
		if (src->addhdr.id != OPAL_FRU_SCN_ID) {
			fprintf(stderr, "%s: invalid section id, expecting 0x%x but found"
			        " 0x%x", __func__, OPAL_FRU_SCN_ID, src->addhdr.id);
			free(src);
			return -EINVAL;
		}
		src->addhdr.length = be16toh(bufsrc->addhdr.length);
		offset += sizeof(struct opal_src_add_scn_hdr);

		while(offset < src->srclength && src->fru_count < OPAL_SRC_FRU_MAX) {
			error = parse_fru_scn(&(src->fru[src->fru_count]), buf + offset, buflen - offset);
			if (error < 0) {
				free(src);
				return error;
			}
			offset += error;
			src->fru_count++;
		}
	}

	return 0;
}

static int print_src_refcode(const struct opal_src_scn *src)
{
	char primary_refcode_display[OPAL_SRC_SCN_PRIMARY_REFCODE_LEN+1];

	memcpy(primary_refcode_display, src->primary_refcode,
	       OPAL_SRC_SCN_PRIMARY_REFCODE_LEN);
	primary_refcode_display[OPAL_SRC_SCN_PRIMARY_REFCODE_LEN] = '\0';

	print_line("Primary Reference Code", "%s", primary_refcode_display);
	print_line("Hex Words 2 - 5", "%08X %08X %08X %08X",
	           src->ext_refcode2, src->ext_refcode3,
	           src->ext_refcode4, src->ext_refcode5);
	print_line("Hex Words 6 - 9", "%08X %08X %08X %08X",
	           src->ext_refcode6, src->ext_refcode7,
	          src->ext_refcode8, src->ext_refcode9);
	return 0;
}

int print_opal_src_scn(const struct opal_src_scn *src)
{
	if (src->v6hdr.id[0] == 'P')
		print_header("Primary System Reference Code");
	else
		print_header("Secondary System Reference Code");

	print_opal_v6_hdr(src->v6hdr);
	print_line("SRC Format", "0x%x", src->flags);
	print_line("SRC Version", "0x%x", src->version);
	print_line("Valid Word Count", "0x%x", src->wordcount);
	print_line("SRC Length", "%x", src->srclength);
	print_src_refcode(src);
	if(src->fru_count) {
		print_center(" ");
		print_center("Callout Section");
		print_center(" ");
		/* Hardcode this to look like FSP, not what what they want here... */
		print_line("Additional Sections", "Disabled");
		print_line("Callout Count", "%d", src->fru_count);
		int i;
		for (i = 0; i < src->fru_count; i++)
			print_fru_scn(src->fru[i]);
	}
	print_center(" ");
	return 0;
}

