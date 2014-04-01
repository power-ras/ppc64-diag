#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <ctype.h>
#include <assert.h>
#include "libopalevents.h"

#define LINE_LENGTH	81
#define TITLE_LENGTH	29
#define ARG_LENGTH	(LINE_LENGTH - TITLE_LENGTH)

struct id_msg usr_hdr_event_type[] =  {
	EVENT_TYPE
};

struct sev_type usr_hdr_severity[] = {
	SEVERITY_TYPE
};

struct subsystem_id usr_hdr_subsystem_id[] = {
	SUBSYSTEMS
};

struct creator_id prv_hdr_creator_id[] = {
	CREATORS
};

struct event_scope usr_hdr_event_scope[] = {
	EVENT_SCOPE
};

struct header_id elog_hdr_id[] = {
	HEADER_ORDER
};

struct generic_desc fru_scn_priority[] = {
	FRU_PRIORITY
};

struct generic_desc fru_id_scn_component[] = {
	FRU_ID_COMPONENT
};

static int print_bar(void)
{
	int i;
	putchar('|');
	for(i = 0; i < LINE_LENGTH-3; i++)
		putchar('-');
	printf("|\n");
	return LINE_LENGTH;
}

static int print_center(const char *output)
{
	int len = strlen(output);
	int i;

	putchar('|');

	i = 0;
	while (i < (LINE_LENGTH - 3 - len)/2) {
		putchar(' ');
		i++;
	}

	printf("%s", output);

	i = 0;
	while(i < (LINE_LENGTH - 3 - len) / 2) {
		putchar(' ');
		i++;
	}

	if ((LINE_LENGTH - 2 - len) % 2 == 0)
		putchar(' ');

	printf("|\n");

	return 0;
}

static int print_header(const char *header)
{
	print_bar();

	print_center(header);

	print_bar();

	return 0;
}

static int print_line(char *entry, const char *format, ...)
	__attribute__ ((format (printf, 2, 3)));
static int print_line(char *entry, const char *format, ...)
{
	va_list args;
	int written;
	int title;
	char buf[ARG_LENGTH];

	if(strlen(entry) > LINE_LENGTH - 6)
		entry[LINE_LENGTH - 6] = '\0';

	title = printf("| %-25s: ", entry);
	if (title < TITLE_LENGTH)
		return title;

	va_start(args, format);
	written = vsnprintf(buf, LINE_LENGTH - title - 1, format, args);
	va_end(args);
	if (written < LINE_LENGTH - title && written >= 0) {
		/* Didn't overflow the 80 character per line limit */
		written = title;
		written += printf("%s", buf);
		while (written < LINE_LENGTH - 2) {
			putchar(' ');
			written++;
		}
		written += printf("|\n");
	} else if (written > 0) {
		/* Did overflow the 80 character per line limit, print the rest on
		* the next line(s)
		*/
		char *remainder = malloc((written + 1) * sizeof(char));
		va_start(args, format);
		vsnprintf(remainder, written + 1, format, args);
		va_end(args);
		written = title;
		written += printf("%s|\n",buf);
		int i = ARG_LENGTH - 2;
		written += printf("|%*c: ",TITLE_LENGTH-3,' ');
		while (remainder[i] != '\0') {
			putchar(remainder[i]);
			i++;
			written++;
			if ((written + 2) % (LINE_LENGTH) == 0) {
				/* Swallow leading spaces */
				while(remainder[i] == ' ') {
					i++;
				}
				/* Only print this goodness if there is another line */
				if(remainder[i] != '\0') {
					written += printf("|\n");
					written += printf("|%*c: ",TITLE_LENGTH-3,' ');
				}
			}
		}

		while ((written + 2) % LINE_LENGTH) {
			putchar(' ');
			written++;
		}
		written += printf("|\n");
		free(remainder);
	}

	return written;
}

