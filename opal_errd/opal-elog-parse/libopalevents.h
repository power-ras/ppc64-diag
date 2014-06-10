#ifndef _H_OPAL_EVENTS
#define _H_OPAL_EVENTS

#include <stdio.h>
#include <inttypes.h>

#ifndef __packed
#define __packed __attribute__((packed))
#endif

#include "opal-datetime.h"
#include "opal-v6-hdr.h"
#include "opal-priv-hdr-scn.h"
#include "opal-mtms-scn.h"
#include "opal-lr-scn.h"
#include "opal-eh-scn.h"
#include "opal-ep-scn.h"
#include "opal-sw-scn.h"

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

struct opal_hm_scn {
	struct opal_v6_hdr v6hdr;
	struct opal_mtms_struct mtms;
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
  struct opal_mtms_struct mtms;
#define OPAL_FRU_PE_PCE_MAX 32
  char pce[OPAL_FRU_PE_PCE_MAX];
} __packed;

struct opal_fru_scn {
#define OPAL_FRU_SCN_STATIC_SIZE (4 * sizeof(uint8_t))
#define OPAL_FRU_SCN_ID 0xc0
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

struct opal_src_add_scn_hdr {
  uint8_t id;
#define OPAL_FRU_MORE 0x01
  uint8_t flags;
  uint16_t length; /* This is counted in words */
} __packed;

/* SRC section */
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
#define OPAL_SRC_SCN_STATIC_SIZE sizeof(struct opal_src_scn) \
	- sizeof(struct opal_src_add_scn_hdr) \
	- (OPAL_SRC_FRU_MAX * sizeof(struct opal_fru_scn)) \
	- sizeof(uint8_t)
	struct opal_src_add_scn_hdr addhdr;
#define OPAL_SRC_ADD_SCN 0x1
#define OPAL_SRC_FRU_MAX 10
	struct opal_fru_scn fru[OPAL_SRC_FRU_MAX]; /*Optional */
	uint8_t fru_count;
} __packed;

struct opal_lp_scn {
	struct opal_v6_hdr v6hdr;
	uint16_t primary;
	uint8_t length_name;
	uint8_t lp_count;
	uint32_t partition_id;
	char name[0]; /* variable length */
	/* uint16_t *lps; variable length
	 * exists after name, position will be name + length name
	 */
} __packed;

struct opal_ie_scn {
	struct opal_v6_hdr v6hdr;
#define IE_TYPE_ERROR_DET 0x01
#define IE_TYPE_ERROR_REC 0x02
#define IE_TYPE_EVENT 0x03
#define IE_TYPE_RPC_PASS_THROUGH 0x04
	uint8_t type;
	uint8_t rpc_len;
	uint8_t scope;
#define IE_SUBTYPE_REBALANCE 0x01
#define IE_SUBTYPE_NODE_ONLINE 0x03
#define IE_SUBTYPE_NODE_OFFLINE 0x04
#define IE_SUBTYPE_PLAT_MAX_CHANGE 0x05
	uint8_t subtype;
	uint32_t drc;
#define IE_DATA_MAX 216
	union {
		uint8_t rpc[IE_DATA_MAX];
		uint64_t max;
	} data;
} __packed;

struct opal_mi_scn {
	struct opal_v6_hdr v6hdr;
	uint32_t flags;
	uint32_t reserved;
} __packed;

struct opal_ei_env_scn {
	uint32_t corrosion;
	uint16_t temperature;
	uint16_t rate;
} __packed;

struct opal_ei_scn {
	struct opal_v6_hdr v6hdr;
	uint64_t g_timestamp;
	struct opal_ei_env_scn genesis;
#define CORROSION_RATE_NORM 0x00
#define CORROSION_RATE_ABOVE 0x01
	uint8_t status;
	uint8_t user_data_scn;
	uint16_t read_count;
	struct opal_ei_env_scn readings[0]; /* variable length */
} __packed;

struct opal_ed_scn {
	struct opal_v6_hdr v6hdr;
	uint8_t creator_id;
	uint8_t reserved[3];
	uint8_t user_data[0]; /* variable length */
} __packed;

struct opal_dh_scn {
	struct opal_v6_hdr v6hdr;
	uint32_t dump_id;
#define DH_FLAG_DUMP_HEX 0x40
	uint8_t flags;
	uint8_t reserved[2];
	uint8_t length_dump_os;
	uint64_t dump_size;
#define DH_DUMP_STR_MAX 40
	union {
		char dump_str[DH_DUMP_STR_MAX];
		uint32_t dump_hex;
	} shared;
} __packed;

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
#define OPAL_UH_ACTION_HEALTH            0x4000
#define OPAL_UH_ACTION_REPORT_EXTERNALLY 0x2000
#define OPAL_UH_ACTION_HMC_ONLY          0x1000
#define OPAL_UH_ACTION_CALL_HOME         0x0800
#define OPAL_UH_ACTION_ISO_INCOMPLETE    0x0400
#define OPAL_UH_ACTION_TERMINATION       0x0100
	uint32_t	reserved2;
} __packed;

extern struct opal_datetime parse_opal_datetime(const struct opal_datetime);
#endif
