#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "opal-src-fru-scn.h"
#include "parse_helpers.h"
#include "opal-event-data.h"
#include "print_helpers.h"

static int parse_opal_fru_hdr(struct opal_fru_hdr *hdr, const char *buf, int buflen) {
	struct opal_fru_hdr *bufsrc = (struct opal_fru_hdr *)buf;

	hdr->length = bufsrc->length;
	hdr->flags = bufsrc->flags;
	hdr->type = be16toh(bufsrc->type);
	return 0;
}

/* This function will return a negative on error but a positive on success
 * The positive integer indicates how many bytes were consumed from buf
 */
static int parse_src_fru_mr_scn(struct opal_fru_mr_sub_scn *mr, const char *buf, int buflen)
{
	struct opal_fru_mr_sub_scn *bufsrc = (struct opal_fru_mr_sub_scn *)buf;
	int min_length = sizeof(struct opal_fru_hdr) + sizeof(uint32_t);
	int error = check_buflen(buflen, min_length, __func__);
	if (error)
		return error;

	parse_opal_fru_hdr(&(mr->hdr), buf, buflen);
	if (mr->hdr.type != OPAL_FRU_MR_TYPE) {
		fprintf(stderr, "%s: unexpected ID type, expecting 0x%x but found 0x%x\n",
		        __func__, OPAL_FRU_MR_TYPE, mr->hdr.type);
		return -EINVAL;
	}
	mr->reserved = bufsrc->reserved;

	int i = 0;
	while (mr->hdr.length > min_length) {
		if(mr->hdr.length - min_length < sizeof(struct opal_fru_mr_mru_scn)) {
			fprintf(stderr, "%s: sub section is too short, expecting %lu bytes"
			        "for mru sub section.\n", __func__, sizeof(struct opal_fru_mr_mru_scn));
			return -EINVAL;
		}

		error = check_buflen(buflen, min_length + sizeof(struct opal_fru_mr_mru_scn), __func__);
		if (error)
			return error;

		mr->mru[i] = bufsrc->mru[i];
		mr->mru[i].id = be32toh(bufsrc->mru[i].id);
		i++;
		min_length += sizeof(struct opal_fru_mr_mru_scn);
	}

	return min_length;
}

/* This function will return a negative on error but a positive on success
 * The positive integer indicates how many bytes were consumed from buf
 */
static int parse_src_fru_pe_scn(struct opal_fru_pe_sub_scn *pe, const char *buf, int buflen)
{
	struct opal_fru_pe_sub_scn *bufsrc = (struct opal_fru_pe_sub_scn *)buf;

	int min_length = sizeof(struct opal_fru_hdr) + sizeof(struct opal_mtms_struct);
	int error = check_buflen(buflen, min_length, __func__);
	if (error)
		return error;

	parse_opal_fru_hdr(&(pe->hdr), buf, buflen);
	memcpy(pe->mtms.model, bufsrc->mtms.model, OPAL_SYS_MODEL_LEN);
	memcpy(pe->mtms.serial_no, bufsrc->mtms.serial_no, OPAL_SYS_SERIAL_LEN);
	if (pe->hdr.type != OPAL_FRU_PE_TYPE) {
		fprintf(stderr, "%s: unexpected ID type, expecting 0x%x but found 0x%x\n",
		        __func__, OPAL_FRU_PE_TYPE, pe->hdr.type);
		return -EINVAL;
	}
	pe->pce[0] = '\0';
	/* Check to see if we have a string */
	if(pe->hdr.length > min_length) {
		/* Ensure string isn't too log */
		if(pe->hdr.length - min_length > OPAL_FRU_PE_PCE_MAX) {
			fprintf(stderr, "%s: sub section is too long, expecting a max of %d chars"
			        "for pce string.\n", __func__, OPAL_FRU_PE_PCE_MAX);
			return -EINVAL;
		}

		error = check_buflen(buflen, min_length + (pe->hdr.length - min_length), __func__);
		if (error)
			return error;

		memcpy(pe->pce, buf + min_length, pe->hdr.length - min_length);
		min_length = pe->hdr.length;
	}
	return min_length;
}

