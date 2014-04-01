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
	char	opal_release_version[OPAL_VER_LEN]; /* Null terminated */
	char	opal_subsys_version[OPAL_VER_LEN]; /* Null terminated */
	uint32_t reserved_0;
	struct opal_datetime event_ref_datetime;
	uint16_t reserved_1;
	uint8_t reserved_2;
	uint8_t opal_symid_len;
	char	opalsymid[0]; /* variable sized */
} __packed;

/* Extended header section */
struct opal_ch_scn {
	struct	opal_v6_hdr v6hdr;
#define OPAL_CH_COMMENT_MAX_LEN 144
	char comment[0]; /* varsized up to 144 byte null terminated */
} __packed;

/* User defined data header section */
struct opal_ud_scn {
	struct opal_v6_hdr v6hdr;
	uint8_t data[0]; /* variable sized */
} __packed;

struct opal_fru_hdr {
  uint16_t type;
  uint8_t length;
  uint8_t flags;
} __packed;

struct opal_fru_id_sub_scn {
#define OPAL_FRU_ID_TYPE 0x4944 /* 'ID' in hex */
	struct opal_fru_hdr hdr;
#define OPAL_FRU_ID_PART 0x08
#define OPAL_FRU_ID_PROC 0x02
#define OPAL_FRU_ID_PART_MAX 8
	char part[OPAL_FRU_ID_PART_MAX];
#define OPAL_FRU_ID_CCIN 0x04 /* OPAL_FRU_ID_PART must be set */
#define OPAL_FRU_ID_CCIN_MAX 4
	char ccin[OPAL_FRU_ID_CCIN_MAX];
#define OPAL_FRU_ID_SERIAL_MAX 12
#define OPAL_FRU_ID_SERIAL 0x1 /* OPAL_FRU_ID_PART must be set */
  char serial[OPAL_FRU_ID_SERIAL_MAX]; /* not null terminated */
} __packed;

struct opal_fru_mr_mru_scn {
  uint8_t reserved0;
  uint8_t reserved1;
  uint8_t reserved2;
  uint8_t priority;
  uint32_t id;
} __packed;

struct opal_fru_mr_sub_scn {
#define OPAL_FRU_MR_TYPE 0x4d52 /* 'MR" in hex */
  struct opal_fru_hdr hdr;
  uint32_t reserved;
#define OPAL_FRU_MR_MRU_MAX 15
  struct opal_fru_mr_mru_scn mru[OPAL_FRU_MR_MRU_MAX]; /*Max 15 */
} __packed;

struct opal_fru_pe_sub_scn {
#define OPAL_FRU_PE_TYPE 0x5045 /* 'PE' in hex */
  struct opal_fru_hdr hdr;
  struct opal_mt_struct mtms;
#define OPAL_FRU_PE_PCE_MAX 32
  char pce[OPAL_FRU_PE_PCE_MAX];
} __packed;

struct opal_src_add_scn_hdr {
  uint8_t id;
#define OPAL_FRU_MORE 0x01
  uint8_t flags;
  uint16_t length;
} __packed;

struct opal_fru_scn {
#define OPAL_FRU_SCN_ID 0xc0
  struct opal_src_add_scn_hdr hdr;
  uint8_t length; /* Length of everything below including this */
  uint8_t type;
  uint8_t priority;
  uint8_t loc_code_len; /* Must be a multiple of 4 */
#define OPAL_FRU_LOC_CODE_MAX 80
  char location_code[OPAL_FRU_LOC_CODE_MAX]; /* Variable length indicated my loc_code_len max 80 */
#define OPAL_FRU_ID_SUB 0x08
  struct opal_fru_id_sub_scn id; /* Optional */
#define OPAL_FRU_PE_SUB 0x03
  struct opal_fru_pe_sub_scn pe; /* Optional */
#define OPAL_FRU_MR_SUB 0x04
  struct opal_fru_mr_sub_scn mr; /* Optional */
} __packed;