static int print_opal_v6_hdr(struct opal_v6_hdr hdr) {
	print_line("Section Version", "%d (%c%c)", hdr.version,
					hdr.id[0], hdr.id[1]);
	print_line("Sub-section type", "0x%x", hdr.subtype);
	print_line("Section Length", "0x%x", hdr.length);
	print_line("Component ID", "%x", hdr.component_id);
	return 0;
}

int print_usr_hdr_action(struct opal_usr_hdr_scn *usrhdr)
{
	char *entry = "Action Flags";

	if (usrhdr->action & OPAL_UH_ACTION_REPORT_EXTERNALLY) {
		if (usrhdr->action & OPAL_UH_ACTION_HMC_ONLY)
			print_line(entry,"Report to HMC");
		else
			print_line(entry, "Report to Operating System");
		entry = "";
	}

	if (usrhdr->action & OPAL_UH_ACTION_SERVICE) {
		print_line(entry, "Service Action Required");
		if (usrhdr->action & OPAL_UH_ACTION_CALL_HOME)
			print_line("","HMC Call Home");
		if(usrhdr->action & OPAL_UH_ACTION_TERMINATION)
			print_line("","Termination Error");
		entry = "";
	}

	if (usrhdr->action & OPAL_UH_ACTION_ISO_INCOMPLETE) {
		print_line(entry, "Error isolation incomplete");
		print_line("","Further analysis required");
		entry = "";
	}

	if(entry[0] != '\0') {
		print_line(entry, "Unknown action flag (0x%08x)", usrhdr->action);
	}

	return 0;
}

static int get_field_desc(struct generic_desc *data, int length, int id, int default_id, int default_index) {
	int i;
	int to_print = default_index;
	for (i = 0; i < length; i++) {
		if (id == data[i].id) {
			return i;
		} else if(default_id == usr_hdr_subsystem_id[i].id) {
			to_print = i;
		}
	}
	return to_print;
}

int print_usr_hdr_event_data(struct opal_usr_hdr_scn *usrhdr)
{
	int to_print;

	to_print = get_field_desc((struct generic_desc *)usr_hdr_event_scope, MAX_EVENT_SCOPE, usrhdr->event_data, usrhdr->event_data, -1);
	if (to_print >= 0)
		print_line("Event Scope", "%s", usr_hdr_event_scope[to_print].desc);

	to_print = get_field_desc((struct generic_desc *)usr_hdr_severity, MAX_SEV, usrhdr->event_severity, usrhdr->event_severity & 0xF0, 0);
	print_line("Event Severity", "%s", usr_hdr_severity[to_print].desc);

	to_print = get_field_desc((struct generic_desc *)usr_hdr_event_type, MAX_EVENT, usrhdr->event_type, usrhdr->event_type, -1);
	if (to_print >= 0)
		print_line("Event Type", "%s", usr_hdr_event_type[to_print].msg);

	return 0;
}

int print_usr_hdr_subsystem_id(struct opal_usr_hdr_scn *usrhdr)
{
	int to_print;
	unsigned int id = usrhdr->subsystem_id;
	print_header("User Header");
	print_opal_v6_hdr(usrhdr->v6hdr);
	to_print = get_field_desc((struct generic_desc *)usr_hdr_subsystem_id,
						              MAX_SUBSYSTEMS, id, id & 0xF0, 0);
	print_line("Subsystem", "%s", usr_hdr_subsystem_id[to_print].name);

	return 0;
}

int print_src_refcode(struct opal_src_scn *src)
{
	char primary_refcode_display[OPAL_SRC_SCN_PRIMARY_REFCODE_LEN+1];

	memcpy(primary_refcode_display, src->primary_refcode,
				 OPAL_SRC_SCN_PRIMARY_REFCODE_LEN);
	primary_refcode_display[OPAL_SRC_SCN_PRIMARY_REFCODE_LEN] = '\0';

	print_line("Primary Reference Code", "%s", primary_refcode_display);
	print_line("Hex Words 2 - 5", "0x%08x 0x%08x 0x%08x 0x%08x",
				 src->ext_refcode2, src->ext_refcode3,
				 src->ext_refcode4, src->ext_refcode5);
	print_line("Hex Words 6 - 9", "0x%08x 0x%08x 0x%08x 0x%08x",
				 src->ext_refcode6, src->ext_refcode7,
				 src->ext_refcode8, src->ext_refcode9);
	return 0;
}

