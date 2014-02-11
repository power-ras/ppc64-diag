#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
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
	printf("Section ID		: %x\n", usrhdr->v6hdr.id);
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
	int i;
	printf("Primary Reference Code	: ");
	for (i = 0; i < 32; i++) {
		if (src->primary_refcode[i] == '\0')
			break;
		printf("%c", src->primary_refcode[i]);
	}
	printf("\n");
	printf("Hex Words 2 - 5		: %08x %08x %08x %08x\n", src->ext_refcode2,
						src->ext_refcode3, src->ext_refcode4, src->ext_refcode5);
	printf("Hex Words 6 - 9		: %08x %08x %08x %08x\n", src->ext_refcode6,
						src->ext_refcode7, src->ext_refcode8, src->ext_refcode9);
	return 0;
}

int print_mt_scn(struct opal_mtms_scn *mtms)
{
	if (mtms->v6hdr.id != OPAL_MTMS_SCN) {
		errno = EFAULT;
		return 0;
	}
	printf("|-------------------------------------------------------------|\n");
	printf("|              Machine Type/Model & Serial Number	      |\n");
	printf("|-------------------------------------------------------------|\n");
	printf("Section ID		: %x\n", mtms->v6hdr.id);
	printf("Section Length		: %x\n", mtms->v6hdr.length);
	printf("Version			: %x\n", mtms->v6hdr.version);
	printf("Sub_type		: %x\n", mtms->v6hdr.subtype);
	printf("Component ID		: %x\n", mtms->v6hdr.component_id);
	printf("Machine Type Model	: %s\n", mtms->model);
	printf("Serial Number		: %s\n", mtms->serial_no);
	printf("|-------------------------------------------------------------|\n\n");
	return 0;
}

int print_opal_src_scn(struct opal_src_scn *src)
{
	if (src->v6hdr.id != OPAL_PRIMARY_SRC_SCN) {
		errno = EFAULT;
		return 0;
	}
	printf("|-------------------------------------------------------------|\n");
	printf("|                     Primary Reference                       |\n");
	printf("|-------------------------------------------------------------|\n");
	printf("Section ID		: %x\n", src->v6hdr.id);
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
	if (usrhdr->v6hdr.id != OPAL_USR_HDR_SCN) {
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
	if (privhdr->v6hdr.id != OPAL_PRIV_HDR_SCN) {
		errno = EFAULT;
		return 0;
	}
	printf("|-------------------------------------------------------------|\n");
	printf("|                    Private Header                           |\n");
	printf("|-------------------------------------------------------------|\n");
	printf("Section ID		: %x\n", privhdr->v6hdr.id);
	printf("Section Length		: %x\n", privhdr->v6hdr.length);
	printf("Version			: %x\n", privhdr->v6hdr.version);
	printf("Sub_type		: %x\n", privhdr->v6hdr.subtype);
	printf("Component ID		: %x\n", privhdr->v6hdr.component_id);
	printf("Create Date & Time      : %x-%x-%x | %x:%x:%x\n", privhdr->create_date.year,
		privhdr->create_date.month, privhdr->create_date.day, privhdr->create_time.hour,
		privhdr->create_time.minutes, privhdr->create_time.seconds);
	printf("Commit Date & Time	: %x-%x-%x | %x:%x:%x\n", privhdr->commit_date.year,
		privhdr->commit_date.month, privhdr->commit_date.day, privhdr->commit_time.hour,
		privhdr->commit_time.minutes, privhdr->commit_time.seconds);
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
int parse_mt_scn(char *buf, int buflen)
{
	struct opal_mtms_scn *mt;
	uint16_t id = 0;
check:
	memcpy(&id, buf, 2);
	if (id == OPAL_MTMS_SCN)
		goto out;
	buf = buf + 2;
	goto check;
out:
	mt = malloc(sizeof(*mt));
	if (mt == NULL) {
		errno = ENOMEM;
		return 1;
	}
	memset(mt, 0, sizeof(*mt));
	memcpy(mt, buf, OE_MT_SCN_SZ);
	print_mt_scn(mt);
	return OE_MT_SCN_SZ;
}

/* parse SRC section of the log */
int parse_src_scn(char *buf, int buflen)
{
	struct opal_src_scn *src;

	src = malloc(sizeof(*src));
	if (src == NULL) {
		errno = ENOMEM;
		return 1;
	}
	memset(src, 0, sizeof(*src));
	memcpy(src, buf, OE_SRC_SCN_SZ);
	print_opal_src_scn(src);
	return OE_SRC_SCN_SZ + OE_SRC_EH_SZ;
}

/* parse private header scn */
int parse_priv_hdr_scn(char *buf, int buflen)
{
	struct opal_priv_hdr_scn *privhdr;
	privhdr = malloc(sizeof(*privhdr));
	if (privhdr == NULL) {
		errno = ENOMEM;
		return -1;
	}
	memset(privhdr, 0, sizeof(*privhdr));
	memcpy(privhdr, buf, OE_PRVT_HDR_SCN_SZ);
	print_opal_priv_hdr_scn(privhdr);
	return OE_PRVT_HDR_SCN_SZ;
}

/* parse_usr_hdr_scn */
int parse_usr_hdr_scn(char *buf, int buflen)
{
	struct opal_usr_hdr_scn *usrhdr;
	usrhdr = malloc(sizeof(*usrhdr));
	if (usrhdr == NULL) {
		errno = ENOMEM;
		return -1;
	}
	memset(usrhdr, 0, sizeof(*usrhdr));
	memcpy(usrhdr, buf, OE_USR_HDR_SCN_SZ);
	print_opal_usr_hdr_scn(usrhdr);
	return OE_USR_HDR_SCN_SZ;
}

/* parse all required sections of the log */
int parse_opal_event(char *buf, int buflen)
{
	static int len;
	len = parse_priv_hdr_scn(buf, buflen);
	if (len < buflen) {
		buflen -= len;
		buf = buf + len;
		len = parse_usr_hdr_scn(buf, buflen);
	}
	if (len < buflen) {
		buflen -= len;
		buf = buf + len;
		len = parse_src_scn(buf, buflen);
	}
	if (len < buflen) {
		buflen -= len;
		buf = buf + len;
		len = parse_mt_scn(buf, buflen);
	}
	return 0;
}