/* Primary SRC section */
struct opal_src_scn {
  struct opal_v6_hdr v6hdr;
  uint8_t     version;
  uint8_t     flags;
  uint8_t     reserved_0;
  uint8_t     wordcount;
  uint16_t reserved_1;
  uint16_t srclength;
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
  /* Currently there can only be one type of optional
   * sub section, in the future this may change.
   * This will do for now.
   */
#define OPAL_SRC_ADD_SCN 0x1
#define OPAL_SRC_FRU_MAX 10
  struct opal_fru_scn fru[OPAL_SRC_FRU_MAX]; /*Optional */
  uint8_t fru_count;
} __packed;

struct id_msg{
	int id;
	const char *msg;
};

struct sev_type{
	int sev;
	const char *desc;
};

struct creator_id{
	char id;
	const char *name;
};

struct subsystem_id{
	int id;
	const char *name;
};

struct event_scope{
	int id;
	const char *desc;
};

struct generic_desc{
	int id;
	const char *desc;
};

#define MAX_EVENT	sizeof(usr_hdr_event_type)/sizeof(struct id_msg)
#define MAX_SEV		sizeof(usr_hdr_severity)/sizeof(struct sev_type)
#define MAX_CREATORS		sizeof(prv_hdr_creator_id)/sizeof(struct creator_id)
#define MAX_SUBSYSTEMS		sizeof(usr_hdr_subsystem_id)/sizeof(struct subsystem_id)
#define MAX_EVENT_SCOPE	sizeof(usr_hdr_event_scope)/sizeof(struct event_scope)
#define MAX_FRU_PRIORITY sizeof(fru_scn_priority)/sizeof(struct generic_desc)
#define MAX_FRU_ID_COMPONENT sizeof(fru_id_scn_component)/sizeof(struct generic_desc)
#define EVENT_TYPE \
	{0x01, "Miscellaneous, informational only."},\
	{0x02, "Tracing event"}, \
	{0x08, "Dump notification."},\
	{0x10, "Previously reported error has been corrected by system."},\
	{0x20, "System resources manually deconfigured by user."}, \
	{0x21, "System resources deconfigured by system due to prior error event." }, \
	{0x22, "Resource deallocation event notification."}, \
	{0x30, "Customer environmental problem has returned to normal."}, \
	{0x40, "Concurrent maintenance event."}, \
	{0x60, "Capacity upgrade event."}, \
	{0x70, "Resource sparing event."}, \
	{0x80, "Dynamic reconfiguration event."}, \
	{0xD0, "Normal system/platform shutdown or powered off."}, \
	{0xE0, "Platform powered off by user without normal shutdown."}, \
	{0,    "Unknown event type."}

#define SEVERITY_TYPE \
	{0x00, "Informational or non-error event."}, \
	{0x10, "Recovered error"}, \
	{0x20, "Predictive error"}, \
	{0x21, "Predicting degraded performance."}, \
	{0x22, "Predicting fault may be corrected after platform re-IPL."}, \
	{0x23, "Predicting fault may be corrected after IPL, degraded performance"}, \
	{0x24, "Predicting loss of redundancy"}, \
	{0x40, "Unrecoverable error"}, \
	{0x41, "Error bypassed with degraded performance"}, \
	{0x44, "Error bypassed with loss of redundancy"}, \
	{0x45, "Error bypassed with loss of redundancy and performance"}, \
	{0x48, "Error bypassed with loss of function"}, \
	{0x50, "Critical error"}, \
	{0x51, "Critical error system termination"}, \
	{0x52, "Critical error failure likely or imminent"}, \
	{0x53, "Critical error partition(s) terminal"}, \
	{0x54, "Critical error partition(s) failure likely or imminent"}, \
	{0x60, "Error on diagnostic test"}, \
	{0x61, "Error on diagnostic test, resource may produce incorrect results"}, \
	{0x70, "Symptom"}, \
	{0x71, "Symptom recovered"}, \
	{0x72, "Symptom predictive"}, \
	{0x74, "Symptom unrecoverable"}, \
	{0x75, "Symptom critical"}, \
	{0x76, "Symptom diagnosis error"}

#define CREATORS \
	{'C', "HMC"}, \
	{'E', "Service Processor"}, \
	{'H', "PHYP"}, \
	{'W', "Power Control"}, \
	{'L', "Partition FW"}, \
	{'S', "SLIC"}, \
	{'B', "Hostboot"}, \
	{'T', "OCC"}, \
	{'M', "I/O Drawer"}, \
	{'K', "OPAL"}, \
	{'P', "POWERNV"}

