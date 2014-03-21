#ifndef _H_OPAL_EVENTS
#define _H_OPAL_EVENTS

#include <stdio.h>
#include <inttypes.h>

#define OPAL_SYS_MODEL_LEN	8
#define OPAL_SYS_SERIAL_LEN	12
#define OPAL_VER_LEN		16

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

/* Error log section header */
struct opal_v6_hdr {
	char            id[2];
	uint16_t	length;		/* section length */
	uint8_t		version;	/* section version */
	uint8_t		subtype;	/* section sub-type id */
	uint16_t	component_id;	/* component id of section creator */
} __packed;

struct opal_mt_struct {
	char	model[OPAL_SYS_MODEL_LEN];
	char	serial_no[OPAL_SYS_SERIAL_LEN];
} __packed;

/* opal MTMS section */
struct	opal_mtms_scn {
	struct	opal_v6_hdr v6hdr;
	struct opal_mt_struct mt;
} __packed;

/* Extended header section */
struct opal_eh_scn {
	struct	opal_v6_hdr v6hdr;
	struct opal_mt_struct mt;
	char	opal_release_version[OPAL_VER_LEN]; // Null terminated
	char	opal_subsys_version[OPAL_VER_LEN]; // Null terminated
	uint32_t reserved_0;
	struct opal_datetime event_ref_datetime;
	uint16_t reserved_1;
	uint8_t reserved_2;
	uint8_t opal_symid_len;
	char	opalsymid[0]; // variable sized
} __packed;

/* Extended header section */
struct opal_ch_scn {
	struct	opal_v6_hdr v6hdr;
#define OPAL_CH_COMMENT_MAX_LEN 144
	char comment[0]; // varsized up to 144 byte null terminated
} __packed;

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
	{0x20, " System resources manually deconfigured by user."}, \
	{0x21, " System resources deconfigured by system due to prior error event."}, \
	{0x22, " Resource deallocation event notification."}, \
	{0x30, " Customer environmental problem has returned to normal."}, \
	{0x40, " Concurrent maintenance event."}, \
	{0x60, " Capacity upgrade event."}, \
	{0x70, " Resource sparing event."}, \
	{0x80, " Dynamic reconfiguration event."}, \
	{0xD0, " Normal system/platform shutdown or powered off."}, \
	{0xE0, " Platform powered off by user without normal shutdown."}, \
	{0,    " Not applicable."}

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
	uint32_t    ext_refcode2;
	uint32_t    ext_refcode3;
	uint32_t    ext_refcode4;
	uint32_t    ext_refcode5;
	uint32_t    ext_refcode6;
	uint32_t    ext_refcode7;
	uint32_t    ext_refcode8;
	uint32_t    ext_refcode9;
#define OPAL_SRC_SCN_PRIMARY_REFCODE_LEN 32
	char        primary_refcode[OPAL_SRC_SCN_PRIMARY_REFCODE_LEN];

	/* Optional subsection header
	 * TODO: should be struct as there could be several
	 */
	/* uint8_t    subscn_id;
	 * uint8_t    subscn_flags;
	 * uint16_t    subscn_length;
	 */
	/* and the subsection of length subscn_length would go here */
	/* but we currently don't parse it */
} __packed;

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
	uint8_t    subsystem_id;     /**< subsystem id */
	uint8_t    event_data;
	uint8_t    event_severity;
	uint8_t    event_type;       /**< error/event severity */
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

	uint32_t    reserved1;
	uint8_t     problem_domain;
	uint8_t     problem_vector;
	uint16_t    action;          /**< erro action code */
#define OPAL_UH_ACTION_SERVICE           0x8000
#define OPAL_UH_ACTION_HIDDEN            0x4000
#define OPAL_UH_ACTION_REPORT_EXTERNALLY 0x2000
#define OPAL_UH_ACTION_HMC_ONLY          0x1000
#define OPAL_UH_ACTION_CALL_HOME         0x0800
#define OPAL_UH_ACTION_ISO_INCOMPLETE    0x0400
	uint32_t	reserved2;
} __packed;

extern int parse_opal_event(char *, int);
extern struct opal_datetime parse_opal_datetime(const struct opal_datetime);
#endif
