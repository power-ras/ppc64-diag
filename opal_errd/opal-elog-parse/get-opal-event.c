#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <ctype.h>
#include <assert.h>
#include "libopalevents.h"

struct id_msg usr_hdr_event_type[] =  {
	EVENT_TYPE
};

struct sev_type usr_hdr_severity[] = {
	SEVERITY_TYPE
};


int print_usr_hdr_action(struct opal_usr_hdr_scn *usrhdr)
{
	printf("Action Flag		:");
	switch (usrhdr->action) {
	case 0xa800:
		printf(" Service Action");
		if (usrhdr->action & 0x4000)
			printf(" hidden error");
		if (usrhdr->action & 0x0800)
			printf(" call home");
		printf(" Required.\n");
		break;
	case 0x2000:
		printf(" Report Externally, ");
		if (usrhdr->action & 0x1000)
			printf("HMC only\n");
		else
			printf("HMC and Hypervisor\n");
		break;
	case 0x0400:
		printf(" Error isolation incomplete,\n"
		       "further analysis required.\n");
		break;
	case 0x0000:
		break;
	default:
		printf(" Unknown action flag (0x%08x).\n", usrhdr->action);
	}
	return 0;
}

int print_usr_hdr_event_data(struct opal_usr_hdr_scn *usrhdr)
{
	int i;
	printf("Event Data		: %x\n", usrhdr->event_data);
	printf("Event Type		:");

	for (i = 0; i < MAX_EVENT; i++) {
		if (usrhdr->event_type == usr_hdr_event_type[i].id) {
			printf("%s\n", usr_hdr_event_type[i].msg);
			break;
		}
	}
	printf("Event Severity		:");
	for (i = 0; i < MAX_SEV; i++) {
		if (usrhdr->event_severity == usr_hdr_severity[i].sev) {
			printf("%s\n", usr_hdr_severity[i].desc);
			break;
		}
	}
	return 0;
}

int print_usr_hdr_subsystem_id(struct opal_usr_hdr_scn *usrhdr)
{
	unsigned int id = usrhdr->subsystem_id;
	printf("|-------------------------------------------------------------|\n");
	printf("|                      User Header                            |\n");
	printf("|-------------------------------------------------------------|\n");
	printf("Section ID		: %c%c\n",
	       usrhdr->v6hdr.id[0], usrhdr->v6hdr.id[1]);
	printf("Section Length		: %x\n", usrhdr->v6hdr.length);
	printf("Version			: %x\n", usrhdr->v6hdr.version);
	printf("Sub_type		: %x\n", usrhdr->v6hdr.subtype);
	printf("Component ID		: %x\n", usrhdr->v6hdr.component_id);
	printf("Subsystem ID		:");
	if ((id >= 0x10) && (id <= 0x1F))
		printf(" Processor, including internal cache\n");
	else if ((id >= 0x20) && (id <= 0x2F))
		printf(" Memory, including external cache\n");
	else if ((id >= 0x30) && (id <= 0x3F))
		printf(" I/O hub, bridge, bus\n");
	else if ((id >= 0x40) && (id <= 0x4F))
		printf(" I/O adapter, device and peripheral\n");
	else if ((id >= 0x50) && (id <= 0x5F))
		printf(" CEC Hardware\n");
	else if ((id >= 0x60) && (id <= 0x6F))
		printf(" Power/Cooling System\n");
	else if ((id >= 0x70) && (id <= 0x79))
		printf(" Other Subsystems\n");
	else if ((id >= 0x7A) && (id <= 0x7F))
		printf(" Surveillance Error\n");
	else if ((id >= 0x80) && (id <= 0x8F))
		printf(" Platform Firmware\n");
	else if ((id >= 0x90) && (id <= 0x9F))
		printf(" Software)\n");
	else if ((id >= 0xA0) && (id <= 0xAF))
		printf(" External Environment\n");
	else
		printf("\n");
	return 0;
}

int print_src_refcode(struct opal_src_scn *src)
{
	char primary_refcode_display[OPAL_SRC_SCN_PRIMARY_REFCODE_LEN+1];

	memcpy(primary_refcode_display, src->primary_refcode,
	       OPAL_SRC_SCN_PRIMARY_REFCODE_LEN);
	primary_refcode_display[OPAL_SRC_SCN_PRIMARY_REFCODE_LEN] = '\0';

	printf("Primary Reference Code	: %s", primary_refcode_display);
	printf("\n");
	printf("Hex Words 2 - 5		: %08x %08x %08x %08x\n",
	       src->ext_refcode2, src->ext_refcode3,
	       src->ext_refcode4, src->ext_refcode5);
	printf("Hex Words 6 - 9		: %08x %08x %08x %08x\n",
	       src->ext_refcode6, src->ext_refcode7,
	       src->ext_refcode8, src->ext_refcode9);
	return 0;
}

