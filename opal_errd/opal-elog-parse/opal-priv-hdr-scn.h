#ifndef _H_OPAL_PRIV_HEADER
#define _H_OPAL_PRIV_HEADER

#include "opal-v6-hdr.h"
#include "opal-datetime.h"

#define OPAL_PH_CREAT_SERVICE_PROC   'E'
#define OPAL_PH_CREAT_HYPERVISOR     'H'
#define OPAL_PH_CREAT_POWER_CONTROL  'W'
#define OPAL_PH_CREAT_PARTITION_FW   'L'

/* Private Header section */
struct opal_priv_hdr_scn {
	struct opal_v6_hdr v6hdr;
	struct opal_datetime create_datetime;
	struct opal_datetime commit_datetime;
	uint8_t creator_id;     /* subsystem component id */
	uint8_t reserved0;
	uint8_t reserved1;
	uint8_t scn_count;      /* number of sections in log */
	uint32_t reserved2;
	uint32_t creator_subid_hi;
	uint32_t creator_subid_lo;
	uint32_t plid;          /* platform log id */
	uint32_t log_entry_id;  /* Unique log entry id */
} __attribute__((packed));

int parse_priv_hdr_scn(struct opal_priv_hdr_scn **r_privhdr,
                       const struct opal_v6_hdr *hdr, const char *buf,
                       int buflen);

int print_opal_priv_hdr_scn(const struct opal_priv_hdr_scn *privhdr);

#endif /* _H_OPAL_PRIV_HEADER */