/* This function will return a negative on error but a positive on success
 * The positive integer indicates how many bytes were consumed from buf
 */
static int parse_src_fru_id_scn(struct opal_fru_id_sub_scn *id, const char *buf, int buflen)
{
	int min_length = sizeof(struct opal_fru_hdr);
	int error = check_buflen(buflen, min_length, __func__);
	if(error)
		return error;

	parse_opal_fru_hdr(&(id->hdr), buf, buflen);
	if (id->hdr.type != OPAL_FRU_ID_TYPE) {
		fprintf(stderr, "%s: unexpected ID type, expecting 0x%x but found 0x%x\n",
		        __func__, OPAL_FRU_ID_TYPE, id->hdr.type);
		return -EINVAL;
	}

	if ((id->hdr.flags & OPAL_FRU_ID_PART) &&
	    (id->hdr.flags & OPAL_FRU_ID_PROC)) {
		fprintf(stderr, "%s: cannot have both bits 0x%x and 0x%x set in header\n",
		        __func__, OPAL_FRU_ID_PART, OPAL_FRU_ID_PROC);
		return -EINVAL;
	} else if((id->hdr.flags & OPAL_FRU_ID_PART) ||
			(id->hdr.flags & OPAL_FRU_ID_PROC)) {
		error = check_buflen(buflen, min_length + OPAL_FRU_ID_PART_MAX, __func__);
		if (error)
			return error;

		memcpy(id->part, buf + min_length, OPAL_FRU_ID_PART_MAX);
		min_length += OPAL_FRU_ID_PART_MAX;
	}

	if (id->hdr.flags & OPAL_FRU_ID_CCIN) {
		if (!(id->hdr.flags & OPAL_FRU_ID_PART)) {
			fprintf(stderr, "%s: cannot parse 0x%x bit in the header without 0x%x"
			        " being set\n", __func__, OPAL_FRU_ID_CCIN, OPAL_FRU_ID_PART);
			return -EINVAL;
		}
		error = check_buflen(buflen, min_length + OPAL_FRU_ID_CCIN_MAX, __func__);
		if (error)
			return error;

		memcpy(id->ccin, buf + min_length, OPAL_FRU_ID_CCIN_MAX);
		min_length += OPAL_FRU_ID_CCIN_MAX;
	}

	if (id->hdr.flags & OPAL_FRU_ID_SERIAL) {
		if (!(id->hdr.flags & OPAL_FRU_ID_PART)) {
			fprintf(stderr, "%s: cannot parse 0x%x bit in the header without 0x%x"
			        " being set\n", __func__, OPAL_FRU_ID_SERIAL, OPAL_FRU_ID_PART);
			return -EINVAL;
		}
		error = check_buflen(buflen, min_length + OPAL_FRU_ID_SERIAL_MAX, __func__);
		if (error)
			return error;

		memcpy(id->serial, buf + min_length, OPAL_FRU_ID_SERIAL_MAX);
		min_length += OPAL_FRU_ID_SERIAL_MAX;
	}

	return min_length;
}

/* This function will return a negative on error
 * A non negative value indicates the presence of another fru section
 * increment the buffer by the returned value and parse the next fru
 * section
 */
