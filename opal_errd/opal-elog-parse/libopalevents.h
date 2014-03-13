#ifndef _H_OPAL_EVENTS
#define _H_OPAL_EVENTS

#include <stdio.h>
#include <inttypes.h>

#define OPAL_EVENT_HDR		1
#define OE_EVENT_HDR_SZ		8
#define OE_PRVT_HDR_SCN_SZ	48
#define OE_USR_HDR_SCN_SZ	24
#define OE_SRC_SCN_SZ		80
#define OE_SRC_SUBSCN_SZ	4
#define OE_FRU_SCN_SZ		4
#define OE_FRU_HDR_SZ		4
#define OPAL_SYS_MODEL_LEN	8
#define OPAL_SYS_SERIAL_LEN	12
#define OPAL_VER_LEN		16
#define OPAL_SYMPID_LEN		80
#define OE_SRC_EH_SZ		76
#define OE_MT_SCN_SZ		28

#ifndef __packed
#define __packed __attribute__((packed))
#endif

/* This comes in BCD for some reason, we convert it in parsing */
struct opal_datetime {
	uint16_t   year;
	uint8_t    month;
	uint8_t    day;
	uint8_t    hour;
	uint8_t    minutes;
	uint8_t    seconds;
	uint8_t    hundredths;
} __packed;


/* Error log section ID's */
enum elogSectionId {
	OPAL_PRIV_HDR_SCN	= 0x5048, /* PH */
	OPAL_USR_HDR_SCN	= 0x5548, /* UH */
	OPAL_EH_HDR_SCN		= 0x4548, /* EH */
	OPAL_PRIMARY_SRC_SCN	= 0x5053, /* PS */
	OPAL_MTMS_SCN		= 0x4D54, /* MT */
};

/* Error log section header */
struct opal_v6_hdr {
	char            id[2];
	uint16_t	length;		/* section length */
	uint8_t		version;	/* section version */
	uint8_t		subtype;	/* section sub-type id */
	uint16_t	component_id;	/* component id of section creator */
} __packed;

/* opal MTMS section */
struct	opal_mtms_scn {
	struct	opal_v6_hdr v6hdr;
	char	model[OPAL_SYS_MODEL_LEN];
	char	serial_no[OPAL_SYS_SERIAL_LEN];
};

/* Extended header section */
struct opal_eh_scn {
	struct	opal_v6_hdr v6hdr;
	char	model[OPAL_SYS_MODEL_LEN];
	char	serial_no[OPAL_SYS_SERIAL_LEN];
	char	opal_release_version[OPAL_VER_LEN];
	char	opal_subsys_version[OPAL_VER_LEN];
	uint16_t reserved_0;
	uint32_t extended_header_date;
	uint32_t extended_header_time;
	uint16_t reserved_1;
	uint8_t reserved_2;
	uint8_t opal_symid_len;
	char	opalsymid[OPAL_SYMPID_LEN];
};

struct id_msg{
	int id;
	const char *msg;
};

struct sev_type{
	int sev;
	const char *desc;
};

#define MAX_EVENT	sizeof(usr_hdr_event_type)/sizeof(struct id_msg)
#define MAX_SEV		sizeof(usr_hdr_severity)/sizeof(struct sev_type)

#define EVENT_TYPE \
	{0x01, " Miscellaneous, informational only."},\
	{0x08, " Dump notification."},\
	{0x10, " Previously reported error has been corrected by system."},\
	{0x20, " System resources manually ."}, \
	{0x21, " System resources deconfigured by system due" }, \
	{0x22, " Resource deallocation event notification."}, \
	{0x30, " Customer environmental problem has returned to normal."}, \
	{0x40, " Concurrent maintenance event."}, \
	{0x60, " Capacity upgrade event."}, \
	{0x70, " Resource sparing event."}, \
	{0x80, " Dynamic reconfiguration event."}, \
	{0xD0, " Normal system/platform shutdown or powered off."}, \
	{0xE0, " Platform powered off by user without normal shutdown."}, \
	{0,    " Unknown event type."}

