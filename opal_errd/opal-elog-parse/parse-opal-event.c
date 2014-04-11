#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <ctype.h>
#include <assert.h>
#include "libopalevents.h"
#include "print-opal-event.h"
#include "opal-event-data.h"

struct header_id elog_hdr_id[] = {
			HEADER_ORDER
};

/* parse MTMS section of the log */
int parse_mt_scn(const struct opal_v6_hdr *hdr,
		const char *buf, int buflen)
{
	struct opal_mtms_scn mt;
	struct opal_mtms_scn *bufmt = (struct opal_mtms_scn*)buf;

	if (buflen < sizeof(struct opal_mtms_scn)) {
		fprintf(stderr, "%s: corrupted, expected length %lu, got %u\n",
			__func__,
			sizeof(struct opal_mtms_scn), buflen);
		return -EINVAL;
	}

	if (hdr->length != sizeof(struct opal_mtms_scn)) {
		fprintf(stderr, "%s: section header length disagrees with spec"
			". section header length %u, spec: %lu\n",
			__func__,
			hdr->length, sizeof(struct opal_mtms_scn));
		return -EINVAL;
	}

	mt.v6hdr = *hdr;
	memcpy(mt.mt.model, bufmt->mt.model, OPAL_SYS_MODEL_LEN);
	memcpy(mt.mt.serial_no, bufmt->mt.serial_no, OPAL_SYS_SERIAL_LEN);

	print_mt_scn(&mt);
	return 0;
}

/* It is imperative that this function return negative on error */
int check_buflen(int buflen, int min_length, const char *func)
{
	if (buflen < min_length) {
		fprintf(stderr, "%s: corrupted, expected minimum length %d, got %d\n",
				func, min_length, buflen);
		return -EINVAL;
	}
	return 0;
}

int parse_opal_fru_hdr(struct opal_fru_hdr *hdr, const char *buf, int buflen) {
	struct opal_fru_hdr *bufsrc = (struct opal_fru_hdr *)buf;

	hdr->length = bufsrc->length;
	hdr->flags = bufsrc->flags;
	hdr->type = be16toh(bufsrc->type);
	return 0;
}

/* This function will return a negative on error but a positive on success
 * The positive integer indicates how many bytes were consumed from buf
 */