int parse_fru_scn(struct opal_fru_scn *fru_scn, const char *buf, int buflen)
{
	struct opal_fru_scn *bufsrc = (struct opal_fru_scn *)buf;

/* It is hard to assert that out buffer is big enough here as there are too
 * many variable length sections. Be sure to check throughout
 *
 * The minimum we need is the start of the opan_fru_src_scn struct
 */
	int offset = OPAL_FRU_SCN_STATIC_SIZE;
	int error = check_buflen(buflen, offset, __func__);
	if (error)
		return error;

	fru_scn->length = bufsrc->length;
	fru_scn->type = bufsrc->type;
	fru_scn->priority = bufsrc->priority;
	fru_scn->loc_code_len = bufsrc->loc_code_len;
	if (bufsrc->loc_code_len % 4) {
		fprintf(stderr, "%s: invalide location code length, must be multiple "
		        "of 4, got: %u", __func__, bufsrc->loc_code_len);
		return -EINVAL;
	}

	if (fru_scn->loc_code_len > OPAL_FRU_LOC_CODE_MAX) {
		fprintf(stderr, "%s: invalid location_code length. Expecting value <= %d"
		        " but got %d", __func__, OPAL_FRU_LOC_CODE_MAX,
		        fru_scn->loc_code_len);
		return -EINVAL;
	}

	offset += bufsrc->loc_code_len;
	error = check_buflen(buflen, offset, __func__);
	if (error)
		return error;

	fru_scn->location_code[0] = '\0';
	memcpy(fru_scn->location_code, bufsrc->location_code, fru_scn->loc_code_len);

	if (fru_scn->type & OPAL_FRU_ID_SUB) {
		offset += parse_src_fru_id_scn(&(fru_scn->id), buf + offset, buflen - offset);
		if (error)
			return error;
	}

	if (fru_scn->type & OPAL_FRU_PE_SUB) {
		offset += parse_src_fru_pe_scn(&(fru_scn->pe), buf + offset, buflen - offset);
		if (error)
			return error;
	}

	if (fru_scn->type & OPAL_FRU_MR_SUB) {
		offset += parse_src_fru_mr_scn(&(fru_scn->mr), buf + offset, buflen - offset);
		if (error)
			return error;
	}

	if (offset != fru_scn->length) {
		fprintf(stderr, "%s: Parsed amount of data does not match the header length"
		        " parsed: %d vs expected: %d\n", __func__,
		        offset, fru_scn->length);
		return -EINVAL;
	}
	return offset;
}

static int print_fru_id_scn(const struct opal_fru_id_sub_scn id)
{
	if (id.hdr.flags & OPAL_FRU_ID_PART)
		print_line("Part Number", "%s", id.part);

	if (id.hdr.flags & OPAL_FRU_ID_PROC)
		print_line("Procedure Number", "%s", id.part);

	if (id.hdr.flags & OPAL_FRU_ID_CCIN)
		print_line("CCIN", "%c%c%c%c", id.ccin[0], id.ccin[1],
		           id.ccin[2], id.ccin[3]);

	if (id.hdr.flags & OPAL_FRU_ID_SERIAL) {
		char tmp[OPAL_FRU_ID_SERIAL_MAX+1];
		memset(tmp, 0, OPAL_FRU_ID_SERIAL_MAX+1);
		memcpy(tmp, id.serial, OPAL_FRU_ID_SERIAL_MAX);
		print_line("Serial Number", "%s", tmp);
	}

	return 0;
}

static int print_fru_pe_scn(const struct opal_fru_pe_sub_scn pe)
{
	print_mtms_struct(pe.mtms);
	print_line("PCE", "%s", pe.pce);
	return 0;
}

static int print_fru_mr_scn(const struct opal_fru_mr_sub_scn mr)
{

	int total_mru = mr.hdr.flags & 0x0F;
	int i;
	for (i = 0; i < total_mru; i++) {
		print_line("MRU ID", "0x%x", mr.mru[i].id);

		print_line("Priority", "%s", get_fru_priority_desc(mr.mru[i].priority));
	}
	return 0;
}

int print_fru_scn(const struct opal_fru_scn *fru)
{
	/* FIXME This printing was to roughly confirm the correctness
	 * of the parsing. Improve here.
	 */
	print_center(" ");
	if (fru->type & OPAL_FRU_ID_SUB)
		print_center(get_fru_component_desc(fru->id.hdr.flags & 0xF0));

	print_line("Priority", "%s", get_fru_priority_desc(fru->priority));
	if (fru->location_code[0] != '\0')
		print_line("Location Code","%s", fru->location_code);

	if (fru->type & OPAL_FRU_ID_SUB)
		print_fru_id_scn(fru->id);

	if(fru->type & OPAL_FRU_PE_SUB)
		print_fru_pe_scn(fru->pe);

	if(fru->type & OPAL_FRU_MR_SUB)
		print_fru_mr_scn(fru->mr);

	return 0;
}