int print_mt_scn(struct opal_mtms_scn *mtms)
{
	char model[OPAL_SYS_MODEL_LEN+1];
	char serial_no[OPAL_SYS_SERIAL_LEN+1];

	if (strncmp(mtms->v6hdr.id, "MT", 2)) {
		errno = EFAULT;
		return 0;
	}

	memcpy(model, mtms->model, OPAL_SYS_MODEL_LEN);
	model[OPAL_SYS_MODEL_LEN] = '\0';
	memcpy(serial_no, mtms->serial_no, OPAL_SYS_SERIAL_LEN);
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
	if (strncmp(src->v6hdr.id, "PS", 2)) {
		errno = EFAULT;
		return 0;
	}
	printf("|-------------------------------------------------------------|\n");
	printf("|                     Primary Reference                       |\n");
	printf("|-------------------------------------------------------------|\n");
	printf("Section ID		: %c%c\n",
	       src->v6hdr.id[0],  src->v6hdr.id[1]);
	printf("Section Length		: %x\n", src->v6hdr.length);
	printf("Version			: %x\n", src->v6hdr.version);
	printf("Sub_type		: %x\n", src->v6hdr.subtype);
	printf("Component ID		: %x\n", src->v6hdr.component_id);
	printf("SRC Version		: %x\n", src->version);
	printf("flags			: %x\n", src->flags);
	printf("SRC Length		: %x\n", src->srclength);
	print_src_refcode(src);
	return 0;
}

int print_opal_usr_hdr_scn(struct opal_usr_hdr_scn *usrhdr)
{
	if (strncmp(usrhdr->v6hdr.id, "UH", 2)) {
		errno = EFAULT;
		return 0;
	}
	print_usr_hdr_subsystem_id(usrhdr);
	print_usr_hdr_event_data(usrhdr);
	print_usr_hdr_action(usrhdr);
	return 0;
}

int print_opal_priv_hdr_scn(struct opal_priv_hdr_scn *privhdr)
{
	if (strncmp(privhdr->v6hdr.id, "PH", 2)) {
		errno = EFAULT;
		return 0;
	}
	printf("|-------------------------------------------------------------|\n");
	printf("|                    Private Header                           |\n");
	printf("|-------------------------------------------------------------|\n");
	printf("Section ID		: %c%c\n",
	       privhdr->v6hdr.id[0], privhdr->v6hdr.id[1]);
	printf("Section Length		: 0x%04x (%u)\n",
	       privhdr->v6hdr.length, privhdr->v6hdr.length);
	printf("Version			: %x", privhdr->v6hdr.version);
	if (privhdr->v6hdr.version != 1)
		printf(" UNKNOWN VERSION");
	printf("\nSub_type		: %x\n", privhdr->v6hdr.subtype);
	printf("Component ID		: %x\n", privhdr->v6hdr.component_id);
	printf("Create Date & Time      : %4u-%02u-%02u | %02u:%02u:%02u\n",
	       privhdr->create_datetime.year,
	       privhdr->create_datetime.month,
	       privhdr->create_datetime.day,
	       privhdr->create_datetime.hour,
	       privhdr->create_datetime.minutes,
	       privhdr->create_datetime.seconds);
	printf("Commit Date & Time	: %4u-%02u-%02u | %02u:%02u:%02u\n",
	       privhdr->commit_datetime.year,
	       privhdr->commit_datetime.month,
	       privhdr->commit_datetime.day,
	       privhdr->commit_datetime.hour,
	       privhdr->commit_datetime.minutes,
	       privhdr->commit_datetime.seconds);
	printf("Creator ID		:");
	switch (privhdr->creator_id) {
	case 'C':
		printf(" Hardware Management Console\n");
		break;
	case 'E':
		printf(" Service Processor\n");
		break;
	case 'H':
		printf(" PHYP\n");
		break;
	case 'W':
		printf(" Power Control\n");
		break;
	case 'L':
		printf(" Partition Firmware\n");
		break;
	case 'S':
		printf(" SLIC\n");
		break;
	case 'B':
		printf(" Hostboot\n");
		break;
	case 'T':
		printf(" OCC\n");
		break;
	case 'M':
		printf(" I/O Drawer\n");
		break;
	case 'K':
		printf(" OPAL\n");
		break;
	default:
		printf(" Unknown\n");
		break;
	}
	printf("Creator Subsystem Name	: %x\n", privhdr->creator_subid_hi);
	printf("Platform Log ID		: %x\n", privhdr->plid);
	printf("Log Entry ID		: %x\n", privhdr->log_entry_id);
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
	memcpy(mt.model, bufmt->model, OPAL_SYS_MODEL_LEN);
	memcpy(mt.serial_no, bufmt->serial_no, OPAL_SYS_SERIAL_LEN);

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

static struct opal_datetime parse_opal_datetime(const struct opal_datetime in)
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
			// FIXME required
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
		} else if (strncmp(hdr.id, "CH", 2) == 0) { // FIXME
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