#define SUBSYSTEMS \
	{0x00, "Unknown"}, \
	{0x10, "Processor subsystem"}, \
	{0x11, "Processor FRU"}, \
	{0x12, "Processor chip including internal cache"}, \
	{0x13, "Processor unit (CPU)"}, \
	{0x14, "Processor/system bus controller & interface"}, \
	{0x20, "Memory subsystem"}, \
	{0x21, "Memory controller"}, \
	{0x22, "Memory bus interface including SMI"}, \
	{0x23, "Memory DIMM"}, \
	{0x24, "Memory card/FRU"}, \
	{0x25, "External cache"}, \
	{0x30, "I/O subsystem"}, \
	{0x31, "I/O hub RIO"}, \
	{0x32, "I/O bridge, general (PHB, PCI/PCI, PCI/ISA, EADS, etc.)"}, \
	{0x33, "I/O bus interface"}, \
	{0x34, "I/O processor"}, \
	{0x35, "I/O hub others (SMA, Torrent, etc.)"}, \
	{0x36, "RIO loop and associated RIO hub"}, \
	{0x37, "RIO loop and associated RIO bridge"}, \
	{0x38, "PHB"}, \
	{0x39, "EADS/EADS-X global"}, \
	{0x3a, "EADS/EADS-X slot"}, \
	{0x3b, "InfiniBand hub"}, \
	{0x3c, "Infiniband bridge"}, \
	{0x40, "I/O adapter - I/O deivce - I/O peripheral"}, \
	{0x41, "I/O adapter - communication"}, \
	{0x46, "I/O device"}, \
	{0x47, "I/O device - DASD"}, \
	{0x4c, "I/O peripheral"}, \
	{0x4d, "I/O perpheral - local workstation"}, \
	{0x4e, "Storage mezzanine expansion subsystem"}, \
	{0x50, "CEC hardware"}, \
	{0x51, "CEC hardware - service processor A"}, \
	{0x52, "CEC hardware - service processor B"}, \
	{0x53, "CEC hardware - node controller"}, \
	{0x54, "Reserved for CEC hardware"}, \
	{0x55, "CEC hardware - VPD device and interface (smart chip and I2C device)"}, \
	{0x56, "CEC hardware - I2C devices and interface (non VPD)"}, \
	{0x57, "CEC hardware - CEC chip interface (JTAG, FSI, etc.)"}, \
	{0x58, "CEC hardware - clock & control"}, \
	{0x59, "CEC hardware - Op. panel"}, \
	{0x5a, "CEC hardware - time of day hardware including its battery"}, \
	{0x5b, "CEC hardware - storage/memory device (NVRAM, Flash, SP DRAM, etc.)"}, \
	{0x5c, "CEC hardware - Service processor-Hypervisor hardware interface (PSI, PCI, etc.)"}, \
	{0x5d, "CEC hardware - Service network"}, \
	{0x5e, "CEC hardware - Service processor-Hostboot hardware interface (FSI Mailbox)"}, \
	{0x60, "Power/Cooling subsystem & control"}, \
	{0x61, "Power supply"}, \
	{0x62, "Power control hardware"}, \
	{0x63, "Fan, air moving devices"}, \
	{0x64, "DPSS"}, \
	{0x70, "Others"}, \
	{0x71, "HMC subsystem & hardware (excluding code)"}, \
	{0x72, "Test tool"}, \
	{0x73, "Removable media"}, \
	{0x74, "Multiple subsystems"}, \
	{0x75, "Not applicable (unknown, invalid value, etc.)"}, \
	{0x76, "Reserved"}, \
	{0x77, "CMM A"}, \
	{0x78, "CMM B"}, \
	{0x7a, "Connection Monitoring - Hypervisor lost communication with service processor"}, \
	{0x7b, "Connection Monitoring - Service processor lost communication with hypervisor"}, \
	{0x7c, "Connection Monitoring - Service processor lost communcation with HMC"}, \
	{0x7e, "Connection Monitoring - HMC lost communication with logical partition"}, \
	{0x7e, "Connection Monitoring - HMC lost communication with BPA"}, \
	{0x7f, "Connection Monitoring - HMC lost communication with another HMC"}, \
	{0x80, "Platform firmware"}, \
	{0x81, "Service processor firmware"}, \
	{0x82, "Hypervisor firmware"}, \
	{0x83, "Partition firmware"}, \
	{0x84, "SLIC firmware"}, \
	{0x85, "SPCN firmware"}, \
	{0x86, "Bulk power formware side A"}, \
	{0x87, "HMC code/firmware"}, \
	{0x88, "Bulk power firmware side B"}, \
	{0x89, "Virtual service processor firmware (VSP)"}, \
	{0x8a, "Hostboot"}, \
	{0x8b, "OCC"}, \
	{0x90, "Software"}, \
	{0x91, "Operating system software"}, \
	{0x92, "XPF software"}, \
	{0x93, "Application software"}, \
	{0xa0, "External environment"}, \
	{0xa1, "Input power source (AC)"}, \
	{0xa2, "Room ambient temperature"}, \
	{0xa3, "User error"}, \
	{0xa4, "Unknown"}, \
	{0xb0, "Unknown"}, \
	{0xc0, "Unknown"}, \
	{0xd0, "Unknown"}, \
	{0xe0, "Unknown"}, \
	{0xf0, "Unknown"}

