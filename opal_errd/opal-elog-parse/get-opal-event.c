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

static int print_bar(void)
{
	int i;
	putchar('|');
	for(i = 0; i < LINE_LENGTH-3; i++)
		putchar('-');
	printf("|\n");
	return LINE_LENGTH;
}

static int print_header(const char *header)
{
	int len = strlen(header);
	int i;

	print_bar();

	putchar('|');

	i = 0;
	while (i < (LINE_LENGTH - 3 - len)/2) {
		putchar(' ');
		i++;
	}

	printf("%s", header);

	i = 0;
	while(i < (LINE_LENGTH - 3 - len) / 2) {
		putchar(' ');
		i++;
	}

	if ((LINE_LENGTH - 2 - len) % 2 == 0)
		putchar(' ');

	printf("|\n");

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
	print_line("Section Version", "%d (%c%c)", usrhdr->v6hdr.version,
	       usrhdr->v6hdr.id[0],  usrhdr->v6hdr.id[1]);
	print_line("Sub-section type", "%d", usrhdr->v6hdr.subtype);
	print_line("Component ID", "0x%x", usrhdr->v6hdr.component_id);
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

int print_mt_scn(struct opal_mtms_scn *mtms)
{
	char model[OPAL_SYS_MODEL_LEN+1];
	char serial_no[OPAL_SYS_SERIAL_LEN+1];

	memcpy(model, mtms->mt.model, OPAL_SYS_MODEL_LEN);
	model[OPAL_SYS_MODEL_LEN] = '\0';
	memcpy(serial_no, mtms->mt.serial_no, OPAL_SYS_SERIAL_LEN);
	model[OPAL_SYS_SERIAL_LEN] = '\0';

	printf("|-------------------------------------------------------------|\n");
	printf("|              Machine Type/Model & Serial Number	      |\n");
	printf("|-------------------------------------------------------------|\n");
	printf("Section ID		: %c%c\n",
	       mtms->v6hdr.id[0], mtms->v6hdr.id[1]);
	printf("Section Length		: %x\n", mtms->v6hdr.length);
	printf("Version			: %x\n", mtms->v6hdr.version);
	printf("Sub_type		: %x\n", mtms->v6hdr.subtype);
	printf("Component ID		: %x\n", mtms->v6hdr.component_id);
	printf("Machine Type Model	: %s\n", model);
	printf("Serial Number		: %s\n", serial_no);
	printf("|-------------------------------------------------------------|\n\n");
	return 0;
}

int print_opal_src_scn(struct opal_src_scn *src)
{
	print_header("Primary System Reference Code");
	print_line("Section Version","%d (%c%c)", src->v6hdr.version,
	       src->v6hdr.id[0],  src->v6hdr.id[1]);
	print_line("Sub-section type", "%d", src->v6hdr.subtype);
	print_line("SRC Format", "0x%x", src->flags);
	print_line("SRC Version", "0x%x", src->version);
	print_line("Valid Word Count", "0x%x", src->wordcount);
	print_line("SRC Length", "%x", src->srclength);
	print_src_refcode(src);
	return 0;
}

int print_opal_usr_hdr_scn(struct opal_usr_hdr_scn *usrhdr)
{
	print_usr_hdr_subsystem_id(usrhdr);
	print_usr_hdr_event_data(usrhdr);
	print_usr_hdr_action(usrhdr);
	return 0;
}

int print_opal_priv_hdr_scn(struct opal_priv_hdr_scn *privhdr)
{
	int i;
	print_header("Private Header");
	print_line("Section Version", "%d (%c%c)", privhdr->v6hdr.version,
					privhdr->v6hdr.id[0], privhdr->v6hdr.id[1]);
	print_line("Sub-section type", "0x%x", privhdr->v6hdr.subtype);

	print_line("Component ID", "%x", privhdr->v6hdr.component_id);
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
	for (i = 0; i < MAX_CREATORS; i++) {
		if (privhdr->creator_id == prv_hdr_creator_id[i].id) {
			print_line("Created by", "%s", prv_hdr_creator_id[i].name);
			break;
		}
	}
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

	printf("|-------------------------------------------------------------|\n");
	printf("|                   Extended User Header                      |\n");
	printf("|-------------------------------------------------------------|\n");
	printf("Section ID		: %c%c\n",
	       eh->v6hdr.id[0], eh->v6hdr.id[1]);
	printf("Section Length		: %x\n", eh->v6hdr.length);
	printf("Version			: %x\n", eh->v6hdr.version);
	printf("Sub_type		: %x\n", eh->v6hdr.subtype);
	printf("Component ID		: %x\n", eh->v6hdr.component_id);
	printf("Machine Type Model	: %s\n", model);
	printf("Serial Number		: %s\n", serial_no);
	printf("Server Firmware Release Version : %s\n",
	       eh->opal_release_version);
	printf("Firmware subsystem Driver Version : %s\n",
	       eh->opal_subsys_version);
	printf("Event Common reference Time (UTC): "
	       "%4u-%02u-%02u | %02u:%02u:%02u\n",
	       eh->event_ref_datetime.year,
	       eh->event_ref_datetime.month,
	       eh->event_ref_datetime.day,
	       eh->event_ref_datetime.hour,
	       eh->event_ref_datetime.minutes,
	       eh->event_ref_datetime.seconds);
	if (eh->opal_symid_len)
		printf("Symptom ID             : %s\n", eh->opalsymid);

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

/* parse SRC section of the log */
int parse_src_scn(const struct opal_v6_hdr *hdr,
		  const char *buf, int buflen)
{
	struct opal_src_scn src;
	struct opal_src_scn *bufsrc = (struct opal_src_scn*)buf;

	if (buflen < sizeof(struct opal_src_scn)) {
		fprintf(stderr, "%s: corrupted, expected length %lu, got %u\n",
			__func__,
			sizeof(struct opal_src_scn), buflen);
		return -EINVAL;
	}

	/* header length can be > sizeof() as is variable sized section */
	if (hdr->length < sizeof(struct opal_src_scn)) {
		fprintf(stderr, "%s: section header length less than min size "
			". section header length %u, min size: %lu\n",
			__func__,
			hdr->length, sizeof(struct opal_src_scn));
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
		      const char *buf, int buflen)
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

/* parse all required sections of the log */
int parse_opal_event(char *buf, int buflen)
{
	int rc;
	struct opal_v6_hdr hdr;
	struct opal_priv_hdr_scn ph;
	int i;
	char *start = buf;
	int nrsections = 0;

	while (buflen) {
		rc = parse_section_header(&hdr, buf, buflen);
		if (rc < 0)
			return rc;

		if (nrsections == 0 && strncmp(hdr.id, "PH", 2)) {
			fprintf(stderr, "ERROR %s: First section should be "
				"platform header, instead is 0x%02x%02x\n",
				__func__, hdr.id[0], hdr.id[1]);
			rc = -1;
			break;
		}

		nrsections++;

		if (strncmp(hdr.id, "PH", 2) == 0) {
			parse_priv_hdr_scn(&ph, &hdr, buf, buflen);
		} else if (strncmp(hdr.id, "UH", 2) == 0) {
			// fixme required
			parse_usr_hdr_scn(&hdr, buf, buflen);
		} else if (strncmp(hdr.id, "PS", 2) == 0) {
			// FIXME sometimes required
			parse_src_scn(&hdr, buf, buflen);
		} else if (strncmp(hdr.id, "EH", 2) == 0) {
			parse_eh_scn(&hdr, buf, buflen);
		} else if (strncmp(hdr.id, "MT", 2) == 0) {
			// FIXME: required
			parse_mt_scn(&hdr, buf, buflen);
		} else if (strncmp(hdr.id, "SS", 2) == 0) { // FIXME
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
		} else if (strncmp(hdr.id, "UD", 2) == 0) { // FIXME
		} else if (strncmp(hdr.id, "EI", 2) == 0) { // FIXME
		} else if (strncmp(hdr.id, "ED", 2) == 0) { // FIXME
		} else {
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

		buf+= hdr.length;

		if (nrsections == ph.scn_count)
			break;
	}

	return 0;
}
