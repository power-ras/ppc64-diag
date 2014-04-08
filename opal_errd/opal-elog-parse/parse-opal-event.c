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
	int min_length = sizeof(struct opal_src_add_scn_hdr) + (sizeof(uint8_t) * 4);
	int error = check_buflen(buflen, min_length, __func__);
	if (error)
		return error;

	fru_scn->hdr.flags = bufsrc->hdr.flags;
	fru_scn->hdr.id = bufsrc->hdr.id;
	if (fru_scn->hdr.id != OPAL_FRU_SCN_ID) {
		fprintf(stderr, "%s: invalid section id, expecting 0x%x but found"
				" 0x%x", __func__, OPAL_FRU_SCN_ID, fru_scn->hdr.id);
		return -EINVAL;
	}
	//FIXME It is unclear what length hdr.length is talking about
	fru_scn->hdr.length = be16toh(bufsrc->hdr.length);
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

	min_length += bufsrc->loc_code_len;
	error = check_buflen(buflen, min_length, __func__);
	if (error)
		return error;

	memcpy(fru_scn->location_code, bufsrc->location_code, fru_scn->loc_code_len);

	if (fru_scn->type & OPAL_FRU_ID_SUB) {
		min_length += parse_src_fru_id_scn(&(fru_scn->id), buf + min_length, buflen - min_length);
		if (error)
			return error;
	}

	if (fru_scn->type & OPAL_FRU_PE_SUB) {
		min_length += parse_src_fru_pe_scn(&(fru_scn->pe), buf + min_length, buflen - min_length);
		if (error)
			return error;
	}

	if (fru_scn->type & OPAL_FRU_MR_SUB) {
		min_length += parse_src_fru_mr_scn(&(fru_scn->mr), buf + min_length, buflen - min_length);
		if (error)
			return error;
	}

	if (min_length - sizeof(struct opal_src_add_scn_hdr) != fru_scn->length) {
		fprintf(stderr, "%s: Parsed amount of data does not match the header length"
					" parsed: %lu vs expected: %d\n", __func__,
					min_length - sizeof(struct opal_src_add_scn_hdr), fru_scn->length);
		return -EINVAL;
	}
	return (fru_scn->hdr.flags & OPAL_FRU_MORE) ? min_length : 0;
}
/* parse SRC section of the log */
int parse_src_scn(const struct opal_v6_hdr *hdr,
			const char *buf, int buflen)
{
	struct opal_src_scn src;
	struct opal_src_scn *bufsrc = (struct opal_src_scn*)buf;

	int offset = sizeof(struct opal_src_scn) -
		(sizeof(struct opal_fru_scn) * OPAL_SRC_FRU_MAX) -
		sizeof(uint8_t);

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
	if (!(hdr->length > offset))
		offset = 0;
	while(offset) {
		error = parse_fru_scn(&(src.fru[src.fru_count]), buf + offset, buflen - offset);
		if (error < 0)
			return error;
		else if (error > 0)
			offset += error;
		else
			offset = 0;
		src.fru_count++;
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

static int parse_section_header(struct opal_v6_hdr *hdr, const char *buf, int buflen)
{
	if (buflen < 8) {
		fprintf(stderr, "ERROR %s: section header is too small, "
			"is meant to be 8 bytes, only %u found.\n",
			__func__, buflen);
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
		} else if (strncmp(hdr.id, "SW", 2) == 0) { // FIXME
		} else if (strncmp(hdr.id, "LP", 2) == 0) { // FIXME
		} else if (strncmp(hdr.id, "LR", 2) == 0) { // FIXME
		} else if (strncmp(hdr.id, "HM", 2) == 0) {
			parse_hm_scn(&hdr, buf, buflen);
		} else if (strncmp(hdr.id, "EP", 2) == 0) { // FIXME
		} else if (strncmp(hdr.id, "IE", 2) == 0) { // FIXME
		} else if (strncmp(hdr.id, "MI", 2) == 0) { // FIXME
		} else if (strncmp(hdr.id, "CH", 2) == 0) {
			parse_ch_scn(&hdr, buf, buflen);
		} else if (strncmp(hdr.id, "UD", 2) == 0) {
			parse_ud_scn(&hdr, buf, buflen);
		} else if (strncmp(hdr.id, "EI", 2) == 0) { // FIXME
		} else if (strncmp(hdr.id, "ED", 2) == 0) { // FIXME
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
