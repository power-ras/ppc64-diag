#ifndef _H_OPAL_USR_SCN
#define _H_OPAL_USR_SCN

#include "opal-v6-hdr.h"

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

#define OPAL_UH_ACTION_SERVICE           0x8000
#define OPAL_UH_ACTION_HEALTH            0x4000
#define OPAL_UH_ACTION_REPORT_EXTERNALLY 0x2000
#define OPAL_UH_ACTION_HMC_ONLY          0x1000
#define OPAL_UH_ACTION_CALL_HOME         0x0800
#define OPAL_UH_ACTION_ISO_INCOMPLETE    0x0400
#define OPAL_UH_ACTION_TERMINATION       0x0100

struct opal_usr_hdr_scn {
	struct opal_v6_hdr v6hdr;
	uint8_t    subsystem_id;     /**< subsystem id */
	uint8_t    event_data;
	uint8_t    event_severity;
	uint8_t    event_type;       /**< error/event severity */
	uint32_t   reserved1;
	uint8_t    problem_domain;
	uint8_t    problem_vector;
	uint16_t   action;          /**< erro action code */
	uint32_t   reserved2;
} __attribute__((packed));

int parse_usr_hdr_scn(struct opal_usr_hdr_scn **r_usrhdr,
                      const struct opal_v6_hdr *hdr,
                      const char *buf, int buflen, int *is_error);

int print_opal_usr_hdr_scn(const struct opal_usr_hdr_scn *usrhdr);

#endif /* _H_OPAL_USR_SCN */