int print_mt_data(struct opal_mt_struct mt) {
	char model[OPAL_SYS_MODEL_LEN+1];
	char serial_no[OPAL_SYS_SERIAL_LEN+1];

	memcpy(model, mt.model, OPAL_SYS_MODEL_LEN);
	model[OPAL_SYS_MODEL_LEN] = '\0';
	memcpy(serial_no, mt.serial_no, OPAL_SYS_SERIAL_LEN);
	model[OPAL_SYS_SERIAL_LEN] = '\0';

	print_line("Machine Type Model", "%s", model);
	print_line("Serial Number", "%s", serial_no);

	return 0;
}

int print_mt_scn(struct opal_mtms_scn *mtms)
{

	print_header("Machine Type/Model & Serial Number");
	print_opal_v6_hdr(mtms->v6hdr);
	print_mt_data(mtms->mt);
	return 0;
}

int print_fru_id_scn(struct opal_fru_id_sub_scn id) {
	int to_print;

	print_center(" ");
	to_print = get_field_desc(fru_id_scn_component, MAX_FRU_ID_COMPONENT, 
			id.hdr.flags & 0xF0, 0, -1);
	print_center(fru_id_scn_component[to_print].desc);
	if (id.hdr.flags & OPAL_FRU_ID_PART)
		print_line("Part Number", "%s", id.part);

	if (id.hdr.flags & OPAL_FRU_ID_PROC)
		print_line("Procedure ID", "%s", id.part);

	if (id.hdr.flags & OPAL_FRU_ID_CCIN)
		print_line("CCIN", "%c%c%c%c", id.ccin[0], id.ccin[1],
				id.ccin[2], id.ccin[3]);
	
	if (id.hdr.flags & OPAL_FRU_ID_SERIAL) {
		char tmp[OPAL_FRU_ID_SERIAL_MAX+1];
		memcpy(tmp, id.serial, OPAL_FRU_ID_SERIAL);
		tmp[OPAL_FRU_ID_SERIAL] = '\0';
		print_line("Serial Number", "%s", tmp);
	}

	return 0;
}

int print_fru_pe_scn(struct opal_fru_pe_sub_scn pe) {
	print_mt_data(pe.mtms);
	print_line("PCE", "%s", pe.pce);
	return 0;
}

int print_fru_mr_scn(struct opal_fru_mr_sub_scn mr) {

	int total_mru = mr.hdr.flags & 0x0F;
	int i;
	int to_print;
	for (i = 0; i < total_mru; i++) {
		print_line("MRU ID", "0x%x", mr.mru[i].id);

		to_print = get_field_desc(fru_scn_priority, MAX_FRU_PRIORITY, mr.mru[i].priority, 'L', 0);
		print_line("Priority", "%s", fru_scn_priority[to_print].desc);
	}
	return 0;
}

int print_fru_scn(struct opal_fru_scn fru) {
	int to_print;
	/* FIXME This printing was to roughly confirm the correctness
	 * of the parsing. Improve here.
	 */

		
	to_print = get_field_desc(fru_scn_priority, MAX_FRU_PRIORITY, fru.priority, 'L', 0);
	print_line("Priority", "%s", fru_scn_priority[to_print].desc);
	print_line("Location Code","%s", fru.location_code);
	if (fru.type & OPAL_FRU_ID_SUB) {
		print_fru_id_scn(fru.id);
	}

	if(fru.type & OPAL_FRU_PE_SUB) {
		print_fru_pe_scn(fru.pe);
	}

	if(fru.type & OPAL_FRU_MR_SUB) {
		print_fru_mr_scn(fru.mr);
	}

	return 0;
}

int print_opal_src_scn(struct opal_src_scn *src)
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

int print_opal_usr_hdr_scn(struct opal_usr_hdr_scn *usrhdr)
{
	print_usr_hdr_subsystem_id(usrhdr);
	print_usr_hdr_event_data(usrhdr);
	print_usr_hdr_action(usrhdr);
	return 0;
}