#define EVENT_SCOPE \
	{0x1, "Single partition"}, \
	{0x2, "Multiple partitions"}, \
	{0x3, "Single platform"}, \
	{0x4, "Possibly multiple platforms"}

#define FRU_PRIORITY \
	{'L', "Low Prioirty"}, /*Default Value */ \
	{'H', "Mandatory, replace all with this type as a unit"}, \
	{'M', "Medium Priority"}, \
	{'A', "Medium Priority group A"}, \
	{'B', "Medium Priority group B"}, \
	{'C', "Medium Priority group C"},

#define FRU_ID_COMPONENT \
	{0x00, "Reserved"}, \
	{0x10, "Hardware FRU"}, \
	{0x20, "Code FRU"}, \
	{0x30, "Configuration error"}, \
	{0x40, "Maintenance Procedure required"}, \
	{0x90, "External FRU"}, \
	{0xa0, "External code FRU"}, \
	{0xb0, "Tool FRU"}, \
	{0xc0, "Symbolic FRU"}, \
	{0xe0, "Symbolic FRU with trusted location code"}, \
	{0xf0, "Reserved"}

/* Header ID,
 * Required? (1 = yes, 2 = only with error),
 * Position (0 = no specific pos),
 * Max amount (0 = no max)
 */
struct header_id{
  const char *id;
  int req;
  int pos;
  int max;
};

#define HEADER_ORDER_MAX sizeof(elog_hdr_id)/sizeof(struct header_id)

#define HEADER_NOT_REQ 0x0
#define HEADER_REQ 0x1
#define HEADER_REQ_W_ERROR 0x2

#define HEADER_ORDER \
  {"PH", 1, 1, 1}, \
  {"UH", 1, 2, 1}, \
  {"PS", 2, 3, 1}, \
  {"EH", 1, 0, 1}, \
  {"MT", 2, 0, 1}, \
  {"SS", 0, 0, -1}, \
  {"DH", 0, 0, 1}, \
  {"SW", 0, 0, -1}, \
  {"LP", 0, 0 ,1}, \
  {"LR", 0, 0, 1}, \
  {"HM", 0, 0, 1}, \
  {"EP", 0, 0, 1}, \
  {"IE", 0, 0, 1}, \
  {"MI", 0, 0, 1}, \
  {"CH", 0, 0, 1}, \
  {"UD", 0, 0, -1}, \
  {"EI", 0, 0, 1}, \
  {"ED", 0, 0, -1}

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
#define OPAL_UH_ACTION_TERMINATION       0x0100
	uint32_t	reserved2;
} __packed;

extern int parse_opal_event(char *, int);
extern struct opal_datetime parse_opal_datetime(const struct opal_datetime);
extern const char *get_creator_name(uint8_t);
#endif