int parse_src_fru_mr_scn(struct opal_fru_mr_sub_scn *mr, const char *buf, int buflen)
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
int parse_src_fru_pe_scn(struct opal_fru_pe_sub_scn *pe, const char *buf, int buflen)
{
	struct opal_fru_pe_sub_scn *bufsrc = (struct opal_fru_pe_sub_scn *)buf;

	int min_length = sizeof(struct opal_fru_hdr) + sizeof(struct opal_mt_struct);
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
int parse_src_fru_id_scn(struct opal_fru_id_sub_scn *id, const char *buf, int buflen)
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

	if ((id->hdr.flags & OPAL_FRU_ID_PART)
				&& (id->hdr.flags & OPAL_FRU_ID_PROC)) {
				fprintf(stderr, "%s: cannot have both bits 0x%x and 0x%x set in header\n",
					__func__, OPAL_FRU_ID_PART, OPAL_FRU_ID_PROC);
				return -EINVAL;
	} else if((id->hdr.flags & OPAL_FRU_ID_PART)
				|| (id->hdr.flags & OPAL_FRU_ID_PROC)) {
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
/* parse SRC section of the log */
int parse_src_scn(const struct opal_v6_hdr *hdr,
			const char *buf, int buflen)
{
	struct opal_src_scn src;
	struct opal_src_scn *bufsrc = (struct opal_src_scn*)buf;

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

	src.v6hdr = *hdr;
	src.version = bufsrc->version;
	src.flags = bufsrc->flags;
	src.wordcount = bufsrc->wordcount;
	src.srclength = be16toh(bufsrc->srclength);
	src.ext_refcode2 = be32toh(bufsrc->ext_refcode2);
	src.ext_refcode3 = be32toh(bufsrc->ext_refcode3);
	src.ext_refcode4 = be32toh(bufsrc->ext_refcode4);
	src.ext_refcode5 = be32toh(bufsrc->ext_refcode5);
	src.ext_refcode6 = be32toh(bufsrc->ext_refcode6);
	src.ext_refcode7 = be32toh(bufsrc->ext_refcode7);
	src.ext_refcode8 = be32toh(bufsrc->ext_refcode8);
	src.ext_refcode9 = be32toh(bufsrc->ext_refcode9);

	memcpy(src.primary_refcode, bufsrc->primary_refcode,
				OPAL_SRC_SCN_PRIMARY_REFCODE_LEN);

	src.fru_count = 0;
	if (src.flags & OPAL_SRC_ADD_SCN) {
		error = check_buflen(buflen, offset + sizeof(struct opal_src_add_scn_hdr), __func__);
		if (error)
			return error;

		src.addhdr.flags = bufsrc->addhdr.flags;
		src.addhdr.id = bufsrc->addhdr.id;
		if (src.addhdr.id != OPAL_FRU_SCN_ID) {
			fprintf(stderr, "%s: invalid section id, expecting 0x%x but found"
					" 0x%x", __func__, OPAL_FRU_SCN_ID, src.addhdr.id);
			return -EINVAL;
		}
		src.addhdr.length = be16toh(bufsrc->addhdr.length);
		offset += sizeof(struct opal_src_add_scn_hdr);

		while(offset < src.srclength) {
			error = parse_fru_scn(&(src.fru[src.fru_count]), buf + offset, buflen - offset);
			if (error < 0)
				return error;
			offset += error;
			src.fru_count++;
		}
	}

	print_opal_src_scn(&src);
	return 0;
}

static uint16_t from_bcd16(uint16_t bcd)
{
	return  (bcd & 0x000f) +
		((bcd & 0x00f0) >> 4) * 10 +
		((bcd & 0x0f00) >> 8) * 100 +
		((bcd & 0xf000) >> 12) * 1000;
}

static uint8_t from_bcd8(uint8_t bcd)
{
	return  (bcd & 0x0f) +
		((bcd & 0xf0) >> 4) * 10;
}

struct opal_datetime parse_opal_datetime(const struct opal_datetime in)
{
	struct opal_datetime out;

	out.year =  from_bcd16(be16toh(in.year));
	out.month = from_bcd8(in.month);
	out.day =   from_bcd8(in.day);
	out.hour =  from_bcd8(in.hour);
	out.minutes =    from_bcd8(in.minutes);
	out.seconds =    from_bcd8(in.seconds);
	out.hundredths = from_bcd8(in.hundredths);

	return out;
}

/* parse private header scn */
int parse_priv_hdr_scn(struct opal_priv_hdr_scn *privhdr,
					const struct opal_v6_hdr *hdr,
					const char *buf, int buflen)
{
	struct opal_priv_hdr_scn *bufhdr = (struct opal_priv_hdr_scn*)buf;

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

	privhdr->v6hdr = *hdr;
	privhdr->create_datetime = parse_opal_datetime(bufhdr->create_datetime);
	privhdr->commit_datetime = parse_opal_datetime(bufhdr->commit_datetime);
	privhdr->creator_id = bufhdr->creator_id;
	privhdr->scn_count = bufhdr->scn_count;

	// FIXME: are these ASCII? Need spec clarification
	privhdr->creator_subid_hi = bufhdr->creator_subid_hi;
	privhdr->creator_subid_lo = bufhdr->creator_subid_lo;

	privhdr->plid = be32toh(bufhdr->plid);

	privhdr->log_entry_id = be32toh(bufhdr->log_entry_id);

	print_opal_priv_hdr_scn(privhdr);
	return 0;
}

/* parse_usr_hdr_scn */
int parse_usr_hdr_scn(const struct opal_v6_hdr *hdr,
				const char *buf, int buflen, int *is_error)
{
	struct opal_usr_hdr_scn usrhdr;
	struct opal_usr_hdr_scn *bufhdr = (struct opal_usr_hdr_scn*)buf;

	if (buflen < sizeof(struct opal_usr_hdr_scn)) {
		fprintf(stderr, "%s: corrupted, expected length %lu, got %u\n",
			__func__,
			sizeof(struct opal_usr_hdr_scn), buflen);
		return -EINVAL;
	}

	if (hdr->length != sizeof(struct opal_usr_hdr_scn)) {
		fprintf(stderr, "%s: section header length disagrees with spec"
			". section header length %u, spec: %lu\n",
			__func__,
			hdr->length, sizeof(struct opal_usr_hdr_scn));
		return -EINVAL;
	}

	usrhdr.v6hdr = *hdr;
	usrhdr.subsystem_id = bufhdr->subsystem_id;
	usrhdr.event_data = bufhdr->event_data;
	usrhdr.event_severity = bufhdr->event_severity;
	*is_error = !usrhdr.event_severity;
	usrhdr.event_type = bufhdr->event_type;
	usrhdr.problem_domain = bufhdr->problem_domain;
	usrhdr.problem_vector = bufhdr->problem_vector;
	usrhdr.action = be16toh(bufhdr->action);

	print_opal_usr_hdr_scn(&usrhdr);
	return 0;
}

/* Extended User Header Section */
static int parse_eh_scn(struct opal_v6_hdr *hdr, const char *buf, int buflen)
{
	struct opal_eh_scn *eh;
	struct opal_eh_scn *bufeh = (struct opal_eh_scn*)buf;

	eh = (struct opal_eh_scn*) malloc(hdr->length);
	if (!eh)
		return -ENOMEM;

	if (buflen < sizeof(struct opal_eh_scn)) {
		fprintf(stderr, "%s: corrupted, expected length >= %lu, got %u\n",
			__func__,
			sizeof(struct opal_eh_scn), buflen);
		return -EINVAL;
	}

	eh->v6hdr = *hdr;
	memcpy(eh->mt.model, bufeh->mt.model, OPAL_SYS_MODEL_LEN);
	memcpy(eh->mt.serial_no, bufeh->mt.serial_no, OPAL_SYS_SERIAL_LEN);

	/* these are meant to be null terimnated strings, so should be safe */
	strcpy(eh->opal_release_version, bufeh->opal_release_version);
	strcpy(eh->opal_subsys_version, bufeh->opal_subsys_version);

	eh->event_ref_datetime = parse_opal_datetime(bufeh->event_ref_datetime);

	eh->opal_symid_len = bufeh->opal_symid_len;
	strcpy(eh->opalsymid, bufeh->opalsymid);

	print_eh_scn(eh);
	return 0;
}

/* Call Home Section */
static int parse_ch_scn(struct opal_v6_hdr *hdr, const char *buf, int buflen)
{
	struct opal_ch_scn *ch;
	struct opal_ch_scn *bufch = (struct opal_ch_scn*)buf;

	ch = (struct opal_ch_scn*) malloc(hdr->length);
	if (!ch)
		return -ENOMEM;

	if (buflen < sizeof(struct opal_ch_scn)) {
		fprintf(stderr, "%s: corrupted, expected length >= %lu, got %u\n",
			__func__,
			sizeof(struct opal_ch_scn), buflen);
		return -EINVAL;
	}

	ch->v6hdr = *hdr;

	strncpy(ch->comment, bufch->comment, OPAL_CH_COMMENT_MAX_LEN);

	print_ch_scn(ch);
	return 0;
}

static int parse_ud_scn(const struct opal_v6_hdr *hdr,
				const char *buf, int buflen)
{
	struct opal_ud_scn *ud;
	struct opal_ud_scn *bufud = (struct opal_ud_scn *)buf;

	if (buflen < sizeof(struct opal_ud_scn)) {
		fprintf(stderr, "%s: corrupted, expected length >= %lu, got %u\n",
			__func__,
			sizeof(struct opal_ud_scn), buflen);
		return -EINVAL;
	}

	ud = (struct opal_ud_scn *) malloc(hdr->length);
	if (!ud)
		return -ENOMEM;

	ud->v6hdr = *hdr;
	memcpy(ud->data, bufud->data, hdr->length - 8);
	print_ud_scn(ud);
	return 0;
}

static int parse_hm_scn(const struct opal_v6_hdr *hdr,
				 const char *buf, int buflen)
{
	struct opal_hm_scn hm;
	struct opal_hm_scn *bufhm = (struct opal_hm_scn *)buf;

	if (buflen < sizeof(struct opal_hm_scn)) {
		fprintf(stderr, "%s: corrupted, expected length >= %lu, got %u\n",
			__func__,
			sizeof(struct opal_hm_scn), buflen);
		return -EINVAL;
	}

	hm.v6hdr = *hdr;
	memcpy(hm.mt.model, bufhm->mt.model, OPAL_SYS_MODEL_LEN);
	memcpy(hm.mt.serial_no, bufhm->mt.serial_no, OPAL_SYS_SERIAL_LEN);

	print_hm_scn(&hm);
	return 0;
}

static int parse_ep_scn(const struct opal_v6_hdr *hdr,
				 const char *buf, int buflen)
{
	struct opal_ep_scn ep;
	struct opal_ep_scn *bufep = (struct opal_ep_scn *)buf;

	if (buflen < sizeof(struct opal_ep_scn)) {
		fprintf(stderr, "%s: corrupted, expected length >= %lu, got %u\n",
			__func__,
			sizeof(struct opal_ep_scn), buflen);
		return -EINVAL;
	}

	ep.v6hdr = *hdr;

	ep.value = bufep->value;
	ep.modifier = bufep->modifier;
	ep.ext_modifier = be16toh(bufep->ext_modifier);
	ep.reason = be32toh(bufep->reason);

	print_ep_scn(&ep);

	return 0;
}

static int parse_sw_scn(struct opal_v6_hdr *hdr, const char *buf, int buflen)
{
	struct opal_sw_scn *sw;
	struct opal_sw_scn *swbuf = (struct opal_sw_scn *)buf;

	sw = (struct opal_sw_scn *)malloc(hdr->length);
	sw->v6hdr = *hdr;

	if (hdr->version == 1) {
		if (buflen < sizeof(struct opal_sw_v1_scn) + sizeof(struct opal_v6_hdr)) {
			fprintf(stderr, "%s: corrupted, expected length => %lu, got %u\n",
					__func__,
					sizeof(struct opal_sw_v1_scn) + sizeof(struct opal_v6_hdr),
					buflen);
			return -EINVAL;
		}
		sw->version.v1.rc = be32toh(swbuf->version.v1.rc);
		sw->version.v1.line_num = be32toh(swbuf->version.v1.line_num);
		sw->version.v1.object_id = be32toh(swbuf->version.v1.object_id);
		sw->version.v1.id_length = swbuf->version.v1.id_length;
		strncpy(sw->version.v1.file_id, swbuf->version.v1.file_id, sw->version.v1.id_length);
	} else if (hdr->version == 2) {
		if (buflen < sizeof(struct opal_sw_v2_scn) + sizeof(struct opal_v6_hdr)) {
			fprintf(stderr, "%s: corrupted, expected length == %lu, got %u\n",
					__func__,
					sizeof(struct opal_sw_v2_scn) + sizeof(struct opal_v6_hdr),
					buflen);
			return -EINVAL;
		}
		sw->version.v2.rc = be32toh(swbuf->version.v2.rc);
		sw->version.v2.file_id = be16toh(swbuf->version.v2.file_id);
		sw->version.v2.location_id = be16toh(swbuf->version.v2.location_id);
		sw->version.v2.object_id = be32toh(swbuf->version.v2.object_id);
	} else {
		fprintf(stderr, "ERROR %s: unknown version %d\n", __func__, hdr->version);
		return -EINVAL;
	}

	print_sw_scn(sw);
	return 0;
}

static int parse_lp_scn(struct opal_v6_hdr *hdr, const char *buf, int buflen)
{
	struct opal_lp_scn *lp;
	struct opal_lp_scn *lpbuf = (struct opal_lp_scn *)buf;
	uint16_t *lps;
	uint16_t *lpsbuf;
	if (buflen < sizeof(struct opal_lp_scn)) {
		fprintf(stderr, "%s: corrupted, expected length => %lu, got %u\n",
				__func__, sizeof(struct opal_lp_scn), buflen);
		return -EINVAL;
	}

	lp = malloc(hdr->length);
	if (!lp) {
		fprintf(stderr, "%s: out of memory\n", __func__);
		return -ENOMEM;
	}

	lp->v6hdr = *hdr;
	lp->primary = be16toh(lpbuf->primary);
	lp->length_name = lpbuf->length_name;
	lp->lp_count = lpbuf->lp_count;
	lp->partition_id = be32toh(lpbuf->partition_id);
	lp->name[0] = '\0';
	if (buflen < sizeof(struct opal_lp_scn) + lp->length_name) {
		fprintf(stderr, "%s: corrupted, expected length => %lu, got %u",
				__func__, sizeof(struct opal_lp_scn) + lp->length_name,
				buflen);
		return -EINVAL;
	}
	memcpy(lp->name, lpbuf->name, lp->length_name);

	if (buflen < sizeof(struct opal_lp_scn) + lp->length_name +
			(lp->lp_count * sizeof(uint16_t))) {
		fprintf(stderr, "%s: corrupted, expected length => %lu, got %u",
				__func__, sizeof(struct opal_lp_scn) + lp->length_name +
				(lp->lp_count * sizeof(uint16_t)), buflen);
		return -EINVAL;
	}

	lpsbuf = (uint16_t *)(lpbuf->name + lpbuf->length_name);
	lps = (uint16_t *)(lp->name + lp->length_name);
	int i;
	for(i = 0; i < lp->lp_count; i++)
		lps[i] = be16toh(lpsbuf[i]);

	print_lp_scn(lp);
	return 0;
}

static int parse_lr_scn(struct opal_v6_hdr *hdr, const char *buf, int buflen)
{
	struct opal_lr_scn lr;
	struct opal_lr_scn *lrbuf = (struct opal_lr_scn *)buf;

	if (buflen < sizeof(struct opal_lr_scn)) {
		fprintf(stderr, "%s: corrupted, expected length => %lu, got %u\n",
				__func__, sizeof(struct opal_lr_scn), buflen);
		return -EINVAL;
	}

	lr.v6hdr = *hdr;
	lr.res_type = lrbuf->res_type;
	lr.capacity = be16toh(lrbuf->capacity);
	lr.shared = be32toh(lrbuf->shared);
	lr.memory_addr = be32toh(lrbuf->memory_addr);

	print_lr_scn(&lr);
	return 0;
}

static int parse_ie_scn(struct opal_v6_hdr *hdr, const char *buf, int buflen)
{
	struct opal_ie_scn ie;
	struct opal_ie_scn *iebuf = (struct opal_ie_scn *)buf;

	if (buflen < sizeof(struct opal_ie_scn) - IE_DATA_MAX) {
		fprintf(stderr, "%s: corrupted, expected length => %lu, got %u\n",
				__func__, sizeof(struct opal_ie_scn) - IE_DATA_MAX, buflen);
		return -EINVAL;
	}

	ie.v6hdr = *hdr;
	ie.type = iebuf->type;
	ie.rpc_len = iebuf->rpc_len;
	ie.scope = iebuf->scope;
	ie.subtype = iebuf->subtype;
	ie.drc = be32toh(iebuf->drc);
	if (ie.type == IE_TYPE_RPC_PASS_THROUGH) {
		if (buflen < sizeof(struct opal_ie_scn) - IE_DATA_MAX + ie.rpc_len) {
			fprintf(stderr, "%s: corrupted, exptected length => %lu, got %u",
					__func__, sizeof(struct opal_ie_scn) - IE_DATA_MAX +
						ie.rpc_len, buflen);
			return -EINVAL;
		}
		memcpy(ie.data.rpc, iebuf->data.rpc, ie.rpc_len);
	}
	if (ie.subtype == IE_SUBTYPE_PLAT_MAX_CHANGE) {
		if (buflen < sizeof(struct opal_ie_scn) - IE_DATA_MAX + sizeof(uint64_t)) {
			fprintf(stderr, "%s: corrupted, exptected length => %lu, got %u",
					__func__, sizeof(struct opal_ie_scn) - IE_DATA_MAX +
					sizeof(uint64_t), buflen);
			return -EINVAL;
		}
		ie.data.max = be64toh(iebuf->data.max);
	}

	print_ie_scn(&ie);
	return 0;
}

static int parse_mi_scn(struct opal_v6_hdr *hdr, const char *buf, int buflen)
{
	struct opal_mi_scn mi;
	struct opal_mi_scn *mibuf = (struct opal_mi_scn *)buf;

	if (buflen < sizeof(struct opal_mi_scn)) {
		fprintf(stderr, "%s: corrupted, expected length => %lu, got %u",
				__func__, sizeof(struct opal_mi_scn), buflen);
		return -EINVAL;
	}

	mi.v6hdr = *hdr;
	mi.flags = be32toh(mibuf->flags);

	print_mi_scn(&mi);
	return 0;
}

static int parse_ei_scn(struct opal_v6_hdr *hdr, const char *buf, int buflen)
{
	struct opal_ei_scn *ei;
	struct opal_ei_scn *eibuf = (struct opal_ei_scn *)buf;

	if (check_buflen(buflen, sizeof(struct opal_ei_scn), __func__) < 0 ||
			check_buflen(hdr->length, sizeof(struct opal_ei_scn), __func__) < 0)
		return -EINVAL;

	ei = (struct opal_ei_scn *) malloc(hdr->length);

	ei->v6hdr = *hdr;
	ei->g_timestamp = be64toh(eibuf->g_timestamp);
	ei->genesis.corrosion = be32toh(eibuf->genesis.corrosion);
	ei->genesis.temperature = be16toh(eibuf->genesis.temperature);
	ei->genesis.rate = be16toh(eibuf->genesis.rate);
	ei->status = eibuf->status;
	ei->user_data_scn = eibuf->user_data_scn;
	ei->read_count = be16toh(eibuf->read_count);
	if (check_buflen(hdr->length, sizeof(struct opal_ei_scn) +
				(ei->read_count * sizeof(struct opal_ei_env_scn)),
				__func__) < 0 ||
			check_buflen(buflen, sizeof(struct opal_ei_scn) +
				(ei->read_count * sizeof(struct opal_ei_env_scn)),
				__func__)) {
		free(ei);
		return -EINVAL;
	}

	int i;
	for (i = 0; i < ei->read_count; i++) {
		ei->readings[i].corrosion = be32toh(eibuf->readings[i].corrosion);
		ei->readings[i].temperature = be16toh(eibuf->readings[i].temperature);
		ei->readings[i].rate = be16toh(eibuf->readings[i].rate);
	}

	print_ei_scn(ei);
	return 0;
}

static int parse_ed_scn(struct opal_v6_hdr *hdr, const char *buf, int buflen)
{
	struct opal_ed_scn *ed;
	struct opal_ed_scn *edbuf = (struct opal_ed_scn *)buf;

	if (check_buflen(buflen, sizeof(struct opal_ed_scn), __func__) < 0 ||
			check_buflen(buflen, hdr->length, __func__) < 0 ||
			check_buflen(hdr->length, sizeof(struct opal_ed_scn), __func__) < 0)
		return -EINVAL;
	ed = (struct opal_ed_scn *) malloc(hdr->length);
	ed->v6hdr = *hdr;
	ed->creator_id = edbuf->creator_id;
	memcpy(ed->user_data, edbuf->user_data, hdr->length - 12);

	return 0;
}

static int parse_section_header(struct opal_v6_hdr *hdr, const char *buf, int buflen)
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
			__func__,
			hdr->id[0], hdr->id[1],
			hdr->length, hdr->version, hdr->subtype,
			hdr->component_id);
		return -EINVAL;
	}

	return 0;
}