const char *get_creator_name(uint8_t creator_id)
{
	int i;
	for (i = 0; i < MAX_CREATORS; i++) {
		if (creator_id != prv_hdr_creator_id[i].id)
			continue;
		return prv_hdr_creator_id[i].name;
	}
	return "Unknown";
}

int print_opal_priv_hdr_scn(struct opal_priv_hdr_scn *privhdr)
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

int print_eh_scn(struct opal_eh_scn *eh)
{
	char model[OPAL_SYS_MODEL_LEN+1];
	char serial_no[OPAL_SYS_SERIAL_LEN+1];

	memcpy(model, eh->mt.model, OPAL_SYS_MODEL_LEN);
	model[OPAL_SYS_MODEL_LEN] = '\0';
	memcpy(serial_no, eh->mt.serial_no, OPAL_SYS_SERIAL_LEN);
	model[OPAL_SYS_SERIAL_LEN] = '\0';

	print_header("Extended User Header");
	print_line("Version", "%d (%c%c)", eh->v6hdr.version,
				 eh->v6hdr.id[0], eh->v6hdr.id[1]);
	print_line("Sub-section type", "%d", eh->v6hdr.subtype);
	print_line("Component ID", "%x", eh->v6hdr.component_id);
	print_line("Reporting Machine Type", "%s", model);
	print_line("Reporting Serial Number", "%s", serial_no);
	print_line("FW Released Ver", "%s",
				 eh->opal_release_version);
	print_line("FW SubSys Version", "%s",
				 eh->opal_subsys_version);
	print_line("Common Ref Time (UTC)",
				 "%4u-%02u-%02u | %02u:%02u:%02u",
				 eh->event_ref_datetime.year,
				 eh->event_ref_datetime.month,
				 eh->event_ref_datetime.day,
				 eh->event_ref_datetime.hour,
				 eh->event_ref_datetime.minutes,
				 eh->event_ref_datetime.seconds);
	print_line("Symptom Id Len", "%d", eh->opal_symid_len);
	if (eh->opal_symid_len)
		print_line("Symptom Id", "%s", eh->opalsymid);

	return 0;
}

static int print_ch_scn(struct opal_ch_scn *ch)
{
	printf("|-------------------------------------------------------------|\n");
	printf("|                   Call Home Log Comment                     |\n");
	printf("|-------------------------------------------------------------|\n");
	printf("Section ID		: %c%c\n",
				 ch->v6hdr.id[0], ch->v6hdr.id[1]);
	printf("Section Length		: %x\n", ch->v6hdr.length);
	printf("Version			: %x\n", ch->v6hdr.version);
	printf("Sub_type		: %x\n", ch->v6hdr.subtype);
	printf("Component ID		: %x\n", ch->v6hdr.component_id);
	printf("Call Home Comment	: %s\n", ch->comment);

	return 0;
}

int print_ud_scn(struct opal_ud_scn * ud)
{
	print_header("User Defined Data");
	print_opal_v6_hdr(ud->v6hdr);
	int i = 0;
	int written = 0;
	/*FIXME this data should be parsable if documentation appears/exists
	 * In the mean time, just dump it in hex
	 */ 
	print_line("User data hex","length %d",ud->v6hdr.length - 8);
	while(i < (ud->v6hdr.length - 8)) {
		if(written % LINE_LENGTH == 0)
			written = printf("| ");

			written += printf("%.2x",ud->data[i]);

		if((written + 2) % LINE_LENGTH == 0) {
			written += printf("|\n");
		} else {
			putchar(' ');
			written++;
		}

		i++;
	}
	while ((written + 2) % LINE_LENGTH) {
		putchar(' ');
		written++;
	}
	printf("|\n");
	return 0;
}

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
	 */
	/* The minimum we need is the start of the opan_fru_src_scn struct */
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
		} else if (strncmp(hdr.id, "HM", 2) == 0) { // FIXME
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