#define SEVERITY_TYPE \
	{0x00, " Informational or non-error event."}, \
	{0x10, " Recovered error, general."}, \
	{0x20, " Predictive error, general."}, \
	{0x21, " Predictive error, degraded performance."}, \
	{0x22, " Predictive error, fault may be corrected after platform re-IPL."}

/* Primary SRC section */
struct opal_src_scn {
	struct opal_v6_hdr v6hdr;
	uint8_t		version;
	uint8_t		flags;
	uint8_t		reserved_0;
	uint8_t		wordcount;
	uint16_t	reserved_1;
	uint16_t	srclength;
	uint32_t    ext_refcode2:32;
	uint32_t    ext_refcode3:32;
	uint32_t    ext_refcode4:32;
	uint32_t    ext_refcode5:32;
	uint32_t    ext_refcode6:32;
	uint32_t    ext_refcode7:32;
	uint32_t    ext_refcode8:32;
	uint32_t    ext_refcode9:32;
	char        primary_refcode[36];
	uint32_t    subscn_id:8;
	uint32_t    subscn_platform_data:8;
	uint32_t    subscn_length:16;
	struct opal_fru_scn *fru_scns;
};

/* Private Header section */
struct opal_priv_hdr_scn {
	struct opal_v6_hdr v6hdr;
	struct opal_datetime create_datetime;
	struct opal_datetime commit_datetime;
	uint8_t creator_id;	/* subsystem component id */
#define OPAL_PH_CREAT_SERVICE_PROC   'E'
#define OPAL_PH_CREAT_HYPERVISOR     'H'
#define OPAL_PH_CREAT_POWER_CONTROL  'W'
#define OPAL_PH_CREAT_PARTITION_FW   'L'

	uint8_t reserved0;
	uint8_t reserved1;
	uint8_t scn_count;	/* number of sections in log */
	uint32_t reserved2;
	uint32_t creator_subid_hi;
	uint32_t creator_subid_lo;
	uint32_t plid;		/* platform log id */
	uint32_t log_entry_id;	/* Unique log entry id */
} __packed;

/* user header section */
struct opal_usr_hdr_scn {
	struct opal_v6_hdr v6hdr;
	uint32_t    subsystem_id:8;     /**< subsystem id */
	uint32_t    event_data:8;
	uint32_t    event_severity:8;
	uint32_t    event_type:8;       /**< error/event severity */
#define OPAL_UH_TYPE_NA                   0x00
#define OPAL_UH_TYPE_INFO_ONLY            0x01
#define OPAL_UH_TYPE_DUMP_NOTIFICATION    0x08
#define OPAL_UH_TYPE_PREVIOUSLY_REPORTED  0x10
#define OPAL_UH_TYPE_DECONFIG_USER        0x20
#define OPAL_UH_TYPE_DECONFIG_SYSTEM      0x21
#define OPAL_UH_TYPE_DECONFIG_NOTICE      0x22
#define OPAL_UH_TYPE_RETURN_TO_NORMAL     0x30
#define OPAL_UH_TYPE_CONCURRENT_MAINT     0x40
#define OPAL_UH_TYPE_CAPACITY_UPGRADE     0x60
#define OPAL_UH_TYPE_RESOURCE_SPARING     0x70
#define OPAL_UH_TYPE_DYNAMIC_RECONFIG     0x80
#define OPAL_UH_TYPE_NORMAL_SHUTDOWN      0xD0
#define OPAL_UH_TYPE_ABNORMAL_SHUTDOWN    0xE0

	uint32_t    reserved1:32;
	uint32_t    reserved2:16;
	uint32_t    action:16;          /**< erro action code */
#define OPAL_UH_ACTION_SERVICE           0x8000
#define OPAL_UH_ACTION_HIDDEN            0x4000
#define OPAL_UH_ACTION_REPORT_EXTERNALLY 0x2000
#define OPAL_UH_ACTION_HMC_ONLY          0x1000
#define OPAL_UH_ACTION_CALL_HOME         0x0800
#define OPAL_UH_ACTION_ISO_INCOMPLETE    0x0400

	uint32_t	reserved3:32;
};

extern int parse_opal_event(char *, int);
#endif