int header_id_lookup(char* id) {
	int i;
	for (i = 0; i < HEADER_ORDER_MAX; i++)
		if (strncmp(id,elog_hdr_id[i].id,2) == 0)
			return i;
	return -1;
}

/* parse all required sections of the log */
int parse_opal_event(char *buf, int buflen)
{
	int rc;
	struct opal_v6_hdr hdr;
	struct opal_priv_hdr_scn ph;
	int header_pos;
	struct header_id *hdr_data;
	char *start = buf;
	int nrsections = 0;
	int is_error = 0;
	int i;

	while (buflen) {
		rc = parse_section_header(&hdr, buf, buflen);
		if (rc < 0)
			return rc;

		header_pos = header_id_lookup(hdr.id);
		if (header_pos == -1) {
				printf("Unknown section header: %c%c at %lu:\n",
						hdr.id[0], hdr.id[1], buf-start);
				printf("Length: %u (incl 8 byte header)\n", hdr.length);
				printf("Hex:\n");
				for (i = 8; i < hdr.length; i++) {
					printf("0x%02x ", *(buf+i));
					if (i % 16)
						printf("\n");
				}
				printf("Text (. = unprintable):\n");
				for (i = 8; i < hdr.length; i++) {
					printf("%c",
							(isgraph(*(buf+i)) | isspace(*(buf+i))) ?
							*(buf+i) : '.');
				}
		}

		hdr_data = &elog_hdr_id[header_pos];

		nrsections++;

		if (hdr_data->pos != 0 && hdr_data->pos != nrsections &&
				((hdr_data->req & HEADER_REQ) ||
				((hdr_data->req & HEADER_REQ_W_ERROR) && is_error))) {
				fprintf(stderr, "ERROR %s: Section number %d should be "
						"%s, instead is 0x%02x%02x (%c%c)\n",
						__func__, nrsections, hdr_data->id,
						hdr.id[0], hdr.id[1], hdr.id[0], hdr.id[1]);
			rc = -1;
			break;
		}

		if (hdr_data->max == 0) {
			fprintf(stderr, "ERROR %s: Section %s has already appeared the "
					"required times and should not be seen again\n", __func__,
					hdr_data->id);
		} else if (hdr_data->max > 0) {
			hdr_data->max--;
		}

		if (strncmp(hdr.id, "PH", 2) == 0) {
			parse_priv_hdr_scn(&ph, &hdr, buf, buflen);
		} else if (strncmp(hdr.id, "UH", 2) == 0) {
			parse_usr_hdr_scn(&hdr, buf, buflen, &is_error);
		} else if (strncmp(hdr.id, "PS", 2) == 0) {
			parse_src_scn(&hdr, buf, buflen);
		} else if (strncmp(hdr.id, "EH", 2) == 0) {
			parse_eh_scn(&hdr, buf, buflen);
		} else if (strncmp(hdr.id, "MT", 2) == 0) {
			parse_mt_scn(&hdr, buf, buflen);
		} else if (strncmp(hdr.id, "SS", 2) == 0) {
			parse_src_scn(&hdr, buf, buflen);
		} else if (strncmp(hdr.id, "DH", 2) == 0) { // FIXME
		} else if (strncmp(hdr.id, "SW", 2) == 0) {
			parse_sw_scn(&hdr, buf, buflen);
		} else if (strncmp(hdr.id, "LP", 2) == 0) {
			parse_lp_scn(&hdr, buf, buflen);
		} else if (strncmp(hdr.id, "LR", 2) == 0) {
			parse_lr_scn(&hdr, buf, buflen);
		} else if (strncmp(hdr.id, "HM", 2) == 0) {
			parse_hm_scn(&hdr, buf, buflen);
		} else if (strncmp(hdr.id, "EP", 2) == 0) {
			parse_ep_scn(&hdr, buf, buflen);
		} else if (strncmp(hdr.id, "IE", 2) == 0) {
			parse_ie_scn(&hdr, buf, buflen);
		} else if (strncmp(hdr.id, "MI", 2) == 0) {
			parse_mi_scn(&hdr, buf, buflen);
		} else if (strncmp(hdr.id, "CH", 2) == 0) {
			parse_ch_scn(&hdr, buf, buflen);
		} else if (strncmp(hdr.id, "UD", 2) == 0) {
			parse_ud_scn(&hdr, buf, buflen);
		} else if (strncmp(hdr.id, "EI", 2) == 0) {
			parse_ei_scn(&hdr, buf, buflen);
		} else if (strncmp(hdr.id, "ED", 2) == 0) {
			parse_ed_scn(&hdr, buf, buflen);
		}

		buf+= hdr.length;

		if (nrsections == ph.scn_count)
			break;
	}

	for (i = 0; i < HEADER_ORDER_MAX; i++) {
		if (((elog_hdr_id[i].req & HEADER_REQ) ||
					((elog_hdr_id[i].req & HEADER_REQ_W_ERROR) && is_error))
				&& elog_hdr_id[i].max != 0) {
			fprintf(stderr,"ERROR %s: Truncated error log, expected section %s"
					" not found\n", __func__, elog_hdr_id[i].id);
		}
	}
	return rc;
}
